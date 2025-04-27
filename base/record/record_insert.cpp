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

void Record::insert_record(const std::string& table_name, const std::string& cols, const std::string& vals) {
    this->table_name = table_name;
    if (!table_exists(this->table_name)) {
        throw std::runtime_error("表 '" + this->table_name + "' 不存在。");
    }
    if (!cols.empty()) {
        parse_columns(cols);
        parse_values(vals);
        validate_columns();
        validate_types();
    }
    else {
        // 没有指定列名，自动使用所有字段
            std::vector<FieldBlock> fields = read_field_blocks(table_name);
        columns.clear();
        for (const auto& field : fields) {
            columns.push_back(field.name);
        }

        parse_values(vals);

        if (columns.size() != values.size()) {
            throw std::runtime_error("插入值数量与表字段数量不匹配");
        }

        validate_types();
    }
    insert_into();
}

void Record::parse_columns(const std::string& cols) {
    columns.clear();
    std::stringstream ss(cols);
    std::string column;
    while (std::getline(ss, column, ',')) {
        column.erase(0, column.find_first_not_of(" \t"));
        column.erase(column.find_last_not_of(" \t") + 1);
        columns.push_back(column);
    }
}

void Record::parse_values(const std::string& vals) {
    values.clear();
    std::stringstream ss(vals);
    std::string value;
    bool in_quotes = false;
    std::string current_value;

    for (char c : vals) {
        if (c == '\'' || c == '"') {
            in_quotes = !in_quotes;
            current_value += c;
        }
        else if (c == ',' && !in_quotes) {
            current_value.erase(0, current_value.find_first_not_of(" \t"));
            current_value.erase(current_value.find_last_not_of(" \t") + 1);
            values.push_back(current_value);
            current_value.clear();
        }
        else {
            current_value += c;
        }
    }
    if (!current_value.empty()) {
        current_value.erase(0, current_value.find_first_not_of(" \t"));
        current_value.erase(current_value.find_last_not_of(" \t") + 1);
        values.push_back(current_value);
    }
}

void Record::insert_into() {
    std::vector<FieldBlock> fields = read_field_blocks(table_name);
    std::unordered_map<std::string, size_t> field_indices;
    for (size_t i = 0; i < fields.size(); ++i) {
        field_indices[fields[i].name] = i;
    }

    std::vector<std::string> record_values(fields.size(), "NULL");
    for (size_t i = 0; i < columns.size(); ++i) {
        size_t idx = field_indices[columns[i]];
        record_values[idx] = values[i];
    }

    // 使用完整字段名和值检查约束（在设置 record_values 后）
    std::vector<std::string> all_columns, all_values;
    for (const auto& field : fields) {
        all_columns.push_back(field.name);
    }
    all_values = record_values;

    std::vector<ConstraintBlock> constraints = read_constraints(table_name);
    if (!check_constraints(all_columns, all_values, constraints)) {
        throw std::runtime_error("插入数据违反表约束");
    }

    // 校验类型（已修复：跳过 NULL）
    for (size_t i = 0; i < fields.size(); ++i) {
        const FieldBlock& field = fields[i];
        const std::string& value = record_values[i];
        if (value == "NULL") continue;
        if (!is_valid_type(value, get_type_string(field.type))) {
            throw std::runtime_error("字段 '" + std::string(field.name) + "' 的值 '" + value + "' 不符合类型要求");
        }
    }

    // 写入数据
    std::string file_name = dbManager::getInstance().get_current_database()->getDBPath() + "/" + this->table_name + ".trd";
    std::ofstream file(file_name, std::ios::app | std::ios::binary);
    if (!file) {
        throw std::runtime_error("打开文件" + file_name + "失败。");
    }

    for (size_t i = 0; i < fields.size(); ++i) {
        write_field(file, fields[i], record_values[i]);
    }

    file.close();
    std::cout << "记录插入表 " << this->table_name << " 成功。" << std::endl;
}

