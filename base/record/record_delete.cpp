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
    this->table_name = tableName;
    if (!table_exists(this->table_name)) {
        throw std::runtime_error("表 '" + this->table_name + "' 不存在。");
    }

    std::vector<FieldBlock> fields = read_field_blocks(table_name);
    this->table_structure = read_table_structure_static(table_name);
    if (!condition.empty()) parse_condition(condition);

    bool inTransaction = TransactionManager::instance().isActive();

    std::string trd_path = dbManager::getInstance().get_current_database()->getDBPath() + "/" + table_name + ".trd";

    int deleted_count = 0;

    if (inTransaction) {
        std::fstream file(trd_path, std::ios::binary | std::ios::in | std::ios::out);
        if (!file) throw std::runtime_error("无法打开数据文件进行删除操作。");

        std::ifstream infile(trd_path, std::ios::binary);
        if (!infile) throw std::runtime_error("无法打开数据文件进行读取操作。");

        while (true) {
            std::streampos pos_before = infile.tellg();  // 用 infile 的位置
            if (pos_before == -1) break;  // 已到 EOF

            uint64_t row_id = 0;
            std::unordered_map<std::string, std::string> record_data;

            // 通过 infile 读取数据
            if (!read_record_from_file(infile, fields, record_data, row_id, /*skip_deleted=*/false)) {
                break;
            }

            // 已逻辑删除的跳过
            if (record_data["_deleted_flag"] == "1") continue;

            // 如果符合删除条件
            if (condition.empty() || matches_condition(record_data, false)) {
                if (!check_references_before_delete(table_name, record_data)) {
                    throw std::runtime_error("删除操作违反引用完整性约束");
                }

                // 使用 infile.tellg() 的位置来定位
                file.seekp(static_cast<std::streamoff>(pos_before) + sizeof(uint64_t));  // 跳过 row_id
                char flag = 1;  // 设置删除标记为 1
                file.write(&flag, sizeof(char));  // 写入删除标记

                // 添加到 undo 区，记录删除操作
                TransactionManager::instance().addUndo(DmlType::DELETE, table_name, row_id);

                // 更新索引
                std::vector<std::string> deletedValues;
                for (const auto& field : fields) {
                    deletedValues.push_back(record_data[field.name]);
                }
                updateIndexesAfterDelete(table_name, deletedValues, RecordPointer{ row_id });

                deleted_count++;
            }
        }

        file.close();
        infile.close();
    }
    else {
        // 非事务：物理删除（你的原逻辑）
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

                    // 删除记录索引
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

        // 按 row_id 顺序写回
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

    // 更新记录数和修改时间
    

    return deleted_count;
}



//int Record::delete_by_rowid(const std::string& table_name, uint64_t target_row_id) {
//    this->table_name = table_name;
//    if (!table_exists(this->table_name)) {
//        throw std::runtime_error("表 '" + this->table_name + "' 不存在。");
//    }
//
//    // 读取字段结构
//    std::vector<FieldBlock> fields = read_field_blocks(table_name);
//    this->table_structure = read_table_structure_static(table_name);
//
//    // 读取所有记录
//    auto records = read_records(table_name);
//
//    // 将不需要删除的记录保留下来
//    std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> records_to_keep;
//    int deleted_count = 0;
//
//    for (const auto& [row_id, record_data] : records) {
//        if (row_id == target_row_id) {
//            // 找到匹配的 rowID 进行删除
//            deleted_count++;
//            continue;  // 跳过该记录，相当于删除
//        }
//
//        // 保留其余记录
//        records_to_keep.emplace_back(row_id, record_data);
//    }
//
//    // 排序保留下来的记录（保持原有顺序）
//    std::stable_sort(records_to_keep.begin(), records_to_keep.end(), [](const auto& a, const auto& b) {
//        return a.first < b.first;
//        });
//
//    // 打开文件进行写回
//    std::string trd_path = dbManager::getInstance().get_current_database()->getDBPath() + "/" + table_name + ".trd";
//    std::ofstream outfile(trd_path, std::ios::binary | std::ios::trunc);
//    if (!outfile) throw std::runtime_error("无法打开数据文件进行写入操作。");
//
//    uint64_t new_row_id = 1; // 从1开始写入新的 row_id
//    for (const auto& [old_row_id, record] : records_to_keep) {
//        // 写入新的 row_id 和记录
//        outfile.write(reinterpret_cast<const char*>(&new_row_id), sizeof(uint64_t));
//        char delete_flag = 0;  // 没有删除标志
//        outfile.write(&delete_flag, sizeof(char));
//
//        // 写入每个字段的数据
//        for (const auto& field : fields) {
//            std::string field_name(field.name);
//            write_field(outfile, field, record.at(field_name));
//        }
//
//        new_row_id++;  // 递增 row_id
//    }
//
//    // 更新记录数和最后修改时间
//    dbManager::getInstance().get_current_database()->getTable(table_name)->incrementRecordCount(-deleted_count);
//    dbManager::getInstance().get_current_database()->getTable(table_name)->setLastModifyTime(std::time(nullptr));
//
//    return deleted_count;
//}


