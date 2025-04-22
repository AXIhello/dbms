#ifndef RECORD_H
#define RECORD_H
#include <string>
#include <vector>
#include <unordered_map>
#include "base/block/fieldBlock.h"
#include "base/block/constraintBlock.h"
#include <filesystem> 

class Record {
private:
    static std::vector<std::unordered_map<std::string, std::string>> read_records(const std::string table_name);
    static bool read_single_record(std::ifstream& file, const std::vector<FieldBlock>& fields,
        std::unordered_map<std::string, std::string>& record_data);
    std::string table_name;
    std::vector<std::string> columns;
    std::vector<std::string> values;
    std::unordered_map<std::string, std::string> table_structure; // 列名 -> 数据类型
    static std::vector<FieldBlock> read_field_blocks(const std::string& table_name);
    // 条件解析相关
    std::string full_condition;
    void parse_condition(const std::string& condition);
    bool matches_condition(const std::unordered_map<std::string, std::string>& record_data) const;

    // 解析列名和值
    void parse_columns(const std::string& cols);
    void parse_values(const std::string& vals);
    // 验证列名和类型
    void validate_columns();
    void validate_types();
    void validate_types_without_columns();
    // 检查值的类型是否有效
    bool is_valid_type(const std::string& value, const std::string& type);

    // 约束检查相关
    static std::vector<ConstraintBlock> read_constraints(const std::string& table_name);
    bool check_constraints(const std::vector<std::string>& columns,
        std::vector<std::string>& values,
        const std::vector<ConstraintBlock>& constraints);
    bool check_primary_key_constraint(const ConstraintBlock& constraint,
        const std::string& value);
    bool check_foreign_key_constraint(const ConstraintBlock& constraint,
        const std::string& value);
    bool check_unique_constraint(const ConstraintBlock& constraint,
        const std::string& value);
    bool check_not_null_constraint(const ConstraintBlock& constraint,
        const std::string& value);
    bool check_check_constraint(const ConstraintBlock& constraint,
        const std::string& value);
    bool check_default_constraint(const ConstraintBlock& constraint,
        std::string& value);
    bool check_auto_increment_constraint(const ConstraintBlock& constraint,
        std::string& value);

    // 检查引用完整性
    bool check_references_before_delete(const std::string& table_name,
        const std::unordered_map<std::string, std::string>& record_data);
    // 计算数据本体的大小（不含null标志）
    static size_t get_field_data_size(int type, int param);
    // 写入一个字段，包括 null_flag + 数据 + padding
    static void write_field(std::ofstream& out, const FieldBlock& field, const std::string& value);
    // 读取一个字段，返回字符串值（带 null 判断）
    static std::string read_field(std::ifstream& in, const FieldBlock& field);

public:
    // 构造函数
    Record();
    void write_to_tdf_format(const std::string& table_name, const std::vector<std::string>& columns,
        const std::vector<std::string>& types, const std::vector<int>& params);
    bool validate_field_block(const std::string& value, const FieldBlock& field);
    // 表操作相关函数
    void insert_record(const std::string& table_name, const std::string& cols, const std::string& vals);
    void insert_into();
    static std::vector<Record> select(
        const std::string& columns,
        const std::string& table_name,
        const std::string& condition,
        const std::string& group_by,
        const std::string& order_by,
        const std::string& having);
    int update(const std::string& tableName, const std::string& setClause, const std::string& condition);
    int delete_(const std::string& tableName, const std::string& condition);
    // 辅助函数
    static bool table_exists(const std::string& table_name);
    static std::unordered_map<std::string, std::string> read_table_structure_static(const std::string& table_name);
    static std::vector<std::string> parse_column_list(const std::string& columns);
    static std::string get_type_string(int type);
    // Setter 和 Getter 方法
    void set_table_name(const std::string& name);
    void add_column(const std::string& column);
    void add_value(const std::string& value);
    std::string get_table_name() const;
    const std::vector<std::string>& get_columns() const;
    const std::vector<std::string>& get_values() const;
};
std::tm custom_strptime(const std::string& datetime_str, const std::string& format);

#endif // RECORD_H