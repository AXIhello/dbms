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