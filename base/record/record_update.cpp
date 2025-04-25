#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "Record.h"
#include "manager/parse.h"
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

    // 读取所有记录到内存中
    std::ifstream infile(table_name + ".trd", std::ios::binary);
    if (!infile) throw std::runtime_error("无法打开数据文件。");

    int updated = 0;
    std::unordered_map<std::string, std::string> record_data;
    std::vector<std::unordered_map<std::string, std::string>> all_records;

    while (read_single_record(infile, fields, record_data)) {
        std::unordered_map<std::string, std::string> new_data = record_data;
        if (condition.empty() || matches_condition(record_data, false)) {
            for (const auto& [col, val] : updates) {
                new_data[col] = val;
            }
            std::vector<std::string> cols, vals;
            for (const auto& field : fields) {
                std::string field_name(field.name);
                cols.push_back(field_name);
                vals.push_back(new_data[field_name]);
            }
            std::vector<ConstraintBlock> constraints = read_constraints(table_name);
            if (!check_constraints(cols, vals, constraints)) {
                throw std::runtime_error("更新数据违反表约束");
            }
            updated++;
        }
        all_records.push_back(new_data);
    }
    infile.close();

    // 清空原文件并重写
    std::ofstream outfile(table_name + ".trd", std::ios::binary | std::ios::trunc);
    if (!outfile) throw std::runtime_error("无法打开数据文件进行写入。");

    // 写入所有记录
    for (const auto& record : all_records) {
        for (const auto& field : fields) {
            std::string field_name(field.name);
            write_field(outfile, field, record.at(field_name));
        }
    }
    outfile.close();

    return updated;
}