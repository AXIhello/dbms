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
// 修改insert_into方法，使用符合.trd格式的方式写入数据
void Record::insert_into() {
    std::string file_name = this->table_name + ".trd";
    std::ofstream file(file_name, std::ios::app | std::ios::binary);

    if (!file) {
        throw std::runtime_error("打开文件" + file_name + "失败。");
    }

    // 读取字段结构
    std::vector<FieldBlock> fields = read_field_blocks(table_name);
    std::unordered_map<std::string, size_t> field_indices;

    // 构建字段名到索引的映射
    for (size_t i = 0; i < fields.size(); ++i) {
        field_indices[fields[i].name] = i;
    }

    // 准备一条完整记录的数据
    std::vector<std::string> record_values(fields.size());

    // 填充数据
    for (size_t i = 0; i < columns.size(); ++i) {
        if (field_indices.find(columns[i]) != field_indices.end()) {
            size_t idx = field_indices[columns[i]];
            record_values[idx] = values[i];
        }
    }

    // 按照.trd格式写入记录
    // 这里假设.trd文件格式为每个字段值直接连续存储，可能需要根据实际格式调整
    for (size_t i = 0; i < record_values.size(); ++i) {
        const FieldBlock& field = fields[i];
        const std::string& value = record_values[i];

        // 根据字段类型写入值
        switch (field.type) {
        case 1: { // INTEGER
            int int_val = std::stoi(value);
            file.write(reinterpret_cast<const char*>(&int_val), sizeof(int));
            break;
        }
        case 2: { // FLOAT
            float float_val = std::stof(value);
            file.write(reinterpret_cast<const char*>(&float_val), sizeof(float));
            break;
        }
        case 3: // VARCHAR
        case 4: { // CHAR
            // 写入固定长度的字符串
            char* buffer = new char[field.param];
            std::memset(buffer, 0, field.param);
            std::strncpy(buffer, value.c_str(), field.param - 1);
            file.write(buffer, field.param);
            delete[] buffer;
            break;
        }
        case 5: { // DATETIME
            // 简化处理，实际应转换为时间戳
            std::time_t now = std::time(nullptr);
            file.write(reinterpret_cast<const char*>(&now), sizeof(std::time_t));
            break;
        }
        default:
            throw std::runtime_error("未知的字段类型");
        }

        // 如果需要，可以添加填充以确保每个字段是4字节的倍数
        // 计算需要填充的字节数
        size_t bytes_written = 0;
        switch (field.type) {
        case 1: bytes_written = sizeof(int); break;
        case 2: bytes_written = sizeof(float); break;
        case 3:
        case 4: bytes_written = field.param; break;
        case 5: bytes_written = sizeof(std::time_t); break;
        default: bytes_written = 0;
        }

        // 计算需要填充的字节数，以使总字节数是4的倍数
        size_t padding = (4 - (bytes_written % 4)) % 4;
        if (padding > 0) {
            char* pad = new char[padding]();  // 初始化为0
            file.write(pad, padding);
            delete[] pad;
        }
    }

    file.close();
    std::cout << "记录插入表 " << this->table_name << " 成功。" << std::endl;
}