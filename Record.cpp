#include "Record.h"
#include "parse.h"
#include "output.h"

#include <regex>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <algorithm>

Record::Record() {
    // 暂定为空
}

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
    else if (cols.empty()) {
        // 从.tdf文件中读取字段信息
        read_table_structure();

        // 解析值
        parse_values(vals);

        // 检查值的数量是否与表中字段数量匹配
        if (values.size() != table_structure.size()) {
            throw std::runtime_error("Value count (" + std::to_string(values.size()) +
                ") does not match column count (" +
                std::to_string(table_structure.size()) + ").");
        }

        // 验证值的类型是否匹配
        validate_types_without_columns();
    }
    else {
        throw std::invalid_argument("Invalid SQL insert statement.");
    }
    insert_into();
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
    columns.clear(); // 清空列名

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
        if (i < values.size()) {  // 确保不会越界
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
}

void Record::insert_into() {
    std::string file_name = this->table_name + ".trd";
    std::ofstream file(file_name, std::ios::app);

    if (!file) {
        throw std::runtime_error("打开文件" + file_name + "失败。");
    }

    // 使用新的格式：字段.值,字段.值,...,END
    for (size_t i = 0; i < columns.size() && i < values.size(); ++i) {
        file << columns[i] << "." << values[i];
        if (i + 1 < columns.size() && i + 1 < values.size()) {
            file << ",";
        }
    }
    file << ",END" << std::endl;  // 使用END标记结束一条记录

    file.close();
    std::cout << "记录插入表 " << this->table_name << " 成功。" << std::endl;
}

std::vector<Record> Record::select(const std::string& columns, const std::string& table_name) {
    std::vector<Record> results;

    // 检查表是否存在
    if (!table_exists(table_name)) {
        throw std::runtime_error("表 '" + table_name + "' 不存在。");
    }

    // 读取表结构以获取所有字段信息
    std::unordered_map<std::string, std::string> table_structure = read_table_structure_static(table_name);
    std::vector<std::string> all_columns = get_column_order(table_name);

    // 确定要查询的列
    std::vector<std::string> selected_columns;
    if (columns == "*") {
        // 查询所有列
        selected_columns = all_columns;
    }
    else {
        // 解析指定的列
        selected_columns = parse_column_list(columns);

        // 验证指定的列是否存在于表中
        for (const auto& col : selected_columns) {
            if (table_structure.find(col) == table_structure.end()) {
                throw std::runtime_error("字段 '" + col + "' 不存在于表 '" + table_name + "' 中。");
            }
        }
    }

    // 读取记录文件
    std::string trd_filename = table_name + ".trd";
    std::ifstream file(trd_filename);

    if (!file) {
        // 文件不存在或不可读，返回空结果
        return results;
    }

    std::string line;
    // 读取每一行记录
    while (std::getline(file, line)) {
        // 解析记录行，格式：字段1.值1,字段2.值2,...,END
        std::unordered_map<std::string, std::string> record_data;
        std::string field_value_pair;
        std::stringstream ss(line);

        // 处理每个字段值对
        while (std::getline(ss, field_value_pair, ',')) {
            // 如果遇到END标记，结束当前记录的解析
            if (field_value_pair == "END") {
                break;
            }

            // 分割字段名和值
            size_t dot_pos = field_value_pair.find('.');
            if (dot_pos != std::string::npos) {
                std::string field = field_value_pair.substr(0, dot_pos);
                std::string value = field_value_pair.substr(dot_pos + 1);

                // 存储字段和值
                record_data[field] = value;
            }
        }

        // 创建一个新的记录对象
        Record record;
        record.set_table_name(table_name);

        // 添加选中的列和对应的值
        for (const auto& col : selected_columns) {
            record.add_column(col);
            // 如果记录中有该列，则添加值，否则添加NULL或空值
            if (record_data.find(col) != record_data.end()) {
                record.add_value(record_data[col]);
            }
            else {
                record.add_value("NULL");  // 或者其他表示缺失值的标记
            }
        }

        // 将记录添加到结果集
        results.push_back(record);
    }

    return results;
}

// 静态版本的表结构读取函数，用于select操作
std::unordered_map<std::string, std::string> Record::read_table_structure_static(const std::string& table_name) {
    std::unordered_map<std::string, std::string> result;
    std::string tdf_filename = table_name + ".tdf";
    std::ifstream file(tdf_filename);

    if (!file) {
        throw std::runtime_error("Failed to open table definition file: " + tdf_filename);
    }

    std::string line;
    // 假设.tdf文件格式为: column_name column_type
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string column_name, column_type;

        if (ss >> column_name >> column_type) {
            result[column_name] = column_type;
        }
    }

    return result;
}

// 获取列的顺序
std::vector<std::string> Record::get_column_order(const std::string& table_name) {
    std::vector<std::string> result;
    std::string tdf_filename = table_name + ".tdf";
    std::ifstream file(tdf_filename);

    if (!file) {
        throw std::runtime_error("Failed to open table definition file: " + tdf_filename);
    }

    std::string line;
    // 假设.tdf文件格式为: column_name column_type
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string column_name;

        if (ss >> column_name) {
            result.push_back(column_name);
        }
    }

    return result;
}

// 解析列名列表
std::vector<std::string> Record::parse_column_list(const std::string& columns) {
    std::vector<std::string> result;
    std::stringstream ss(columns);
    std::string column;

    while (std::getline(ss, column, ',')) {
        // 去除前后空格
        column.erase(0, column.find_first_not_of(" \t"));
        column.erase(column.find_last_not_of(" \t") + 1);
        result.push_back(column);
    }

    return result;
}

// 设置表名
void Record::set_table_name(const std::string& name) {
    this->table_name = name;
}

// 添加列
void Record::add_column(const std::string& column) {
    columns.push_back(column);
}

// 添加值
void Record::add_value(const std::string& value) {
    values.push_back(value);
}

// 获取表名
std::string Record::get_table_name() const {
    return table_name;
}

// 获取所有列
const std::vector<std::string>& Record::get_columns() const {
    return columns;
}

// 获取所有值
const std::vector<std::string>& Record::get_values() const {
    return values;
}

void Record::update(const std::string& tableName, const std::string& alterCommand) {
   
    
}
void Record::delete_(const std::string& tableName, const std::string& condition) {

}