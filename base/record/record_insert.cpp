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

void Record::insert_record(const std::string& table_name, const std::string& cols, const std::string& vals) {
    this->table_name = table_name;
    // 检查表是否存在
    if (!table_exists(this->table_name)) {
        throw std::runtime_error("表 '" + this->table_name + "' 不存在。");
    }
    if (!cols.empty()) {
        // 带列名的情况
        // 解析列名和值
        parse_columns(cols);
        parse_values(vals);

        // 验证列名是否存在于表中
        validate_columns();

        // 验证值的类型是否匹配
        validate_types();
    }
    else {
        throw std::invalid_argument("Invalid SQL insert statement.");
    }
    insert_into();
}
void Record::parse_columns(const std::string& cols) {
    columns.clear();
    std::stringstream ss(cols);
    std::string column;

    while (std::getline(ss, column, ',')) {
        // 去除前后空格
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
        if (c == '\'' || c == '\"') {
            in_quotes = !in_quotes;
            current_value += c;
        }
        else if (c == ',' && !in_quotes) {
            // 完成一个值的解析
            current_value.erase(0, current_value.find_first_not_of(" \t"));
            current_value.erase(current_value.find_last_not_of(" \t") + 1);
            values.push_back(current_value);
            current_value.clear();
        }
        else {
            current_value += c;
        }
    }

    // 添加最后一个值
    if (!current_value.empty()) {
        current_value.erase(0, current_value.find_first_not_of(" \t"));
        current_value.erase(current_value.find_last_not_of(" \t") + 1);
        values.push_back(current_value);
    }
}
void Record::insert_into() {
    std::vector<ConstraintBlock> constraints = read_constraints(table_name);
    if (!check_constraints(columns, values, constraints)) {
        throw std::runtime_error("插入数据违反表约束");
    }

    std::string file_name = this->table_name + ".trd";
    std::ofstream file(file_name, std::ios::app | std::ios::binary);
    if (!file) {
        throw std::runtime_error("打开文件" + file_name + "失败。");
    }

    std::vector<FieldBlock> fields = read_field_blocks(table_name);
    std::vector<bool> is_null(fields.size(), true);
    std::unordered_map<std::string, size_t> field_indices;
    for (size_t i = 0; i < fields.size(); ++i) {
        field_indices[fields[i].name] = i;
    }

    std::vector<std::string> record_values(fields.size());
    for (size_t i = 0; i < columns.size(); ++i) {
        if (field_indices.find(columns[i]) != field_indices.end()) {
            size_t idx = field_indices[columns[i]];
            record_values[idx] = values[i];
            is_null[idx] = false;
        }
    }

    for (size_t i = 0; i < record_values.size(); ++i) {
        const FieldBlock& field = fields[i];
        const std::string& value = record_values[i];
        char null_flag = is_null[i] ? 1 : 0;
        file.write(&null_flag, sizeof(char));
        size_t bytes_written = sizeof(char);

        if (!is_null[i]) {
            switch (field.type) {
            case 1: { // INTEGER
                int int_val = std::stoi(value);
                file.write(reinterpret_cast<const char*>(&int_val), sizeof(int));
                bytes_written += sizeof(int);
                break;
            }
            case 2: { // FLOAT
                float float_val = std::stof(value);
                file.write(reinterpret_cast<const char*>(&float_val), sizeof(float));
                bytes_written += sizeof(float);
                break;
            }
            case 3: // VARCHAR
            case 4: { // CHAR
                std::vector<char> buffer(field.param, 0);
                std::memcpy(buffer.data(), value.c_str(), std::min((size_t)field.param, value.size()));
                file.write(buffer.data(), field.param);
                bytes_written += field.param;
                break;
            }
            case 5: { // DATETIME
                std::tm tm = custom_strptime(value, "%Y-%m-%d %H:%M:%S");
                std::time_t t = std::mktime(&tm);
                file.write(reinterpret_cast<const char*>(&t), sizeof(std::time_t));
                bytes_written += sizeof(std::time_t);
                break;
            }
            default:
                throw std::runtime_error("未知的字段类型");
            }
        }
        else {
            switch (field.type) {
            case 1: bytes_written += sizeof(int); break;
            case 2: bytes_written += sizeof(float); break;
            case 3:
            case 4: bytes_written += field.param; break;
            case 5: bytes_written += sizeof(std::time_t); break;
            default: break;
            }
        }

        size_t padding = (4 - (bytes_written % 4)) % 4;
        if (padding > 0) {
            char pad[4] = { 0 };
            file.write(pad, padding);
        }
    }

    file.close();
    std::cout << "记录插入表 " << this->table_name << " 成功。" << std::endl;
}
