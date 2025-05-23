#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "Record.h"
#include "parse/parse.h"
#include "ui/output.h"

#include <regex>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <algorithm>

int Record::update(const std::string& tableName, const std::string& setClause, const std::string& condition) {
    auto& transaction = TransactionManager::instance();
    transaction.beginImplicitTransaction();

    this->table_name = tableName;
    if (!table_exists(tableName)) throw std::runtime_error("表 '" + table_name + "' 不存在。");

    std::vector<FieldBlock> fields = read_field_blocks(table_name);
    this->table_structure = read_table_structure_static(table_name);

    // 设置 columns，供索引更新用
    columns.clear();
    for (const auto& field : fields) {
        columns.push_back(field.name);
    }

    if (!condition.empty()) parse_condition(condition);

    std::unordered_map<std::string, std::string> updates;
    std::istringstream ss(setClause);
    std::string pair;
    while (std::getline(ss, pair, ',')) {
        size_t eq = pair.find('=');
        if (eq == std::string::npos) throw std::runtime_error("SET语法错误: " + pair);
        std::string col = pair.substr(0, eq);
        std::string val = pair.substr(eq + 1);
        col.erase(0, col.find_first_not_of(" \t"));
        col.erase(col.find_last_not_of(" \t") + 1);
        val.erase(0, val.find_first_not_of(" \t"));
        val.erase(val.find_last_not_of(" \t") + 1);
        if (table_structure.find(col) == table_structure.end())
            throw std::runtime_error("字段不存在: " + col);
        if (!is_valid_type(val, table_structure[col]))
            throw std::runtime_error("值类型不匹配: " + col + " = " + val);
        updates[col] = val;
    }

    std::string trd_path = dbManager::getInstance().get_current_database()->getDBPath() + "/" + table_name + ".trd";
    std::ifstream infile(trd_path, std::ios::binary);
    if (!infile) throw std::runtime_error("无法打开数据文件。");

    int updated = 0;
    std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> all_records;
    std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> undo_records;

    while (infile.peek() != EOF) {
        std::unordered_map<std::string, std::string> record_data;
        uint64_t row_id = 0;

        if (read_record_from_file(infile, fields, record_data, row_id, /*skip_deleted=*/true)) {
            if (condition.empty() || matches_condition(record_data, false)) {
                // 事务处理
                if (transaction.isActive()) {
                    try {
                        std::vector<std::pair<std::string, std::string>> oldValues, newPairs;
                        for (const auto& [col, _] : updates) {
                            oldValues.emplace_back(col, record_data[col]);
                        }
                        transaction.addUndo(DmlType::UPDATE, table_name, row_id, oldValues);

                        for (const auto& [col, val] : updates) {
                            newPairs.emplace_back(col, val);
                        }

                        LogManager::instance().logUpdate(table_name, row_id, oldValues, newPairs);
                        transaction.commitImplicitTransaction();
                    }
                    catch (const std::exception& e) {
                        transaction.rollback();
                        std::cerr << "更新操作失败: " << e.what() << std::endl;
                        throw;
                    }
                }

                // 构造 oldValues/newValues 用于更新索引
                std::vector<std::string> oldValues, newValues;
                for (const auto& [col, _] : updates) {
                    oldValues.push_back(record_data[col]);
                }

                undo_records.emplace_back(row_id, record_data); // 记录旧数据

                for (const auto& [col, val] : updates) {
                    record_data[col] = val;
                    newValues.push_back(val);
                }

                // 约束检查
                std::vector<std::string> cols, vals;
                for (const auto& field : fields) {
                    std::string field_name(field.name);
                    cols.push_back(field_name);
                    vals.push_back(record_data[field_name]);
                }
                std::vector<ConstraintBlock> constraints = read_constraints(table_name);
                if (!check_constraints(cols, vals, constraints)) {
                    throw std::runtime_error("更新数据违反表约束");
                }

                // 更新索引
                updateIndexesAfterUpdate(table_name, oldValues, newValues, RecordPointer{ row_id });
                updated++;
            }

            all_records.emplace_back(row_id, record_data); // 无论是否更新都保留
        }
    }
    infile.close();

    // 写回文件
    std::stable_sort(all_records.begin(), all_records.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
        });

    std::ofstream outfile(trd_path, std::ios::binary | std::ios::trunc);
    if (!outfile) throw std::runtime_error("无法打开数据文件进行写入。");

    for (const auto& [row_id, record] : all_records) {
        outfile.write(reinterpret_cast<const char*>(&row_id), sizeof(uint64_t));
        char delete_flag = 0;
        outfile.write(&delete_flag, sizeof(char));
        for (const auto& field : fields) {
            write_field(outfile, field, record.at(field.name));
        }
    }
    outfile.close();

    dbManager::getInstance().get_current_database()->getTable(table_name)->setLastModifyTime(std::time(nullptr));
    return updated;
}


void Record::updateByRowid(uint64_t rowId, const std::vector<std::pair<std::string, std::string>>& newValues) {
    std::string file_path = dbManager::getInstance().get_current_database()->getDBPath() + "/" + this->table_name + ".trd";
    std::ofstream file(file_path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file) throw std::runtime_error("无法打开文件进行更新");

    std::vector<FieldBlock> fields = read_field_blocks(this->table_name);
    size_t record_size = sizeof(uint64_t) + sizeof(char);
    for (const auto& f : fields) {
        record_size += get_field_size(f);
    }

    std::streampos pos = (rowId - 1) * record_size;
    file.seekp(std::streamoff(pos) + sizeof(uint64_t) + sizeof(char), std::ios::beg);

    std::unordered_map<std::string, std::string> val_map;
    for (const auto& [col, val] : newValues) {
        val_map[col] = val;
    }

    for (const auto& field : fields) {
        std::string value = val_map.count(field.name) ? val_map[field.name] : "NULL";
        write_field(file, field, value);
    }

    file.close();
}