int Record::delete_by_flag(const std::string& tableName) {
    this->table_name = tableName;
    if (!table_exists(this->table_name)) {
        throw std::runtime_error("表 '" + this->table_name + "' 不存在。");
    }

    std::vector<FieldBlock> fields = read_field_blocks(table_name);
    this->table_structure = read_table_structure_static(table_name);
    std::string trd_filename = dbManager::getInstance().get_current_database()->getDBPath() + "/" + table_name + ".trd";

    std::ifstream infile(trd_filename, std::ios::binary);
    if (!infile) throw std::runtime_error("无法打开数据文件进行物理删除操作。");

    // 读取所有未被标记删除的记录
    std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> valid_records;
    int deleted_count = 0;

    while (infile.peek() != EOF) {
        std::unordered_map<std::string, std::string> record_data;
        uint64_t row_id = 0;

        // 保存当前位置以便可以读取delete_flag
        std::streampos pos = infile.tellg();

        // 先读取row_id
        infile.read(reinterpret_cast<char*>(&row_id), sizeof(uint64_t));
        if (!infile) break;

        // 读取delete_flag
        char delete_flag;
        infile.read(&delete_flag, sizeof(char));
        if (!infile) break;

        // 如果记录已被标记为删除，跳过并计数
        if (delete_flag == 1) {
            deleted_count++;
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
            // 回到记录开始位置
            infile.seekg(pos);

            // 读取未删除的记录
            if (read_record_from_file(infile, fields, record_data, row_id, /*skip_deleted=*/false)) {
                valid_records.emplace_back(row_id, record_data);
            }
        }
    }

    infile.close();

    // 如果没有记录被删除，就不需要重写文件
    if (deleted_count == 0) {
        return 0;
    }

    // 按 row_id 顺序排序
    std::stable_sort(valid_records.begin(), valid_records.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });

    // 重写文件，为每条记录分配新的row_id
    std::ofstream outfile(trd_filename, std::ios::binary | std::ios::trunc);
    if (!outfile) throw std::runtime_error("无法打开数据文件进行写入操作。");

    uint64_t new_row_id = 1;
    for (const auto& [old_row_id, record] : valid_records) {
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

    // 更新数据库中表的记录计数
    // 注意：这里可能不需要减少记录数，因为标记删除时可能已经更新了记录计数
    // 如果标记删除时没有更新记录计数，则需要在此更新
    dbManager::getInstance().get_current_database()->getTable(table_name)->incrementRecordCount(-deleted_count);
    // 更新表的最后修改时间
    dbManager::getInstance().get_current_database()->getTable(tableName)->setLastModifyTime(std::time(nullptr));

    return deleted_count;
}