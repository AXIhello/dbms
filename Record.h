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

    // 验证列名和类型
    void validate_columns();
    void validate_types();
    void validate_types_without_columns();

    // 检查值的类型是否有效
    bool is_valid_type(const std::string& value, const std::string& type);

    // 读取表结构
    void read_table_structure();

public:
    // 构造函数
    Record();

    // 表操作相关函数
    void insert_record(const std::string& table_name, const std::string& cols, const std::string& vals);
    void insert_into();
    static std::vector<Record> select(const std::string& columns, const std::string& table_name);
    void update(const std::string& tableName, const std::string& alterCommand);
    void delete_(const std::string& tableName, const std::string& condition);

    // 辅助函数
    static bool table_exists(const std::string& table_name);
    static std::unordered_map<std::string, std::string> read_table_structure_static(const std::string& table_name);
    static std::vector<std::string> get_column_order(const std::string& table_name);
    static std::vector<std::string> parse_column_list(const std::string& columns);

    // Setter 和 Getter 方法
    void set_table_name(const std::string& name);
    void add_column(const std::string& column);
    void add_value(const std::string& value);

    std::string get_table_name() const;
    const std::vector<std::string>& get_columns() const;
    const std::vector<std::string>& get_values() const;
};

#endif // RECORD_H