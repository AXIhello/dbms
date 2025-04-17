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