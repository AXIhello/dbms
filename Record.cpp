#include "Record.h"
#include <regex>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>

Record::Record(const std::string& record) {
    // 暂定为空
}

void Record::insert_record(const std::string& table_name,const std::string& cols,const std::string& vals) {
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
    else if (cols.empty()) {
        // 从.tdf文件中读取字段信息
        read_table_structure();

        // 解析值
        parse_values(vals);
        /*
        * 注释，如果值的数量小于字段数量 补空
        * 
        // 检查值的数量是否与表中字段数量匹配
        if (values.size() != table_structure.size()) {
            throw std::runtime_error("Value count (" + std::to_string(values.size()) +
                ") does not match column count (" +
                std::to_string(table_structure.size()) + ").");
        }
        */
        // 验证值的类型是否匹配
        validate_types_without_columns();
    }
    else {
        throw std::invalid_argument("Invalid SQL insert statement.");
    }
}

bool Record::table_exists(const std::string& table_name) {
    std::string tdf_filename = table_name + ".tdf";
    std::ifstream file(tdf_filename);
    return file.good();
}

void Record::read_table_structure() {
    std::string tdf_filename = table_name + ".tdf";
    std::ifstream file(tdf_filename);

    if (!file) {
        throw std::runtime_error("Failed to open table definition file: " + tdf_filename);
    }

    std::string line;
    table_structure.clear(); // 确保清空之前的结构信息

    // 假设.tdf文件格式为: column_name column_type
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string column_name, column_type;

        if (ss >> column_name >> column_type) {
            // 将列名和类型添加到表结构中
            table_structure[column_name] = column_type;
            // 同时保持原始的列顺序
            columns.push_back(column_name);
        }
    }
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

void Record::validate_columns() {
    // 首先从.tdf文件读取表结构
    if (table_structure.empty()) {
        read_table_structure();
    }

    // 验证所有提供的列名是否在表结构中
    for (const auto& column : columns) {
        if (table_structure.find(column) == table_structure.end()) {
            throw std::runtime_error("字段'" + column + "' 并不存在于表'" + table_name + "'中。");
        }
    }
}

bool Record::is_valid_type(const std::string& value, const std::string& type) {
    if (type == "INT") {
        // 检查是否为整数
        try {
            std::stoi(value);
            return true;
        }
        catch (...) {
            return false;
        }
    }
    else if (type == "FLOAT" || type == "DOUBLE") {
        // 检查是否为浮点数
        try {
            std::stod(value);
            return true;
        }
        catch (...) {
            return false;
        }
    }
    else if (type == "VARCHAR" || type == "CHAR" || type == "TEXT") {
        // 检查字符串是否被引号括起来
        return (value.front() == '\'' && value.back() == '\'') ||
            (value.front() == '\"' && value.back() == '\"');
    }
    else if (type == "DATE" || type == "DATETIME") {
        // 简单检查日期格式，待拓展
        std::regex date_regex(R"('(\d{4}-\d{2}-\d{2})')");
        return std::regex_match(value, date_regex);
    }

    return true;
}

void Record::validate_types() {
    // 验证每个值的类型是否与对应的列类型匹配
    for (size_t i = 0; i < columns.size(); ++i) {
        std::string column = columns[i];
        std::string value = values[i];
        std::string expected_type = table_structure[column];

        if (!is_valid_type(value, expected_type)) {
            throw std::runtime_error("Type mismatch for column '" + column +
                "'. Expected " + expected_type +
                ", got value: " + value);
        }
    }
}

void Record::validate_types_without_columns() {
    // 按表结构中的列顺序验证值的类型
    size_t i = 0;
    for (const auto& column : columns) {
        std::string expected_type = table_structure[column];
        std::string value = values[i];

        if (!is_valid_type(value, expected_type)) {
            throw std::runtime_error("Type mismatch for column '" + column +
                "'. Expected " + expected_type +
                ", got value: " + value);
        }
        ++i;
    }
}

void Record::insert_into(const std::string& db_name) {
    std::string file_name = this->table_name + ".trd";
    std::ofstream file(file_name, std::ios::app);

    if (!file) {
        throw std::runtime_error("打开文件" + file_name+"失败。");
    }

    // 写入记录到文件
    file << "INSERT INTO " << this->table_name << " (";
    for (size_t i = 0; i < columns.size(); ++i) {
        file << columns[i] << (i + 1 < columns.size() ? ", " : "");
    }
    file << ") VALUES (";
    for (size_t i = 0; i < values.size(); ++i) {
        file << values[i] << (i + 1 < values.size() ? ", " : "");
    }
    file << ");" << std::endl;

    file.close();
    std::cout << "记录插入表 " << this->table_name << " 成功。" << std::endl;
}