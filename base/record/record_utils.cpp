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
#include<time.h>
#include <ctime>
#include <sstream>
#include <iomanip>

std::tm custom_strptime(const std::string& datetime_str, const std::string& format) {
    std::tm tm = {};
#ifdef _WIN32
    std::istringstream ss(datetime_str);
    ss >> std::get_time(&tm, format.c_str());
    if (ss.fail()) {
        throw std::runtime_error("时间字符串解析失败：" + datetime_str);
    }
#else
    strptime(datetime_str.c_str(), format.c_str(), &tm);
#endif
    return tm;
}

Record::Record() {
    // 暂定为空
}

bool Record::table_exists(const std::string& table_name) {
    std::string tdf_filename = table_name + ".tdf";
    std::ifstream file(tdf_filename,std::ios::binary);
    return file.good();
}

std::unordered_map<std::string, std::string> Record::read_table_structure_static(const std::string& table_name) {
    std::unordered_map<std::string, std::string> result;
    // 替换为类似 read_field_blocks 的二进制读取
    std::vector<FieldBlock> fields = read_field_blocks(table_name);

    for (const auto& field : fields) {
        std::string column_name = field.name;
        std::string column_type;

        // 将类型代码转换为字符串类型
        switch (field.type) {
        case 1: column_type = "INTEGER"; break;
        case 2: column_type = "FLOAT"; break;
        case 3: column_type = "VARCHAR"; break;
        case 4: column_type = "BOOL"; break;
        case 5: column_type = "DATETIME"; break;
        default: column_type = "UNKNOWN";
        }

        result[column_name] = column_type;
    }

    return result;
}



