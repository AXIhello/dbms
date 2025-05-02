#ifndef RECORD_H
#define RECORD_H
#include <string>
#include <vector>
#include <unordered_map>
#include "base/block/fieldBlock.h"
#include "base/block/constraintBlock.h"
#include <filesystem> 
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <map>

struct JoinPair {
    std::string left_table;
    std::string right_table;
    std::vector<std::pair<std::string, std::string>> conditions; // 这一对表的连接条件
};

struct JoinInfo {
    std::vector<std::string> tables;
    std::vector<JoinPair> joins;       // 变成多组条件
};


class Record {
private:
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
    bool matches_condition(const std::unordered_map<std::string, std::string>& record_data, bool use_prefix = false) const;

    // 解析列名和值
    void parse_columns(const std::string& cols);
    void parse_values(const std::string& vals);
    // 验证列名和类型
    void validate_columns();
    void validate_types();
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

public:
    // 构造函数
    Record();
    void write_to_tdf_format(const std::string& table_name, const std::vector<std::string>& columns,
        const std::vector<std::string>& types, const std::vector<int>& params);
    bool validate_field_block(const std::string& value, const FieldBlock& field);
    // 表操作相关函数
    static std::vector<std::unordered_map<std::string, std::string>> read_records(const std::string table_name);
    void insert_record(const std::string& table_name, const std::string& cols, const std::string& vals);

    // 写入一个字段，包括 null_flag + 数据 + padding
    static void write_field(std::ofstream& out, const FieldBlock& field, const std::string& value);
    void insert_into();
    static std::vector<Record> select(
        const std::string& columns,
        const std::string& table_name,
        const std::string& condition,
        const std::string& group_by,
        const std::string& order_by,
        const std::string& having,
        const JoinInfo* join_info=nullptr);
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

	//索引相关函数

    //std::string normalize_condition_string(const std::string& input);
    // 支持等值或范围查找的索引方法
    static std::vector<std::unordered_map<std::string, std::string>>
        read_by_index(const std::string& table_name,
            const std::string& column,
            const std::string& op,
            const std::string& value);

    // 判断条件是否可以用于索引查找，并提取字段、操作符和值
    static bool can_use_index(const std::string& condition, std::string& field_out, std::string& value_out, std::string& op_out);

    // 检查未使用索引的表达式是否满足
    static bool evaluate_fallback_conditions(
        const std::unordered_map<std::string, std::string>& rec,
        bool use_prefix,
        const std::vector<size_t>& fallback_indices,
        const std::vector<std::string>& expressions,
        const std::vector<std::string>& logic_ops);


    // 尝试使用索引加速 select 查询
    static std::vector<std::unordered_map<std::string, std::string>> try_index_select(
        const std::string& table_name,
        const std::string& condition);
};
// 条件表达式判断工具
bool evaluate_single_expression(
    const std::unordered_map<std::string, std::string>& rec,
    const std::string& expression,
    bool use_prefix);

std::map<std::string, int> read_index_map(const std::string& filename);
bool file_exists(const std::string& filename);
std::unordered_map<std::string, std::string> read_record_by_index(const std::string& table_name, int offset);
std::vector<std::string> get_fields_from_line(const std::string& line);

std::tm custom_strptime(const std::string& datetime_str, const std::string& format);

#endif // RECORD_H