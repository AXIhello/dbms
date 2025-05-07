#ifndef RECORD_H
#define RECORD_H
#include <string>
#include <vector>
#include <unordered_map>
#include"base/BTree.h"
#include "base/block/fieldBlock.h"
#include "base/block/constraintBlock.h"
#include"transaction/TransactionManager.h"

#include "base/block/tableBlock.h"
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
    static bool read_record_from_file(std::ifstream& file, const std::vector<FieldBlock>& fields,
        std::unordered_map<std::string, std::string>& record_data, uint64_t& row_id, bool skip_deleted);
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
    static std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>>read_records(const std::string& table_name);
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

    // 运算符类型枚举
    enum class OperatorType {
        EQUALS,           // =
        NOT_EQUALS,       // !=
        GREATER_THAN,     // >
        LESS_THAN,        // <
        GREATER_EQUAL,    // >=
        LESS_EQUAL,       // <=
        LIKE,             // LIKE
        NOT_LIKE,         // NOT LIKE
        IN,               // IN
        NOT_IN,           // NOT IN
        BETWEEN,          // BETWEEN
        IS_NULL,          // IS NULL
        IS_NOT_NULL,      // IS NOT NULL
        UNKNOWN           // 其他
    };

    // 索引条件结构
    struct IndexCondition {
        std::string field_name;      // 字段名
        OperatorType op_type;        // 操作符类型
        std::string value;           // 操作值(用于=, >, <, >=, <=, LIKE等)
        std::string low_value;       // 范围下限(用于BETWEEN)
        std::string high_value;      // 范围上限(用于BETWEEN)
        std::vector<std::string> in_values; // IN列表值
    };

private:
    // 索引相关辅助方法
    static void parse_conditions_by_table(
        const std::string& condition,
        const std::vector<std::string>& tables,
        std::unordered_map<std::string, std::string>& table_conditions,
        std::string& join_condition);

    static void extract_index_conditions(
        const std::string& table_name,
        const std::string& condition,
        std::vector<IndexCondition>& index_conditions);

    static std::pair<uint64_t, std::unordered_map<std::string, std::string>>
        fetch_record_by_pointer(const std::string& table_name, uint64_t row_id);

    BTree* get_index_for_field(const std::string& table_name, const std::string& field_name);

    bool get_btree_for_field(const std::string& table_name, const std::string& field_name,
        BTree*& btree, const IndexBlock*& index_block);

    // 条件解析辅助方法
    static void split_condition_by_and(const std::string& condition, std::vector<std::string>& parts);

    static std::string extract_field_name_from_condition(const std::string& condition);

    static bool table_has_field(const std::string& table_name, const std::string& field_name);

    static void remove_table_prefix(std::string& field_name);

    static std::string trim(const std::string& str);

    // 条件匹配方法
    static bool match_equals_condition(const std::string& condition, std::string& field_name, std::string& value);

    static bool match_greater_than_condition(const std::string& condition, std::string& field_name, std::string& value);

    static bool match_greater_equal_condition(const std::string& condition, std::string& field_name, std::string& value);

    static bool match_less_than_condition(const std::string& condition, std::string& field_name, std::string& value);

    static bool match_less_equal_condition(const std::string& condition, std::string& field_name, std::string& value);

    static bool match_between_condition(const std::string& condition, std::string& field_name,
        std::string& low_value, std::string& high_value);

    static bool match_like_condition(const std::string& condition, std::string& field_name, std::string& pattern);

    bool is_numeric(const std::string& str);

    int compare_values(const std::string& value1, const std::string& value2, const std::string& type);

    std::vector<std::string> get_primary_key_fields(const std::string& table_name);

    std::string capitalize(const std::string& str);

    bool matches_like_pattern(const std::string& str, const std::string& pattern);

public:
    /*
     * 判断条件是否可以使用索引优化
     * @param condition WHERE条件
     * @param table_name 表名
     * @return 如果条件中包含可索引字段并且该字段有索引，返回true
     */
    static bool can_use_index(const std::string& condition, const std::string& table_name);

    //更新索引操作
    void updateIndexesAfterInsert(const std::string& table_name);
    void updateIndexesAfterDelete(const std::string& table_name, const std::vector<std::string>& deletedValues, const RecordPointer& recordPtr);
    void updateIndexesAfterUpdate(const std::string& table_name, const std::vector<std::string>& oldValues, const std::vector<std::string>& newValues, const RecordPointer& recordPtr);
    RecordPointer get_last_inserted_record_pointer(const std::string& table_name);

    static std::string read_field(std::ifstream& file, const FieldBlock& field);



};

std::tm custom_strptime(const std::string& datetime_str, const std::string& format);

#endif // RECORD_H