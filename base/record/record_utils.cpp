#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdexcept>
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
        case 2: column_type = "DOUBLE"; break;
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
std::string Record::get_type_string(int type) {
    switch (type) {
    case 1: return "INT";
    case 2: return "DOUBLE"; // 或 FLOAT
    case 3: return "VARCHAR";
    case 4: return "BOOL";
    case 5: return "DATETIME";
    default: return "UNKNOWN";
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
        else if (types[i] == "DOUBLE") {
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

    case 2: // DOUBLE
        try {
            std::stod(value);
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

/*void Record::validate_types_without_columns() {
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
}*/

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

void Record::parse_condition(const std::string& condition) {
    full_condition = condition;
}

// 实现条件匹配判断方法
bool Record::matches_condition(const std::unordered_map<std::string, std::string>& record_data, bool use_prefix) const {
    if (full_condition.empty()) return true;

    std::regex logic_split(R"(\s+(AND|OR)\s+)", std::regex::icase);
    std::sregex_token_iterator token_iter(full_condition.begin(), full_condition.end(), logic_split, { -1, 1 });
    std::sregex_token_iterator end;

    std::vector<std::string> expressions;
    std::vector<std::string> logic_ops;

    bool is_logic = false;
    for (; token_iter != end; ++token_iter) {
        std::string token = token_iter->str();
        if (is_logic) {
            std::transform(token.begin(), token.end(), token.begin(), ::toupper);
            logic_ops.push_back(token);
        }
        else {
            expressions.push_back(token);
        }
        is_logic = !is_logic;
    }

    auto resolve_field_type = [&](const std::string& key) -> std::string {
        auto it = table_structure.find(key);
        if (it != table_structure.end()) return it->second;

        if (!use_prefix) {
            std::string matched_type;
            int count = 0;
            for (const auto& [k, type] : table_structure) {
                size_t dot_pos = k.find('.');
                std::string suffix = (dot_pos != std::string::npos) ? k.substr(dot_pos + 1) : k;
                if (suffix == key) {
                    matched_type = type;
                    count++;
                }
            }
            if (count == 1) return matched_type;
        }
        throw std::runtime_error("字段 '" + key + "' 无法匹配到表结构中");
        };

    auto get_field_value = [&](const std::string& key) -> std::string {
        auto it = record_data.find(key);
        if (it != record_data.end()) return it->second;

        if (!use_prefix) {
            std::string result;
            int count = 0;
            for (const auto& [k, v] : record_data) {
                size_t dot_pos = k.find('.');
                std::string suffix = (dot_pos != std::string::npos) ? k.substr(dot_pos + 1) : k;
                if (suffix == key) {
                    result = v;
                    count++;
                }
            }
            if (count == 1) return result;
        }
        throw std::runtime_error("字段 '" + key + "' 无法匹配到记录中");
        };

    auto evaluate_single = [&](const std::string& cond) -> bool {
        std::regex expr_regex(R"(^\s*(\w+(\.\w+)?)\s*(=|!=|>=|<=|>|<)\s*(.+?)\s*$)");
        std::smatch m;
        if (!std::regex_match(cond, m, expr_regex)) return false;

        std::string field = m[1];
        std::string op = m[3];
        std::string value = m[4];

        std::string field_val = get_field_value(field);
        std::string field_type = resolve_field_type(field);

        if (field_type == "INTEGER" || field_type == "FLOAT" || field_type == "DOUBLE") {
            try {
                double a = std::stod(field_val);
                double b = std::stod(value);
                if (op == "=") return a == b;
                if (op == "!=") return a != b;
                if (op == ">") return a > b;
                if (op == "<") return a < b;
                if (op == ">=") return a >= b;
                if (op == "<=") return a <= b;
            }
            catch (...) { return false; }
        }
        else if (field_type == "BOOL") {
            return (op == "=") ? (field_val == value) : (op == "!=") ? (field_val != value) : false;
        }
        else {
            if (op == "=") return field_val == value;
            if (op == "!=") return field_val != value;
            if (op == ">") return field_val > value;
            if (op == "<") return field_val < value;
            if (op == ">=") return field_val >= value;
            if (op == "<=") return field_val <= value;
        }

        return false;
        };

    if (expressions.empty()) return true;

    bool result = evaluate_single(expressions[0]);
    for (size_t i = 0; i < logic_ops.size(); ++i) {
        bool next_result = evaluate_single(expressions[i + 1]);
        if (logic_ops[i] == "AND") result = result && next_result;
        else if (logic_ops[i] == "OR") result = result || next_result;
    }

    return result;
}




// 从.trd文件读取记录
std::vector<std::unordered_map<std::string, std::string>> Record::read_records(const std::string table_name) {
    std::vector<std::unordered_map<std::string, std::string>> records;
    std::string trd_filename = table_name + ".trd";
    std::ifstream file(trd_filename, std::ios::binary);

    if (!file) {
        return records; // 如果文件不存在，返回空记录集
    }

    std::vector<FieldBlock> fields = read_field_blocks(table_name);

    while (file.peek() != EOF) {
        std::unordered_map<std::string, std::string> record_data;

        for (const auto& field : fields) {
            char null_flag;
            file.read(&null_flag, sizeof(char));
            if (file.eof()) break;

            size_t bytes_read = sizeof(char);

            if (null_flag == 1) {
                record_data[field.name] = "NULL";

                switch (field.type) {
                case 1: bytes_read += sizeof(int); file.seekg(sizeof(int), std::ios::cur); break;
                case 2: bytes_read += sizeof(double); file.seekg(sizeof(double), std::ios::cur); break;
                case 3: bytes_read += field.param; file.seekg(field.param, std::ios::cur); break;
                case 4: bytes_read += sizeof(char); file.seekg(sizeof(char), std::ios::cur); break;
                case 5: bytes_read += sizeof(std::time_t); file.seekg(sizeof(std::time_t), std::ios::cur); break;
                default: break;
                }
            }
            else {
                switch (field.type) {
                case 1: { // INTEGER
                    int int_val;
                    file.read(reinterpret_cast<char*>(&int_val), sizeof(int));
                    record_data[field.name] = std::to_string(int_val);
                    bytes_read += sizeof(int);
                    break;
                }
                case 2: { // DOUBLE
                    double double_val;
                    file.read(reinterpret_cast<char*>(&double_val), sizeof(double));
                    record_data[field.name] = std::to_string(double_val);
                    bytes_read += sizeof(double);
                    break;
                }
                case 3: { // VARCHAR
                    std::vector<char> buffer(field.param);
                    file.read(buffer.data(), field.param);
                    record_data[field.name] = std::string(buffer.data(), field.param);
                    bytes_read += field.param;
                    break;
                }
                case 4: { // BOOL
                    char b;
                    file.read(&b, sizeof(char));
                    record_data[field.name] = (b == 1) ? "1" : "0";
                    bytes_read += sizeof(char);
                    break;
                }
                case 5: { // DATETIME
                    std::time_t time_val;
                    file.read(reinterpret_cast<char*>(&time_val), sizeof(std::time_t));
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

            size_t padding = (4 - (bytes_read % 4)) % 4;
            if (padding > 0) {
                file.seekg(padding, std::ios::cur);
            }
        }

        if (!record_data.empty()) {
            records.push_back(record_data);
        }
    }

    return records;
}

bool Record::read_single_record(std::ifstream& file, const std::vector<FieldBlock>& fields,
    std::unordered_map<std::string, std::string>& record_data) {
    record_data.clear();

    for (const auto& field : fields) {
        char null_flag;
        file.read(&null_flag, sizeof(char));
        if (!file) return false;  // 读失败或到达末尾

        size_t bytes_read = sizeof(char);

        if (null_flag == 1) {
            record_data[field.name] = "NULL";
            switch (field.type) {
            case 1: bytes_read += sizeof(int); break;
            case 2: bytes_read += sizeof(double); break;
            case 3: bytes_read += field.param; break;
            case 4: bytes_read += sizeof(char); break;
            case 5: bytes_read += sizeof(std::time_t); break;
            default: break;
            }
            file.seekg(bytes_read - sizeof(char), std::ios::cur);  // 跳过空值
        }
        else {
            switch (field.type) {
            case 1: {
                int val;
                file.read(reinterpret_cast<char*>(&val), sizeof(int));
                record_data[field.name] = std::to_string(val);
                bytes_read += sizeof(int);
                break;
            }
            case 2: {
                double val;
                file.read(reinterpret_cast<char*>(&val), sizeof(double));
                record_data[field.name] = std::to_string(val);
                bytes_read += sizeof(double);
                break;
            }
            case 3: {
                char* buf = new char[field.param];
                file.read(buf, field.param);
                record_data[field.name] = std::string(buf);
                delete[] buf;
                bytes_read += field.param;
                break;
            }
            case 4: {
                char b;
                file.read(&b, sizeof(char));
                record_data[field.name] = (b == 1 ? "1" : "0");
                bytes_read += sizeof(char);
                break;
            }
            case 5: {
                std::time_t t;
                file.read(reinterpret_cast<char*>(&t), sizeof(std::time_t));
                char buf[30];
                std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
                record_data[field.name] = "'" + std::string(buf) + "'";
                bytes_read += sizeof(std::time_t);
                break;
            }
            default:
                return false;
            }
        }

        // 填充跳过
        size_t padding = (4 - (bytes_read % 4)) % 4;
        if (padding) file.seekg(padding, std::ios::cur);
    }

    return true;
}

size_t Record::get_field_data_size(int type, int param) {
    switch (type) {
    case 1: return sizeof(int);             // INT
    case 2: return sizeof(double);           // DOUBLE
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
            double d = std::stod(value);
            out.write(reinterpret_cast<const char*>(&d), sizeof(double));
            break;
        }
        case 3: {
            // 写入原始字符串（包含引号）
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

    size_t data_size = get_field_data_size(field.type, field.param);
    std::string result;

    if (null_flag == 1) {
        in.seekg(data_size, std::ios::cur);
        result = "NULL";
    }
    else {
        switch (field.type) {
        case 1: {
            int v;
            in.read(reinterpret_cast<char*>(&v), sizeof(int));
            result = std::to_string(v);
            break;
        }
        case 2: {
            double d;
            in.read(reinterpret_cast<char*>(&d), sizeof(double));
            result = std::to_string(d);
            break;
        }
        case 3: {
            std::vector<char> buf(field.param, 0);
            in.read(buf.data(), field.param);
            result = std::string(buf.data());
            break;
        }
        case 4: {
            char b;
            in.read(&b, sizeof(char));
            result = (b == 1) ? "1" : "0";
            break;
        }
        case 5: {
            std::time_t t;
            in.read(reinterpret_cast<char*>(&t), sizeof(std::time_t));
            result = std::to_string(t);
            break;
        }
        default:
            result = "";
        }
    }
    size_t padding = (4 - (sizeof(char) + data_size) % 4) % 4;
    if (padding > 0) {
        in.seekg(padding, std::ios::cur);
    }

    return result;
}
