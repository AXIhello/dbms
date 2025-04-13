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
        read_table_structure_static(table_name);

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

// insert_record 全局接口实现
void insert_record(const std::string& table_name, const std::string& cols, const std::string& vals) {
    Record r;
    r.insert_record(table_name, cols, vals);
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
        case 4: column_type = "CHAR"; break;
        case 5: column_type = "DATETIME"; break;
        default: column_type = "UNKNOWN";
        }

        result[column_name] = column_type;
    }

    return result;
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
        read_table_structure_static(table_name);
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
        else if (types[i] == "CHAR") {
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

        // 设置完整性约束（暂时不考虑）
        field.integrities = 0;

        // 写入FieldBlock结构
        file.write(reinterpret_cast<const char*>(&field), sizeof(FieldBlock));
    }

    file.close();
    std::cout << "成功创建表定义文件: " << tdf_filename << std::endl;
}
// 从.tdf文件读取FieldBlock结构
std::vector<FieldBlock> Record::read_field_blocks(const std::string& table_name) {
    std::vector<FieldBlock> fields;
    std::string tdf_filename = table_name;
    std::ifstream file(tdf_filename, std::ios::binary);

    if (!file) {
        throw std::runtime_error("无法打开表定义文件: " + tdf_filename);
    }

    // 读取文件头信息（如果有的话）

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
    case 4: // CHAR
        // 检查字符串长度是否符合参数要求
        if (value.length() <= static_cast<size_t>(field.param)) {
            return true;
        }
        return false;

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

void Record::update(const std::string& tableName, const std::string& setClause, const std::string& condition) {
    this->table_name = tableName;

    // 检查表是否存在
    if (!table_exists(this->table_name)) {
        throw std::runtime_error("表 '" + this->table_name + "' 不存在。");
    }

    // 读取表结构
    std::vector<FieldBlock> fields = read_field_blocks(table_name);

    // 解析SET子句
    std::unordered_map<std::string, std::string> update_values;
    std::istringstream set_stream(setClause);
    std::string pair;

    while (std::getline(set_stream, pair, ',')) {
        // 去除前后空格
        pair.erase(0, pair.find_first_not_of(" \t"));
        pair.erase(pair.find_last_not_of(" \t") + 1);

        // 查找等号位置
        size_t eq_pos = pair.find('=');
        if (eq_pos == std::string::npos) {
            throw std::runtime_error("无效的SET子句格式: " + pair);
        }

        // 提取字段名和新值
        std::string field = pair.substr(0, eq_pos);
        std::string value = pair.substr(eq_pos + 1);

        // 去除字段和值的前后空格
        field.erase(0, field.find_first_not_of(" \t"));
        field.erase(field.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        // 检查字段是否存在
        if (table_structure.find(field) == table_structure.end()) {
            throw std::runtime_error("字段 '" + field + "' 不存在于表 '" + this->table_name + "' 中。");
        }

        // 验证值的类型
        if (!is_valid_type(value, table_structure[field])) {
            throw std::runtime_error("值 '" + value + "' 的类型与字段 '" + field + "' 的类型不匹配。");
        }

        update_values[field] = value;
    }

    // 如果没有要更新的字段，则退出
    if (update_values.empty()) {
        throw std::runtime_error("没有提供要更新的字段。");
    }

    // 解析条件
    if (!condition.empty()) {
        parse_condition(condition);
    }

    // 执行更新操作
    // 1. 读取现有记录
     // 以二进制模式打开文件
    std::string trd_filename = this->table_name + ".trd";
    std::string temp_filename = this->table_name + ".tmp";
    std::ifstream infile(trd_filename, std::ios::binary);
    std::ofstream outfile(temp_filename, std::ios::binary);

    if (!infile || !outfile) {
        throw std::runtime_error("无法打开文件进行更新操作");
    }

    int updated_count = 0;

    // 根据字段定义计算记录大小
    size_t record_size = 0;
    for (const auto& field : fields) {
        size_t field_size = 0;
        switch (field.type) {
        case 1: field_size = sizeof(int); break;         // INTEGER
        case 2: field_size = sizeof(float); break;       // FLOAT
        case 3: field_size = field.param; break;         // VARCHAR
        case 4: field_size = field.param; break;         // CHAR
        case 5: field_size = sizeof(std::time_t); break; // DATETIME
        }

        // 添加填充以确保4字节对齐
        field_size = ((field_size + 3) / 4) * 4;
        record_size += field_size;
    }

    // 处理记录
    char* record_buffer = new char[record_size];
    while (infile.read(record_buffer, record_size)) {
        // 将记录解析为字段名到值的映射
        std::unordered_map<std::string, std::string> record_data;
        char* buffer_ptr = record_buffer;

        for (const auto& field : fields) {
            std::string value;

            // 根据字段类型提取值
            switch (field.type) {
            case 1: {  // INTEGER
                int int_val;
                std::memcpy(&int_val, buffer_ptr, sizeof(int));
                value = std::to_string(int_val);
                buffer_ptr += sizeof(int);
                break;
            }
            case 2: {  // FLOAT
                float float_val;
                std::memcpy(&float_val, buffer_ptr, sizeof(float));
                value = std::to_string(float_val);
                buffer_ptr += sizeof(float);
                break;
            }
            case 3:    // VARCHAR
            case 4: {  // CHAR
                value = std::string(buffer_ptr);
                buffer_ptr += field.param;
                break;
            }
            case 5: {  // DATETIME
                std::time_t time_val;
                std::memcpy(&time_val, buffer_ptr, sizeof(std::time_t));
                value = std::to_string(time_val);  // 如果需要，转换为适当的格式
                buffer_ptr += sizeof(std::time_t);
                break;
            }
            }

            size_t field_bytes = 0;
            switch (field.type) {
            case 1: field_bytes = sizeof(int); break;
            case 2: field_bytes = sizeof(float); break;
            case 3:
            case 4: field_bytes = field.param; break;
            case 5: field_bytes = sizeof(std::time_t); break;
            default: field_bytes = 0; break;
            }
            size_t padding = (4 - (field_bytes % 4)) % 4;
            buffer_ptr += padding;

            record_data[field.name] = value;
        }

        // 检查记录是否符合条件
        bool matches = condition.empty() || matches_condition(record_data);

        // 如果记录匹配，则更新
        if (matches) {
            // 应用 setClause 中的更新
            for (const auto& [field_name, new_value] : update_values) {
                record_data[field_name] = new_value;
            }
            updated_count++;
        }

        // 将记录（更新后或原始）写回文件
        buffer_ptr = record_buffer;
        for (const auto& field : fields) {
            std::string value = record_data[field.name];

            // 根据字段类型写入值
            switch (field.type) {
            case 1: {  // INTEGER
                int int_val = std::stoi(value);
                std::memcpy(buffer_ptr, &int_val, sizeof(int));
                buffer_ptr += sizeof(int);
                break;
            }
            case 2: {  // FLOAT
                float float_val = std::stof(value);
                std::memcpy(buffer_ptr, &float_val, sizeof(float));
                buffer_ptr += sizeof(float);
                break;
            }
            case 3:    // VARCHAR
            case 4: {  // CHAR
                // 如果存在引号，则删除
                if ((value.front() == '\'' && value.back() == '\'') ||
                    (value.front() == '\"' && value.back() == '\"')) {
                    value = value.substr(1, value.length() - 2);
                }

                // 确保以null结尾并且长度适当
                std::memset(buffer_ptr, 0, field.param);
                std::strncpy(buffer_ptr, value.c_str(), field.param - 1);
                buffer_ptr += field.param;
                break;
            }
            case 5: {  // DATETIME
                std::time_t time_val;
                // 从字符串转换或使用当前时间
                if (value.empty() || value == "NULL") {
                    time_val = std::time(nullptr);
                }
                else {
                    // 如果可能，从字符串解析时间
                    // 为简单起见，这里使用当前时间
                    time_val = std::time(nullptr);
                }
                std::memcpy(buffer_ptr, &time_val, sizeof(std::time_t));
                buffer_ptr += sizeof(std::time_t);
                break;
            }
            }

            size_t field_bytes = 0;
            switch (field.type) {
            case 1: field_bytes = sizeof(int); break;
            case 2: field_bytes = sizeof(float); break;
            case 3:
            case 4: field_bytes = field.param; break;
            case 5: field_bytes = sizeof(std::time_t); break;
            default: field_bytes = 0; break;
            }
            size_t padding = (4 - (field_bytes % 4)) % 4;
            buffer_ptr += padding;

        }

        outfile.write(record_buffer, record_size);
    }

    delete[] record_buffer;
    infile.close();
    outfile.close();

    // 用临时文件替换原文件
    if (std::remove(trd_filename.c_str()) != 0) {
        std::remove(temp_filename.c_str());
        throw std::runtime_error("无法删除原表数据文件。");
    }

    if (std::rename(temp_filename.c_str(), trd_filename.c_str()) != 0) {
        throw std::runtime_error("无法重命名临时文件。");
    }

    std::cout << "成功更新了 " << updated_count << " 条记录。" << std::endl;
}
void Record::delete_(const std::string& tableName, const std::string& condition) {
    this->table_name = tableName;

    // 检查表是否存在
    if (!table_exists(this->table_name)) {
        throw std::runtime_error("表 '" + this->table_name + "' 不存在。");
    }

    // 读取表结构
    std::vector<FieldBlock> fields = read_field_blocks(table_name);

    // 解析条件
    if (!condition.empty()) {
        try {
            parse_condition(condition);
        }
        catch (const std::exception& e) {
            throw std::runtime_error("条件解析错误: " + std::string(e.what()));
        }
    }

    // 执行删除操作
    std::string trd_filename = this->table_name + ".trd";
    std::string temp_filename = this->table_name + ".tmp";
    std::ifstream infile(trd_filename, std::ios::binary);
    std::ofstream outfile(temp_filename, std::ios::binary);

    if (!infile || !outfile) {
        throw std::runtime_error("无法打开文件进行删除操作");
    }

    int deleted_count = 0;

    // 计算记录大小
    size_t record_size = 0;
    for (const auto& field : fields) {
        size_t field_size = 0;
        switch (field.type) {
        case 1: field_size = sizeof(int); break;         // INTEGER
        case 2: field_size = sizeof(float); break;       // FLOAT
        case 3: field_size = field.param; break;         // VARCHAR
        case 4: field_size = field.param; break;         // CHAR
        case 5: field_size = sizeof(std::time_t); break; // DATETIME
        }

        // 添加填充以确保4字节对齐
        field_size = ((field_size + 3) / 4) * 4;
        record_size += field_size;
    }

    // 处理记录
    char* record_buffer = new char[record_size];
    while (infile.read(record_buffer, record_size)) {
        // 将记录解析为字段名到值的映射
        std::unordered_map<std::string, std::string> record_data;
        char* buffer_ptr = record_buffer;

        for (const auto& field : fields) {
            std::string value;

            // 根据字段类型提取值
            switch (field.type) {
            case 1: {  // INTEGER
                int int_val;
                std::memcpy(&int_val, buffer_ptr, sizeof(int));
                value = std::to_string(int_val);
                buffer_ptr += sizeof(int);
                break;
            }
            case 2: {  // FLOAT
                float float_val;
                std::memcpy(&float_val, buffer_ptr, sizeof(float));
                value = std::to_string(float_val);
                buffer_ptr += sizeof(float);
                break;
            }
            case 3:    // VARCHAR
            case 4: {  // CHAR
                value = std::string(buffer_ptr);
                buffer_ptr += field.param;
                break;
            }
            case 5: {  // DATETIME
                std::time_t time_val;
                std::memcpy(&time_val, buffer_ptr, sizeof(std::time_t));
                value = std::to_string(time_val);
                buffer_ptr += sizeof(std::time_t);
                break;
            }
            }

            size_t field_bytes = 0;
            switch (field.type) {
            case 1: field_bytes = sizeof(int); break;
            case 2: field_bytes = sizeof(float); break;
            case 3:
            case 4: field_bytes = field.param; break;
            case 5: field_bytes = sizeof(std::time_t); break;
            default: field_bytes = 0; break;
            }
            size_t padding = (4 - (field_bytes % 4)) % 4;
            buffer_ptr += padding;


            record_data[field.name] = value;
        }

        // 检查记录是否满足条件
        bool matches = condition.empty() || matches_condition(record_data);

        if (!matches) {
            // 不满足删除条件，保留记录
            outfile.write(record_buffer, record_size);
        }
        else {
            // 满足删除条件，不写入（即删除）
            deleted_count++;
        }
    }

    delete[] record_buffer;
    infile.close();
    outfile.close();

    // 替换原文件
    if (std::remove(trd_filename.c_str()) != 0) {
        std::remove(temp_filename.c_str());
        throw std::runtime_error("无法删除原表数据文件。");
    }

    if (std::rename(temp_filename.c_str(), trd_filename.c_str()) != 0) {
        throw std::runtime_error("无法重命名临时文件。");
    }

    std::cout << "成功删除了 " << deleted_count << " 条记录。" << std::endl;
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

    // 字符串比较（或数值转换失败的情况）
    if (field_type == "VARCHAR" || field_type == "CHAR" || field_type == "TEXT" ||
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

// 实现带条件的select方法
std::vector<Record> Record::select(const std::string& columns, const std::string& table_name, const std::string& condition) {
    std::vector<Record> results;

    // 检查表是否存在
    if (!table_exists(table_name)) {
        throw std::runtime_error("表 '" + table_name + "' 不存在。");
    }

    // 读取表结构以获取所有字段信息
    std::vector<FieldBlock> fields = read_field_blocks(table_name);
    std::unordered_map<std::string, std::string> table_structure;
    for (const auto& field : fields) {
        std::string column_type;
        switch (field.type) {
        case 1: column_type = "INTEGER"; break;
        case 2: column_type = "FLOAT"; break;
        case 3: column_type = "VARCHAR"; break;
        case 4: column_type = "CHAR"; break;
        case 5: column_type = "DATETIME"; break;
        default: column_type = "UNKNOWN";
        }
        table_structure[field.name] = column_type;
    }

    std::vector<std::string> all_columns;
    for (const auto& field : fields) {
        all_columns.push_back(field.name);
    }

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

    // 创建一个临时Record对象用于解析条件
    Record temp_record;
    temp_record.set_table_name(table_name);
    temp_record.table_structure = table_structure;

    // 解析条件
    if (!condition.empty()) {
        temp_record.parse_condition(condition);
    }

    // 读取记录文件
    std::string trd_filename = table_name + ".trd";
    std::ifstream file(trd_filename, std::ios::binary);

    if (!file) {
        // 文件不存在或不可读，返回空结果
        return results;
    }

    // 计算记录大小
    size_t record_size = 0;
    for (const auto& field : fields) {
        size_t field_size = 0;
        switch (field.type) {
        case 1: field_size = sizeof(int); break;         // INTEGER
        case 2: field_size = sizeof(float); break;       // FLOAT
        case 3: field_size = field.param; break;         // VARCHAR
        case 4: field_size = field.param; break;         // CHAR
        case 5: field_size = sizeof(std::time_t); break; // DATETIME
        }

        // 添加填充以确保4字节对齐
        field_size = ((field_size + 3) / 4) * 4;
        record_size += field_size;
    }

    // 处理记录
    char* record_buffer = new char[record_size];
    while (file.read(record_buffer, record_size)) {
        // 将记录解析为字段名到值的映射
        std::unordered_map<std::string, std::string> record_data;
        char* buffer_ptr = record_buffer;

        for (const auto& field : fields) {
            std::string value;

            // 根据字段类型提取值
            switch (field.type) {
            case 1: {  // INTEGER
                int int_val;
                std::memcpy(&int_val, buffer_ptr, sizeof(int));
                value = std::to_string(int_val);
                buffer_ptr += sizeof(int);
                break;
            }
            case 2: {  // FLOAT
                float float_val;
                std::memcpy(&float_val, buffer_ptr, sizeof(float));
                value = std::to_string(float_val);
                buffer_ptr += sizeof(float);
                break;
            }
            case 3:    // VARCHAR
            case 4: {  // CHAR
                value = std::string(buffer_ptr);
                buffer_ptr += field.param;
                break;
            }
            case 5: {  // DATETIME
                std::time_t time_val;
                std::memcpy(&time_val, buffer_ptr, sizeof(std::time_t));
                value = std::to_string(time_val);
                buffer_ptr += sizeof(std::time_t);
                break;
            }
            }

            size_t field_bytes = 0;
            switch (field.type) {
            case 1: field_bytes = sizeof(int); break;
            case 2: field_bytes = sizeof(float); break;
            case 3:
            case 4: field_bytes = field.param; break;
            case 5: field_bytes = sizeof(std::time_t); break;
            default: field_bytes = 0; break;
            }
            size_t padding = (4 - (field_bytes % 4)) % 4;
            buffer_ptr += padding;

            record_data[field.name] = value;
        }

        // 检查记录是否满足条件
        if (!condition.empty() && !temp_record.matches_condition(record_data)) {
            continue; // 不满足条件，跳过此记录
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

    delete[] record_buffer;
    return results;
}