#ifndef RECORD_H
#define RECORD_H

#include <string>
#include <vector>
#include <unordered_map>

class Record {
private:
    std::string table_name;
    std::vector<std::string> columns;
    std::vector<std::string> values;
    std::unordered_map<std::string, std::string> table_structure; // 列名 -> 数据类型

    // 解析列名和值
    void parse_columns(const std::string& cols);
    void parse_values(const std::string& vals);

    // 检查表是否存在
    bool table_exists(const std::string& table_name);

    // 从.tdf文件读取表结构
    void read_table_structure();

    // 验证列名和类型
    void validate_columns();
    void validate_types();
    void validate_types_without_columns();

    // 检查值的类型是否有效
    bool is_valid_type(const std::string& value, const std::string& type);

public:
    Record(const std::string& record);

    // 插入记录相关函数
    void insert_record(const std::string& table_name, const std::string& cols, const std::string& vals);
    void insert_into(const std::string& table_name);
};

#endif // RECORD_H