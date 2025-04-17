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