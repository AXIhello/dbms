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

int Record::delete_(const std::string& tableName, const std::string& condition) {
    auto& transaction = TransactionManager::instance();
    transaction.beginImplicitTransaction();  // 自动开启事务

    this->table_name = tableName;
    if (!table_exists(this->table_name)) {
        throw std::runtime_error("表 '" + this->table_name + "' 不存在。");
    }

    std::vector<FieldBlock> fields = read_field_blocks(table_name);
    this->table_structure = read_table_structure_static(table_name);
    if (!condition.empty()) parse_condition(condition);

    std::string trd_path = dbManager::getInstance().get_current_database()->getDBPath() + "/" + table_name + ".trd";

    int deleted_count = 0;

    if (transaction.isActive()) {
        std::fstream file(trd_path, std::ios::binary | std::ios::in | std::ios::out);
        if (!file) throw std::runtime_error("无法打开数据文件进行删除操作。");

        std::ifstream infile(trd_path, std::ios::binary);
        if (!infile) throw std::runtime_error("无法打开数据文件进行读取操作。");

        try {
            while (true) {
                std::streampos pos_before = infile.tellg();
                if (pos_before == -1) break;

                uint64_t row_id = 1;
                std::unordered_map<std::string, std::string> record_data;

                if (!read_record_from_file(infile, fields, record_data, row_id, /*skip_deleted=*/false)) {
                    break;
                }

                if (condition.empty() || matches_condition(record_data, false)) {
                    if (!check_references_before_delete(table_name, record_data)) {
                        throw std::runtime_error("删除操作违反引用完整性约束");
                    }

                    // 标记删除
                    file.seekp(static_cast<std::streamoff>(pos_before) + sizeof(uint64_t));
                    char flag = 1;
                    file.write(&flag, sizeof(char));
                    file.flush();

                    // 添加 undo
                    transaction.addUndo(DmlType::DELETE, table_name, row_id);

                    // 记录日志
                    std::vector<std::pair<std::string, std::string>> old_values_for_log;
                    for (const auto& field : fields) {
                        old_values_for_log.emplace_back(field.name, record_data[field.name]);
                    }
                    LogManager::instance().logDelete(table_name, row_id, old_values_for_log);

                    // 更新索引
                    std::vector<std::string> deletedValues;
                    for (const auto& field : fields) {
                        deletedValues.push_back(record_data[field.name]);
                    }
                    updateIndexesAfterDelete(table_name, deletedValues, RecordPointer{ row_id });

                    deleted_count++;
                }
            }

            // 循环外统一 commit（自动提交事务）
            transaction.commitImplicitTransaction();
        }
        catch (const std::exception& e) {
            transaction.rollback();  // 事务失败时回滚
            file.close();
            infile.close();
            throw std::runtime_error("删除操作失败，已回滚: " + std::string(e.what()));
        }

        file.close();
        infile.close();
    }
    else {
        // 非事务分支 (直接物理删除)
        std::ifstream infile(trd_path, std::ios::binary);
        if (!infile) throw std::runtime_error("无法打开数据文件进行删除操作。");

        std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> records_to_keep;

        while (infile.peek() != EOF) {
            std::unordered_map<std::string, std::string> record_data;
            uint64_t row_id = 0;

            if (read_record_from_file(infile, fields, record_data, row_id, /*skip_deleted=*/true)) {
                if (condition.empty() || matches_condition(record_data, false)) {
                    if (!check_references_before_delete(table_name, record_data)) {
                        throw std::runtime_error("删除操作违反引用完整性约束");
                    }

                    std::vector<std::string> deletedValues;
                    for (const auto& field : fields) {
                        deletedValues.push_back(record_data[field.name]);
                    }
                    updateIndexesAfterDelete(table_name, deletedValues, RecordPointer{ row_id });

                    deleted_count++;
                    continue;
                }

                records_to_keep.emplace_back(row_id, record_data);
            }
        }
        infile.close();

        std::stable_sort(records_to_keep.begin(), records_to_keep.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

        std::ofstream outfile(trd_path, std::ios::binary | std::ios::trunc);
        if (!outfile) throw std::runtime_error("无法打开数据文件进行写入操作。");

        uint64_t new_row_id = 1;
        for (const auto& [old_row_id, record] : records_to_keep) {
            outfile.write(reinterpret_cast<const char*>(&new_row_id), sizeof(uint64_t));
            char delete_flag = 0;
            outfile.write(&delete_flag, sizeof(char));
            for (const auto& field : fields) {
                std::string field_name(field.name);
                write_field(outfile, field, record.at(field_name));
            }
            new_row_id++;
        }
        outfile.close();
        dbManager::getInstance().get_current_database()->getTable(tableName)->incrementRecordCount(-deleted_count);
        dbManager::getInstance().get_current_database()->getTable(tableName)->setLastModifyTime(std::time(nullptr));
    }

    return deleted_count;
}


int Record::delete_by_flag(const std::string& tableName) {
    this->table_name = tableName;
    if (!table_exists(this->table_name)) {
        throw std::runtime_error("表 '" + this->table_name + "' 不存在。");
    }

    std::vector<FieldBlock> fields = read_field_blocks(table_name);
    this->table_structure = read_table_structure_static(table_name);

    // 初始化 columns 用于索引字段定位
    columns.clear();
    for (const auto& field : fields) {
        columns.push_back(field.name);
    }

    std::string trd_filename = dbManager::getInstance().get_current_database()->getDBPath() + "/" + table_name + ".trd";
    std::ifstream infile(trd_filename, std::ios::binary);
    if (!infile) throw std::runtime_error("无法打开数据文件进行物理删除操作。");

    std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> valid_records;
    int deleted_count = 0;

    while (infile.peek() != EOF) {
        std::unordered_map<std::string, std::string> record_data;
        uint64_t row_id = 1;

        std::streampos pos = infile.tellg();

        infile.read(reinterpret_cast<char*>(&row_id), sizeof(uint64_t));
        if (!infile) break;

        char delete_flag;
        infile.read(&delete_flag, sizeof(char));
        if (!infile) break;

        if (delete_flag == 1) {
            deleted_count++;

            // 回到 pos 再读取整条记录用于索引更新
            infile.seekg(pos);
            std::unordered_map<std::string, std::string> deleted_record;
            infile.read(reinterpret_cast<char*>(&row_id), sizeof(uint64_t));
            infile.ignore(sizeof(char)); // skip delete flag
            if (read_record_from_file(infile, fields, deleted_record, row_id, false)) {
                std::vector<std::string> deletedValues;
                for (const auto& field : fields) {
                    deletedValues.push_back(deleted_record[field.name]);
                }
                updateIndexesAfterDelete(table_name, deletedValues, RecordPointer{ row_id });
            }

            // 跳过字段数据
            for (const auto& field : fields) {
                char null_flag;
                infile.read(&null_flag, sizeof(char));
                size_t bytes_read = sizeof(char);
                size_t field_size = get_field_data_size(field.type, field.param);
                infile.seekg(field_size, std::ios::cur);
                bytes_read += field_size;
                size_t padding = (4 - (bytes_read % 4)) % 4;
                if (padding > 0) infile.seekg(padding, std::ios::cur);
            }
        }
        else {
            infile.seekg(pos);
            if (read_record_from_file(infile, fields, record_data, row_id, false)) {
                valid_records.emplace_back(row_id, record_data);
            }
        }
    }

    infile.close();

    if (deleted_count == 0) return 0;

    std::stable_sort(valid_records.begin(), valid_records.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });

    std::ofstream outfile(trd_filename, std::ios::binary | std::ios::trunc);
    if (!outfile) throw std::runtime_error("无法打开数据文件进行写入操作。");

    uint64_t new_row_id = 1;
    for (const auto& [old_row_id, record] : valid_records) {
        outfile.write(reinterpret_cast<const char*>(&new_row_id), sizeof(uint64_t));
        char delete_flag = 0;
        outfile.write(&delete_flag, sizeof(char));
        for (const auto& field : fields) {
            write_field(outfile, field, record.at(field.name));
        }
        new_row_id++;
    }

    outfile.close();

    auto* tbl = dbManager::getInstance().get_current_database()->getTable(table_name);
    tbl->incrementRecordCount(-deleted_count);
    tbl->setLastModifyTime(std::time(nullptr));

    return deleted_count;
}

void Record::deleteByRowid(uint64_t rowId) {
    std::string file_path = dbManager::getInstance().get_current_database()->getDBPath() + "/" + this->table_name + ".trd";
    std::fstream file(file_path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file) throw std::runtime_error("无法打开文件进行删除");

    std::vector<FieldBlock> fields = read_field_blocks(this->table_name);
    size_t record_size = sizeof(uint64_t) + sizeof(char);
    for (const auto& f : fields) {
        record_size += get_field_size(f); // 你应该能获得字段字节数
    }

    std::streampos pos = (rowId - 1) * record_size + sizeof(uint64_t);  // skip row_id
    file.seekp(pos, std::ios::beg);

    char delete_flag = 1;
    file.write(&delete_flag, sizeof(char));
    file.close();
}