void Record::validate_columns() {
    // 首先从.tdf文件读取表结构
    if (table_structure.empty()) {
        table_structure=read_table_structure_static(table_name);
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
    else if (type == "VARCHAR"  || type == "TEXT") {
        // 检查字符串是否被引号括起来
        return (value.front() == '\'' && value.back() == '\'') ||
            (value.front() == '\"' && value.back() == '\"');
    }
    else if (type == "BOOL") {
        return value == "0" || value == "1";
    }
    else if (type == "DATE" || type == "DATETIME") {
        // 简单检查日期格式，待拓展
        std::regex date_regex(R"('(\d{4}-\d{2}-\d{2})')");
        return std::regex_match(value, date_regex);
    }

    return true;
}

// 修改validate_types方法使用FieldBlock进行验证
void Record::validate_types() {
    std::vector<FieldBlock> fields = read_field_blocks(table_name);
    std::unordered_map<std::string, FieldBlock> field_map;

    // 构建字段名到FieldBlock的映射
    for (const auto& field : fields) {
        field_map[field.name] = field;
    }

    // 验证每个值的类型是否与对应的字段类型匹配
    for (size_t i = 0; i < columns.size(); ++i) {
        std::string column = columns[i];
        std::string value = values[i];

        if (field_map.find(column) == field_map.end()) {
            throw std::runtime_error("字段 '" + column + "' 不存在于表中");
        }

        FieldBlock field = field_map[column];

        if (!validate_field_block(value, field)) {
            throw std::runtime_error("字段 '" + column + "' 的值类型不匹配");
        }
    }
}

// 创建表结构的.tdf文件
void Record::write_to_tdf_format(const std::string& table_name,
    const std::vector<std::string>& columns,
    const std::vector<std::string>& types,
    const std::vector<int>& params) {

    if (columns.size() != types.size() || columns.size() != params.size()) {
        throw std::runtime_error("字段名、类型和参数数量不匹配");
    }

    std::string tdf_filename = table_name + ".tdf";
    std::ofstream file(tdf_filename, std::ios::binary);

    if (!file) {
        throw std::runtime_error("无法创建表定义文件: " + tdf_filename);
    }

    // 写入文件头信息（如果需要的话）

    // 写入每个字段的FieldBlock结构
    for (size_t i = 0; i < columns.size(); ++i) {
        FieldBlock field;
        field.order = static_cast<int>(i + 1);  // 字段顺序从1开始

        // 复制字段名，确保不超过128个字符
        std::strncpy(field.name, columns[i].c_str(), 127);
        field.name[127] = '\0';  // 确保字符串结束

        // 设置字段类型
        if (types[i] == "INTEGER") {
            field.type = 1;
        }
        else if (types[i] == "FLOAT") {
            field.type = 2;
        }
        else if (types[i] == "VARCHAR") {
            field.type = 3;
        }
        else if (types[i] == "BOOL") {
            field.type = 4;
        }
        else if (types[i] == "DATETIME") {
            field.type = 5;
        }
        else {
            field.type = 0;  // 未知类型
        }

        // 设置参数（如VARCHAR的长度）
        field.param = params[i];

        // 设置最后修改时间为当前时间
        field.mtime = std::time(nullptr);

        // 写入FieldBlock结构
        file.write(reinterpret_cast<const char*>(&field), sizeof(FieldBlock));
    }

    file.close();
    std::cout << "成功创建表定义文件: " << tdf_filename << std::endl;
}
// 从.tdf文件读取FieldBlock结构
std::vector<FieldBlock> Record::read_field_blocks(const std::string& table_name) {
    std::vector<FieldBlock> fields;
    std::string tdf_filename = table_name + ".tdf";
    std::ifstream file(tdf_filename, std::ios::binary);

    if (!file) {
        throw std::runtime_error("无法打开表定义文件: " + tdf_filename);
    }

    // 开始读取FieldBlock结构
    FieldBlock field;
    while (file.read(reinterpret_cast<char*>(&field), sizeof(FieldBlock))) {
        fields.push_back(field);
    }
    return fields;
}

// 根据FieldBlock验证值类型
bool Record::validate_field_block(const std::string& value, const FieldBlock& field) {
    // 在函数开始处定义正则表达式，而不是在case中
    std::regex date_regex(R"('(\d{4}-\d{2}-\d{2})')");

    switch (field.type) {
    case 1: // INTEGER
        try {
            std::stoi(value);
            return true;
        }
        catch (...) {
            return false;
        }

    case 2: // FLOAT
        try {
            std::stof(value);
            return true;
        }
        catch (...) {
            return false;
        }

    case 3: // VARCHAR
        return value.length() <= static_cast<size_t>(field.param);
    case 4: // BOOL
        return value == "0" || value == "1";

    case 5: // DATETIME
        // 使用前面定义的正则表达式
        return std::regex_match(value, date_regex);

    default:
        return false;
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

// 实现条件解析方法
void Record::parse_condition(const std::string& condition) {
    // 清除之前的条件数据
    condition_field = "";
    condition_operator = "";
    condition_value = "";

    // 如果条件为空则跳过
    if (condition.empty()) {
        return;
    }

    // 用于匹配不同比较运算符的正则表达式
    std::regex eq_regex("\\s*(\\w+)\\s*=\\s*([^\\s]+)\\s*");   // 字段 = 值
    std::regex neq_regex("\\s*(\\w+)\\s*!=\\s*([^\\s]+)\\s*"); // 字段 != 值
    std::regex gt_regex("\\s*(\\w+)\\s*>\\s*([^\\s]+)\\s*");   // 字段 > 值
    std::regex lt_regex("\\s*(\\w+)\\s*<\\s*([^\\s]+)\\s*");   // 字段 < 值
    std::regex gte_regex("\\s*(\\w+)\\s*>=\\s*([^\\s]+)\\s*"); // 字段 >= 值
    std::regex lte_regex("\\s*(\\w+)\\s*<=\\s*([^\\s]+)\\s*"); // 字段 <= 值

    std::smatch matches;

    // 尝试匹配每种操作符模式
    if (std::regex_match(condition, matches, eq_regex)) {
        condition_field = matches[1];
        condition_operator = "=";
        condition_value = matches[2];
    }
    else if (std::regex_match(condition, matches, neq_regex)) {
        condition_field = matches[1];
        condition_operator = "!=";
        condition_value = matches[2];
    }
    else if (std::regex_match(condition, matches, gt_regex)) {
        condition_field = matches[1];
        condition_operator = ">";
        condition_value = matches[2];
    }
    else if (std::regex_match(condition, matches, lt_regex)) {
        condition_field = matches[1];
        condition_operator = "<";
        condition_value = matches[2];
    }
    else if (std::regex_match(condition, matches, gte_regex)) {
        condition_field = matches[1];
        condition_operator = ">=";
        condition_value = matches[2];
    }
    else if (std::regex_match(condition, matches, lte_regex)) {
        condition_field = matches[1];
        condition_operator = "<=";
        condition_value = matches[2];
    }
    else {
        throw std::runtime_error("无效的条件格式：" + condition);
    }

    // 验证字段是否存在于表中
    if (!table_structure.empty() && table_structure.find(condition_field) == table_structure.end()) {
        throw std::runtime_error("字段 '" + condition_field + "' 不存在于表 '" + table_name + "' 中。");
    }
}

// 实现条件匹配判断方法
bool Record::matches_condition(const std::unordered_map<std::string, std::string>& record_data) const {
    // 如果没有条件，则所有记录都匹配
    if (condition_field.empty() || condition_operator.empty()) {
        return true;
    }

    // 检查记录中是否包含条件字段
    if (record_data.find(condition_field) == record_data.end()) {
        return false; // 字段不存在，不匹配
    }

    // 获取记录中对应字段的值
    std::string field_value = record_data.at(condition_field);

    // 根据字段类型进行比较
    std::string field_type = table_structure.at(condition_field);

    // 数值型比较
    if (field_type == "INT" || field_type == "FLOAT" || field_type == "DOUBLE") {
        try {
            // 将字符串转换为数值进行比较
            double record_val = std::stod(field_value);
            double condition_val = std::stod(condition_value);

            if (condition_operator == "=") return record_val == condition_val;
            if (condition_operator == "!=") return record_val != condition_val;
            if (condition_operator == ">") return record_val > condition_val;
            if (condition_operator == "<") return record_val < condition_val;
            if (condition_operator == ">=") return record_val >= condition_val;
            if (condition_operator == "<=") return record_val <= condition_val;
        }
        catch (const std::exception& e) {
            // 转换失败，按字符串比较
            std::cerr << "数值转换失败，按字符串比较: " << e.what() << std::endl;
        }
    }
    // 布尔型比较（0或1作为字符串）
    if (field_type == "BOOL") {
        if (condition_value != "0" && condition_value != "1") {
            throw std::runtime_error("布尔类型条件值必须是 '0' 或 '1'");
        }

        if (condition_operator == "=") return field_value == condition_value;
        if (condition_operator == "!=") return field_value != condition_value;
        throw std::runtime_error("布尔类型只支持 '=' 和 '!=' 比较");
    }

    // 字符串比较（或数值转换失败的情况）
    if (field_type == "VARCHAR" ||  field_type == "TEXT" ||
        condition_operator == "=" || condition_operator == "!=") {

        // 处理引号
        std::string clean_field_value = field_value;
        std::string clean_condition_value = condition_value;

        // 去除可能存在的引号
        auto remove_quotes = [](std::string& s) {
            if ((s.front() == '\'' && s.back() == '\'') ||
                (s.front() == '\"' && s.back() == '\"')) {
                s = s.substr(1, s.length() - 2);
            }
            };

        remove_quotes(clean_field_value);
        remove_quotes(clean_condition_value);

        if (condition_operator == "=") return clean_field_value == clean_condition_value;
        if (condition_operator == "!=") return clean_field_value != clean_condition_value;
        if (condition_operator == ">") return clean_field_value > clean_condition_value;
        if (condition_operator == "<") return clean_field_value < clean_condition_value;
        if (condition_operator == ">=") return clean_field_value >= clean_condition_value;
        if (condition_operator == "<=") return clean_field_value <= clean_condition_value;
    }

    // 日期比较 (简单实现，可能需要更复杂的日期解析)
    if (field_type == "DATE" || field_type == "DATETIME") {
        // 去除引号后直接比较字符串（因为ISO格式日期字符串可以直接比较）
        std::string clean_field_value = field_value;
        std::string clean_condition_value = condition_value;

        auto remove_quotes = [](std::string& s) {
            if ((s.front() == '\'' && s.back() == '\'') ||
                (s.front() == '\"' && s.back() == '\"')) {
                s = s.substr(1, s.length() - 2);
            }
            };

        remove_quotes(clean_field_value);
        remove_quotes(clean_condition_value);

        if (condition_operator == "=") return clean_field_value == clean_condition_value;
        if (condition_operator == "!=") return clean_field_value != clean_condition_value;
        if (condition_operator == ">") return clean_field_value > clean_condition_value;
        if (condition_operator == "<") return clean_field_value < clean_condition_value;
        if (condition_operator == ">=") return clean_field_value >= clean_condition_value;
        if (condition_operator == "<=") return clean_field_value <= clean_condition_value;
    }

    // 默认返回false
    return false;
}

// 从.trd文件读取记录
std::vector<std::unordered_map<std::string, std::string>> Record::read_records(const std:: string table_name) {
    std::vector<std::unordered_map<std::string, std::string>> records;
    std::string trd_filename = table_name + ".trd";
    std::ifstream file(trd_filename, std::ios::binary);

    if (!file) {
        return records; // 如果文件不存在，返回空记录集
    }

    // 读取表结构以获取字段信息
    std::vector<FieldBlock> fields = read_field_blocks(table_name);

    // 读取文件中的每条记录
    while (file.peek() != EOF) {
        std::unordered_map<std::string, std::string> record_data;

        for (const auto& field : fields) {
            // 读取NULL标志
            char null_flag;
            file.read(&null_flag, sizeof(char));
            size_t bytes_read = sizeof(char);

            if (null_flag == 1) {
                // 是NULL值
                record_data[field.name] = "NULL";

                // 计算若有值时应读取的字节数
                switch (field.type) {
                case 1: bytes_read += sizeof(int); break;
                case 2: bytes_read += sizeof(float); break;
                case 3: bytes_read += field.param; break;
                case 4: bytes_read += sizeof(char); break;
                case 5: bytes_read += sizeof(std::time_t); break;
                default: break;
                }
            }
            else {
                // 非NULL值，根据类型读取
                switch (field.type) {
                case 1: { // INTEGER
                    int int_val;
                    file.read(reinterpret_cast<char*>(&int_val), sizeof(int));
                    record_data[field.name] = std::to_string(int_val);
                    bytes_read += sizeof(int);
                    break;
                }
                case 2: { // FLOAT
                    float float_val;
                    file.read(reinterpret_cast<char*>(&float_val), sizeof(float));
                    record_data[field.name] = std::to_string(float_val);
                    bytes_read += sizeof(float);
                    break;
                }
                case 3: {
                    char* buffer = new char[field.param];
                    file.read(buffer, field.param);
                    record_data[field.name] = "'" + std::string(buffer) + "'";
                    delete[] buffer;
                    break;
                }
                case 4: { // BOOL
                    char b;
                    file.read(&b, sizeof(char));
                    record_data[field.name] = (b == 1) ? "1" : "0";
                    break;
                }
                case 5: { // DATETIME
                    std::time_t time_val;
                    file.read(reinterpret_cast<char*>(&time_val), sizeof(std::time_t));
                    // 将时间戳转换为字符串
                    char time_str[30];
                    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", std::localtime(&time_val));
                    record_data[field.name] = "'" + std::string(time_str) + "'";
                    bytes_read += sizeof(std::time_t);
                    break;
                }
                default:
                    throw std::runtime_error("未知的字段类型");
                }
            }

            // 跳过填充字节
            size_t padding = (4 - (bytes_read % 4)) % 4;
            if (padding > 0) {
                file.seekg(padding, std::ios::cur);
            }
        }

        records.push_back(record_data);
    }

    return records;
}

size_t Record::get_field_data_size(int type, int param) {
    switch (type) {
    case 1: return sizeof(int);             // INT
    case 2: return sizeof(float);           // FLOAT
    case 3: return param;                   // VARCHAR
    case 4: return sizeof(char);            // BOOL
    case 5: return sizeof(std::time_t);     // DATETIME
    default: return 0;
    }
}

void Record::write_field(std::ofstream& out, const FieldBlock& field, const std::string& value) {
    bool is_null = (value == "NULL");
    char null_flag = is_null ? 1 : 0;
    out.write(&null_flag, sizeof(char));

    if (is_null) {
        size_t data_size = get_field_data_size(field.type, field.param);
        std::vector<char> dummy(data_size, 0);
        out.write(dummy.data(), data_size);
    }
    else {
        switch (field.type) {
        case 1: {
            int v = std::stoi(value);
            out.write(reinterpret_cast<const char*>(&v), sizeof(int));
            break;
        }
        case 2: {
            float f = std::stof(value);
            out.write(reinterpret_cast<const char*>(&f), sizeof(float));
            break;
        }
        case 3: {
            std::vector<char> buf(field.param, 0);
            std::memcpy(buf.data(), value.c_str(), std::min((size_t)field.param, value.size()));
            out.write(buf.data(), field.param);
            break;
        }
        case 4: {
            char b = (value == "1") ? 1 : 0;
            out.write(&b, sizeof(char));
            break;
        }
        case 5: {
            std::tm tm = custom_strptime(value, "%Y-%m-%d %H:%M:%S");
            std::time_t t = std::mktime(&tm);
            out.write(reinterpret_cast<const char*>(&t), sizeof(std::time_t));
            break;
        }
        }
    }

    size_t data_size = get_field_data_size(field.type, field.param);
    size_t padding = (4 - (sizeof(char) + data_size) % 4) % 4;
    char pad[4] = { 0 };
    out.write(pad, padding);
}

std::string Record::read_field(std::ifstream& in, const FieldBlock& field) {
    char null_flag;
    in.read(&null_flag, sizeof(char));
    if (in.eof()) return "";

    if (null_flag == 1) {
        in.seekg(get_field_data_size(field.type, field.param), std::ios::cur);
        return "NULL";
    }

    switch (field.type) {
    case 1: {
        int v;
        in.read(reinterpret_cast<char*>(&v), sizeof(int));
        return std::to_string(v);
    }
    case 2: {
        float f;
        in.read(reinterpret_cast<char*>(&f), sizeof(float));
        return std::to_string(f);
    }
    case 3: {
        std::vector<char> buf(field.param, 0);
        in.read(buf.data(), field.param);
        return std::string(buf.data());
    }
    case 4: {
        char b;
        in.read(&b, sizeof(char));
        return (b == 1) ? "1" : "0";
    }
    case 5: {
        std::time_t t;
        in.read(reinterpret_cast<char*>(&t), sizeof(std::time_t));
        return std::to_string(t);
    }
    }

    return "";
}