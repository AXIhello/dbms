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
    // 首先读取所有不需要删除的记录
    std::ifstream infile(dbManager::getInstance().get_current_database()->getDBPath() + "/" + table_name + ".trd", std::ios::binary);
    if (!infile) {
        throw std::runtime_error("无法打开数据文件进行删除操作。");
    }
    int deleted_count = 0;
    std::unordered_map<std::string, std::string> record_data;
    std::vector<std::unordered_map<std::string, std::string>> records_to_keep;
    while (read_single_record(infile, fields, record_data)) {
        if (condition.empty() || matches_condition(record_data, false)) {
            if (!check_references_before_delete(table_name, record_data)) {
                throw std::runtime_error("删除操作违反引用完整性约束");
            }
            deleted_count++;
            continue;
        }
        // 保存需要保留的记录
        records_to_keep.push_back(record_data);
    }
    infile.close();
    // 直接清空并重写文件
    std::ofstream outfile(dbManager::getInstance().get_current_database()->getDBPath() + "/" + table_name + ".trd", std::ios::binary | std::ios::trunc);
    if (!outfile) {
        throw std::runtime_error("无法打开数据文件进行写入操作。");
    }
    // 写入保留的记录
    for (const auto& record : records_to_keep) {
        for (const auto& field : fields) {
            // 将 char[128] 转换为 std::string 以用作键
            std::string field_name(field.name);
            write_field(outfile, field, record.at(field_name));
        }
    }
    outfile.close();
    return deleted_count;
}