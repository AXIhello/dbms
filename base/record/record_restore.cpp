#include"Record.h"

#include "parse/parse.h"
#include "ui/output.h"

#include <regex>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <algorithm>

int Record::rollback_delete_by_rowid(const std::string& tableName, uint64_t rowId) {
    this->table_name = tableName;
    if (!table_exists(this->table_name)) {
        throw std::runtime_error("表 '" + this->table_name + "' 不存在。");
    }

    std::vector<FieldBlock> fields = read_field_blocks(table_name);
    this->table_structure = read_table_structure_static(table_name);

    std::string trd_path = dbManager::getInstance().get_current_database()->getDBPath() + "/" + table_name + ".trd";
    std::fstream file(trd_path, std::ios::binary | std::ios::in | std::ios::out);
    if (!file) throw std::runtime_error("无法打开数据文件进行删除操作。");

    std::ifstream infile(trd_path, std::ios::binary);
    if (!infile) throw std::runtime_error("无法打开数据文件进行读取操作。");

    bool rollback_done = false;

    while (true) {
        std::streampos pos_before = infile.tellg();  // 记录当前记录起点位置
        if (pos_before == -1) break;  // EOF

        uint64_t current_row_id = 1;
        std::unordered_map<std::string, std::string> record_data;

        // 读取当前记录
        if (!read_record_from_file(infile, fields, record_data, current_row_id, /*skip_deleted=*/false)) {
            break;
        }

        // 如果当前记录rowID匹配
        if (current_row_id == rowId ) {
            // 精确定位到删除标记（row_id后面1字节）
            file.seekp(static_cast<std::streamoff>(pos_before) + sizeof(uint64_t));
            char delete_flag = 0;  // 恢复为未删除
            file.write(&delete_flag, sizeof(char));  // 写回删除标记

            rollback_done = true;
            break;
        }
    }

    file.close();
    infile.close();

    if (rollback_done) {
        // 更新记录数和最后修改时间
        dbManager::getInstance().get_current_database()->getTable(tableName)->setLastModifyTime(std::time(nullptr));
        return 1; // 回滚成功
    }

    return 0; // 没找到目标记录
}

int Record::rollback_insert_by_rowid(const std::string& tableName, uint64_t rowId) {
    this->table_name = tableName;
    if (!table_exists(this->table_name)) {
        throw std::runtime_error("表 '" + this->table_name + "' 不存在。");
    }

    std::vector<FieldBlock> fields = read_field_blocks(table_name);
    this->table_structure = read_table_structure_static(table_name);

    std::string trd_path = dbManager::getInstance().get_current_database()->getDBPath() + "/" + table_name + ".trd";
    std::fstream file(trd_path, std::ios::binary | std::ios::in | std::ios::out);
    if (!file) throw std::runtime_error("无法打开数据文件进行删除操作。");

    std::ifstream infile(trd_path, std::ios::binary);
    if (!infile) throw std::runtime_error("无法打开数据文件进行读取操作。");

    bool rollback_done = false;

    while (true) {
        std::streampos pos_before = infile.tellg();  // 记录当前记录起点位置
        if (pos_before == -1) break;  // EOF

        uint64_t current_row_id = 1;
        std::unordered_map<std::string, std::string> record_data;

        // 读取当前记录
        if (!read_record_from_file(infile, fields, record_data, current_row_id, /*skip_deleted=*/false)) {
            break;
        }

        // 如果当前记录rowID匹配
        if (current_row_id == rowId) {
            // 精确定位到删除标记（row_id后面1字节）
            file.seekp(static_cast<std::streamoff>(pos_before) + sizeof(uint64_t));
            char delete_flag = 1;  // 标记为待删除
            file.write(&delete_flag, sizeof(char));  // 写回删除标记

            rollback_done = true;
            break;
        }
    }

    file.close();
    infile.close();

    if (rollback_done) {
        // 更新记录数和最后修改时间
        dbManager::getInstance().get_current_database()->getTable(tableName)->setLastModifyTime(std::time(nullptr));
        return 1; // 回滚成功
    }

    return 0; // 没找到目标记录
}

int Record::rollback_update_by_rowid(const std::string& table_name, const std::vector<std::pair<uint64_t, std::vector<std::pair<std::string, std::string>>>>& undo_list) {
    int updatedCount = 0;
    // 读取现有数据
    auto records = read_records(table_name);

    // 把undo_list快速变成map便于查找
    std::unordered_map<uint64_t, std::unordered_map<std::string, std::string>> undo_map;
    for (const auto& [row_id, cols] : undo_list) {
        for (const auto& [col, val] : cols) {
            undo_map[row_id][col] = val;
        }
    }

    // 逐个覆盖原记录
    for (auto& [row_id, record_data] : records) {
        if (undo_map.count(row_id)) {
            for (const auto& [col, val] : undo_map[row_id]) {
                record_data[col] = val;
            }
            updatedCount++;
        }
    }

    // 覆盖写回.trd文件
    std::string trd_path = dbManager::getInstance().get_current_database()->getDBPath() + "/" + table_name + ".trd";
    std::ofstream outfile(trd_path, std::ios::binary | std::ios::trunc);
    if (!outfile) throw std::runtime_error("无法打开数据文件进行写入。");

    std::vector<FieldBlock> fields = read_field_blocks(table_name);

    for (const auto& [row_id, record] : records) {
        outfile.write(reinterpret_cast<const char*>(&row_id), sizeof(uint64_t));
        char delete_flag = 0;
        outfile.write(&delete_flag, sizeof(char));

        for (const auto& field : fields) {
            std::string field_name(field.name);
            write_field(outfile, field, record.at(field_name));
        }
    }
    outfile.close();
	return updatedCount;
}


