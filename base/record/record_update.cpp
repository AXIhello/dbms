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
    this->table_name = tableName;
    if (!table_exists(tableName)) throw std::runtime_error("表 '" + table_name + "' 不存在。");

    std::vector<FieldBlock> fields = read_field_blocks(table_name);
    this->table_structure = read_table_structure_static(table_name);
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

    while (infile.peek() != EOF) {
        std::unordered_map<std::string, std::string> record_data;
        uint64_t row_id = 0;

        if (read_record_from_file(infile, fields, record_data, row_id, /*skip_deleted=*/true)) {
            if (condition.empty() || matches_condition(record_data, false)) {
                for (const auto& [col, val] : updates) {
                    record_data[col] = val;
                }

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

                updated++;
            }

            // 保留原 row_id
            all_records.emplace_back(row_id, record_data);
        }
    }
    infile.close();

    // 使用稳定排序，保持相同 row_id 的记录按原始顺序排列
    std::stable_sort(all_records.begin(), all_records.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
        });

    std::ofstream outfile(trd_path, std::ios::binary | std::ios::trunc);
    if (!outfile) throw std::runtime_error("无法打开数据文件进行写入。");

    for (const auto& [row_id, record] : all_records) {
        // 先写入 row_id
        outfile.write(reinterpret_cast<const char*>(&row_id), sizeof(uint64_t));

        // 再写入 delete_flag
        char delete_flag = 0;
        outfile.write(&delete_flag, sizeof(char));

        // 最后写入字段值
        for (const auto& field : fields) {
            std::string field_name(field.name);
            write_field(outfile, field, record.at(field_name));
        }
    }
    outfile.close();

    // 更新最后修改时间（注意：update 不改记录数）
    dbManager::getInstance().get_current_database()->getTable(table_name)->setLastModifyTime(std::time(nullptr));
    return updated;
}
