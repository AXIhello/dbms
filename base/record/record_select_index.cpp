#include "Record.h"
#include "parse/parse.h"
#include "ui/output.h"

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <regex>
#include <map>
#include <set>


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

// 解析WHERE条件，按表名拆分条件
void Record::parse_conditions_by_table(
    const std::string& condition,
    const std::vector<std::string>& tables,
    std::unordered_map<std::string, std::string>& table_conditions,
    std::string& join_condition)
{
    // 此函数将条件分解为各表的独立条件和表间连接条件
    // 实现较复杂，这里给出简化版本

    // 1. 先分割AND连接的多个条件
    std::vector<std::string> conditions;
    split_condition_by_and(condition, conditions);

    // 2. 对每个条件，判断它属于哪个表或是连接条件
    for (const auto& cond : conditions) {
        bool is_join_condition = false;
        std::string relevant_table;

        // 检查条件是否包含表名前缀(如 "table1.field = value")
        for (const auto& table : tables) {
            std::string table_prefix = table + ".";
            if (cond.find(table_prefix) != std::string::npos) {
                // 检查是否包含其他表的前缀，如果有则为连接条件
                for (const auto& other_table : tables) {
                    if (other_table != table) {
                        std::string other_prefix = other_table + ".";
                        if (cond.find(other_prefix) != std::string::npos) {
                            is_join_condition = true;
                            break;
                        }
                    }
                }

                if (!is_join_condition) {
                    relevant_table = table;
                }
                break;
            }
        }

        // 如果没有表前缀，尝试根据字段名确定表
        if (relevant_table.empty() && !is_join_condition) {
            // 提取条件中的字段名
            std::string field_name = extract_field_name_from_condition(cond);

            // 对每个表检查是否包含此字段
            for (const auto& table : tables) {
                if (table_has_field(table, field_name)) {
                    relevant_table = table;
                    break;
                }
            }
        }

        // 添加到相应的条件集合
        if (is_join_condition) {
            if (!join_condition.empty()) join_condition += " AND ";
            join_condition += cond;
        }
        else if (!relevant_table.empty()) {
            if (table_conditions.find(relevant_table) == table_conditions.end()) {
                table_conditions[relevant_table] = cond;
            }
            else {
                table_conditions[relevant_table] += " AND " + cond;
            }
        }
        else {
            // 无法确定表的条件，添加到所有表
            for (const auto& table : tables) {
                if (table_conditions.find(table) == table_conditions.end()) {
                    table_conditions[table] = cond;
                }
                else {
                    table_conditions[table] += " AND " + cond;
                }
            }
        }
    }
}

// 从条件中提取适合索引的部分
void Record::extract_index_conditions(
    const std::string& table_name,
    const std::string& condition,
    std::vector<IndexCondition>& index_conditions)
{
    // 分割AND连接的条件
    std::vector<std::string> cond_parts;
    split_condition_by_and(condition, cond_parts);

    for (const auto& part : cond_parts) {
        // 去除空格
        std::string clean_part = trim(part);

        // 尝试匹配各种条件模式
        IndexCondition idx_cond;

        // 尝试匹配 "field = value" 模式
        if (match_equals_condition(clean_part, idx_cond.field_name, idx_cond.value)) {
            idx_cond.op_type = OperatorType::EQUALS;
            // 如果有表前缀，去除
            remove_table_prefix(idx_cond.field_name);
            index_conditions.push_back(idx_cond);
            continue;
        }

        // 尝试匹配 "field > value" 模式
        if (match_greater_than_condition(clean_part, idx_cond.field_name, idx_cond.value)) {
            idx_cond.op_type = OperatorType::GREATER_THAN;
            remove_table_prefix(idx_cond.field_name);
            index_conditions.push_back(idx_cond);
            continue;
        }

        // 尝试匹配 "field >= value" 模式
        if (match_greater_equal_condition(clean_part, idx_cond.field_name, idx_cond.value)) {
            idx_cond.op_type = OperatorType::GREATER_EQUAL;
            remove_table_prefix(idx_cond.field_name);
            index_conditions.push_back(idx_cond);
            continue;
        }

        // 尝试匹配 "field < value" 模式
        if (match_less_than_condition(clean_part, idx_cond.field_name, idx_cond.value)) {
            idx_cond.op_type = OperatorType::LESS_THAN;
            remove_table_prefix(idx_cond.field_name);
            index_conditions.push_back(idx_cond);
            continue;
        }

        // 尝试匹配 "field <= value" 模式
        if (match_less_equal_condition(clean_part, idx_cond.field_name, idx_cond.value)) {
            idx_cond.op_type = OperatorType::LESS_EQUAL;
            remove_table_prefix(idx_cond.field_name);
            index_conditions.push_back(idx_cond);
            continue;
        }

        // 尝试匹配 "field BETWEEN low AND high" 模式
        if (match_between_condition(clean_part, idx_cond.field_name, idx_cond.low_value, idx_cond.high_value)) {
            idx_cond.op_type = OperatorType::BETWEEN;
            remove_table_prefix(idx_cond.field_name);
            index_conditions.push_back(idx_cond);
            continue;
        }

        // 尝试匹配 "field LIKE pattern" 模式
        if (match_like_condition(clean_part, idx_cond.field_name, idx_cond.value)) {
            idx_cond.op_type = OperatorType::LIKE;
            remove_table_prefix(idx_cond.field_name);
            index_conditions.push_back(idx_cond);
            continue;
        }

        // 可以继续添加其他条件类型的匹配...
    }
}

std::string Record::read_field(std::ifstream& file, const FieldBlock& field) {
    if (!file.is_open()) {
        throw std::runtime_error("读取字段失败：文件未打开");
    }

    switch (field.type) {
    case 1: { // INT (4 bytes)
        int32_t val;
        file.read(reinterpret_cast<char*>(&val), sizeof(int32_t));
        return std::to_string(val);
    }
    case 2: { // DOUBLE (8 bytes)
        double val;
        file.read(reinterpret_cast<char*>(&val), sizeof(double));
        return std::to_string(val);
    }
    case 3: { // VARCHAR / CHAR
        // param 表示最大长度（比如 param = 255）
        char buffer[256] = { 0 };
        if (field.param > 255 || field.param <= 0)
            throw std::runtime_error("字段 '" + std::string(field.name) + "' 的 param 无效：" + std::to_string(field.param));
        file.read(buffer, field.param);
        return std::string(buffer, strnlen(buffer, field.param));  // 去掉尾部 \0
    }
    case 4: { // BOOL (1 byte)
        char val;
        file.read(&val, sizeof(char));
        return (val == 0) ? "false" : "true";
    }
    case 5: { // DATETIME (16 bytes)
        char buffer[17] = { 0 };
        file.read(buffer, 16);
        return std::string(buffer, 16);  // 返回格式化字符串，如 "2025-05-06 21:30"
    }
    default:
        throw std::runtime_error("未知字段类型: " + std::to_string(field.type));
    }
}



std::pair<uint64_t, std::unordered_map<std::string, std::string>>
Record::fetch_record_by_pointer(const std::string& table_name, uint64_t target_row_id) {
    std::vector<FieldBlock> fields = read_field_blocks(table_name);
    std::string trd_path = dbManager::getInstance().get_current_database()->getDBPath() + "/" + table_name + ".trd";

    std::ifstream file(trd_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("无法打开数据文件: " + table_name + ".trd");
    }

    while (file.peek() != EOF) {
        uint64_t row_id;
        file.read(reinterpret_cast<char*>(&row_id), sizeof(uint64_t));

        char delete_flag;
        file.read(&delete_flag, sizeof(char));

        std::unordered_map<std::string, std::string> record_data;
        for (const auto& field : fields) {
            std::string value = read_field(file, field);
            record_data[field.name] = value;
        }

        if (delete_flag == 0 && row_id == target_row_id) {
            file.close();
            return { row_id, record_data };
        }
    }

    file.close();
    throw std::runtime_error("未找到 row_id = " + std::to_string(target_row_id) + " 对应的记录");
}



//// 获取表字段的索引
//BTree* Record::get_index_for_field(const std::string& table_name, const std::string& field_name)
//{
//    // 获取表对象
//    Table * table = dbManager::getInstance().get_current_database()->getTable(table_name);
//    if (!table) {
//        return nullptr;
//    }
//
//	std::vector<IndexBlock> indexes = table->getIndexes();
//    std::vector<std::unique_ptr<BTree>> btrees = table->getBTreeByIndexName(); 
//
//    // 查找字段对应的索引
//    for (size_t i = 0; i < indexes.size(); ++i) {
//        const IndexBlock& idx = indexes[i];
//
//        // 检查是否是该字段的索引
//        for (int f = 0; f < idx.field_num; ++f) {
//            if (field_name == idx.field[f]) {
//                // 找到匹配的索引
//                if (i < table->m_btrees.size() && table->m_btrees[i]) {
//                    return table->m_btrees[i].get();
//                }
//            }
//        }
//    }
//
//    return nullptr; // 未找到索引
//}



// 分割条件
void Record::split_condition_by_and(const std::string& condition, std::vector<std::string>& parts)
{
    // 简单实现，不考虑括号内的AND
    size_t start = 0;
    size_t pos;
    while ((pos = condition.find(" AND ", start)) != std::string::npos) {
        parts.push_back(condition.substr(start, pos - start));
        start = pos + 5; // " AND "的长度是5
    }
    parts.push_back(condition.substr(start));
}


// 提取字段名
std::string Record::extract_field_name_from_condition(const std::string& condition)
{
    std::string trimmed = trim(condition);

    // 查找常见操作符
    size_t pos = std::string::npos;
    const std::vector<std::string> operators = { "=", "!=", ">", ">=", "<", "<=", " LIKE ", " IN ", " IS ", " BETWEEN " };

    for (const auto& op : operators) {
        pos = trimmed.find(op);
        if (pos != std::string::npos) {
            // 获取操作符前的部分作为字段名
            std::string field = trimmed.substr(0, pos);
            return trim(field);
        }
    }

    // 未找到操作符，可能整个条件就是字段名（比如IS NULL的简写）
    return trimmed;
}

// 判断表是否包含指定字段
bool Record::table_has_field(const std::string& table_name, const std::string& field_name)
{
    Table* table = dbManager::getInstance().get_current_database()->getTable(table_name);
    if (!table) return false;

    for (const auto& field : table->getFields()) {
        if (field.name == field_name) {
            return true;
        }
    }

    return false;
}

// 移除表前缀
void Record::remove_table_prefix(std::string& field_name)
{
    size_t dot_pos = field_name.find('.');
    if (dot_pos != std::string::npos) {
        field_name = field_name.substr(dot_pos + 1);
    }
}

// 字符串修剪
std::string Record::trim(const std::string& str)
{
    const std::string whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);

    if (start == std::string::npos) {
        return ""; // 字符串全是空白字符
    }

    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

// 匹配等值条件
bool Record::match_equals_condition(const std::string& condition, std::string& field_name, std::string& value)
{
    size_t pos = condition.find('=');
    if (pos == std::string::npos) return false;

    // 检查不是 >= 或 <=
    if (pos > 0 && (condition[pos - 1] == '>' || condition[pos - 1] == '<' || condition[pos - 1] == '!')) {
        return false;
    }

    field_name = trim(condition.substr(0, pos));
    value = trim(condition.substr(pos + 1));

    // 如果值被引号括起来，去除引号
    if ((value.front() == '\'' && value.back() == '\'') ||
        (value.front() == '"' && value.back() == '"')) {
        value = value.substr(1, value.length() - 2);
    }

    return !field_name.empty();
}

// 匹配大于条件
bool Record::match_greater_than_condition(const std::string& condition, std::string& field_name, std::string& value)
{
    size_t pos = condition.find('>');
    if (pos == std::string::npos) return false;

    // 检查不是 >=
    if (pos + 1 < condition.length() && condition[pos + 1] == '=') {
        return false;
    }

    field_name = trim(condition.substr(0, pos));
    value = trim(condition.substr(pos + 1));

    // 如果值被引号括起来，去除引号
    if ((value.front() == '\'' && value.back() == '\'') ||
        (value.front() == '"' && value.back() == '"')) {
        value = value.substr(1, value.length() - 2);
    }

    return !field_name.empty();
}

// 匹配大于等于条件
bool Record::match_greater_equal_condition(const std::string& condition, std::string& field_name, std::string& value)
{
    size_t pos = condition.find(">=");
    if (pos == std::string::npos) return false;

    field_name = trim(condition.substr(0, pos));
    value = trim(condition.substr(pos + 2));

    // 如果值被引号括起来，去除引号
    if ((value.front() == '\'' && value.back() == '\'') ||
        (value.front() == '"' && value.back() == '"')) {
        value = value.substr(1, value.length() - 2);
    }

    return !field_name.empty();
}

// 匹配小于条件
bool Record::match_less_than_condition(const std::string& condition, std::string& field_name, std::string& value)
{
    size_t pos = condition.find('<');
    if (pos == std::string::npos) return false;

    // 检查不是 <=
    if (pos + 1 < condition.length() && condition[pos + 1] == '=') {
        return false;
    }

    field_name = trim(condition.substr(0, pos));
    value = trim(condition.substr(pos + 1));

    // 如果值被引号括起来，去除引号
    if ((value.front() == '\'' && value.back() == '\'') ||
        (value.front() == '"' && value.back() == '"')) {
        value = value.substr(1, value.length() - 2);
    }

    return !field_name.empty();
}

// 匹配小于等于条件
bool Record::match_less_equal_condition(const std::string& condition, std::string& field_name, std::string& value)
{
    size_t pos = condition.find("<=");
    if (pos == std::string::npos) return false;

    field_name = trim(condition.substr(0, pos));
    value = trim(condition.substr(pos + 2));

    // 如果值被引号括起来，去除引号
    if ((value.front() == '\'' && value.back() == '\'') ||
        (value.front() == '"' && value.back() == '"')) {
        value = value.substr(1, value.length() - 2);
    }

    return !field_name.empty();
}

// 匹配BETWEEN条件
bool Record::match_between_condition(const std::string& condition, std::string& field_name,
    std::string& low_value, std::string& high_value)
{
    size_t between_pos = condition.find(" BETWEEN ");
    if (between_pos == std::string::npos) return false;

    size_t and_pos = condition.find(" AND ", between_pos + 9); // 9是" BETWEEN "的长度
    if (and_pos == std::string::npos) return false;

    field_name = trim(condition.substr(0, between_pos));
    low_value = trim(condition.substr(between_pos + 9, and_pos - (between_pos + 9)));
    high_value = trim(condition.substr(and_pos + 5)); // 5是" AND "的长度

    // 如果值被引号括起来，去除引号
    if ((low_value.front() == '\'' && low_value.back() == '\'') ||
        (low_value.front() == '"' && low_value.back() == '"')) {
        low_value = low_value.substr(1, low_value.length() - 2);
    }

    if ((high_value.front() == '\'' && high_value.back() == '\'') ||
        (high_value.front() == '"' && high_value.back() == '"')) {
        high_value = high_value.substr(1, high_value.length() - 2);
    }

    return !field_name.empty();
}

// 匹配LIKE条件
bool Record::match_like_condition(const std::string& condition, std::string& field_name, std::string& pattern)
{
    size_t like_pos = condition.find(" LIKE ");
    if (like_pos == std::string::npos) return false;

    field_name = trim(condition.substr(0, like_pos));
    pattern = trim(condition.substr(like_pos + 6)); // 6是" LIKE "的长度

    // 如果模式被引号括起来，去除引号
    if ((pattern.front() == '\'' && pattern.back() == '\'') ||
        (pattern.front() == '"' && pattern.back() == '"')) {
        pattern = pattern.substr(1, pattern.length() - 2);
    }

    return !field_name.empty();
}

//// 解析列表，如SELECT中的列名列表
//std::vector<std::string> Record::parse_column_list(const std::string& columns_str)
//{
//    std::vector<std::string> result;
//    std::string current;
//    bool in_quotes = false;
//    char quote_char = '\0';
//
//    for (char c : columns_str) {
//        if ((c == '\'' || c == '"') && (quote_char == '\0' || c == quote_char)) {
//            in_quotes = !in_quotes;
//            if (in_quotes) quote_char = c;
//            else quote_char = '\0';
//            current += c;
//        }
//        else if (c == ',' && !in_quotes) {
//            result.push_back(trim(current));
//            current.clear();
//        }
//        else {
//            current += c;
//        }
//    }
//
//    if (!current.empty()) {
//        result.push_back(trim(current));
//    }
//
//    return result;
//}

// 检查一个字符串是否是数字
bool Record::is_numeric(const std::string& str)
{
    if (str.empty()) return false;

    bool has_decimal = false;
    bool has_sign = false;

    for (size_t i = 0; i < str.length(); ++i) {
        char c = str[i];

        if (i == 0 && (c == '+' || c == '-')) {
            has_sign = true;
        }
        else if (c == '.' && !has_decimal) {
            has_decimal = true;
        }
        else if (!std::isdigit(c)) {
            return false;
        }
    }

    return str.length() > (has_sign ? 1 : 0);
}

// 比较两个值，支持不同类型比较
int Record::compare_values(const std::string& value1, const std::string& value2, const std::string& type)
{
    if (type == "INT" || type == "INTEGER") {
        // 整数比较
        int val1 = std::stoi(value1);
        int val2 = std::stoi(value2);
        return (val1 < val2) ? -1 : ((val1 > val2) ? 1 : 0);
    }
    else if (type == "REAL" || type == "FLOAT" || type == "DOUBLE") {
        // 浮点数比较
        double val1 = std::stod(value1);
        double val2 = std::stod(value2);
        return (val1 < val2) ? -1 : ((val1 > val2) ? 1 : 0);
    }
    else if (type == "BOOL" || type == "BOOLEAN") {
        // 布尔值比较
        bool val1 = (value1 == "TRUE" || value1 == "1");
        bool val2 = (value2 == "TRUE" || value2 == "1");
        return (val1 < val2) ? -1 : ((val1 > val2) ? 1 : 0);
    }
    else if (type == "DATE" || type == "TIME" || type == "TIMESTAMP") {
        // 日期时间比较 - 简化为字符串比较
        return value1.compare(value2);
    }
    else {
        // 默认使用字符串比较
        return value1.compare(value2);
    }
}

// 获取表的主键或唯一索引字段
std::vector<std::string> Record::get_primary_key_fields(const std::string& table_name)
{
    std::vector<std::string> primary_keys;

    Table* table = dbManager::getInstance().get_current_database()->getTable(table_name);
    if (!table) return primary_keys;



    // 检查约束以找到主键
    for (const auto& constraint : table->getConstraints()) {
        if (constraint.type == 1) {
            primary_keys.push_back(constraint.field);

            return primary_keys; // 找到主键就返回
        }
    }

    // 没有主键，检查唯一索引
    for (const auto& index : table->getIndexes()) {
        if (index.unique) {
            for (int i = 0; i < index.field_num; ++i) {
                primary_keys.push_back(index.field[i]);
            }
            return primary_keys; // 找到唯一索引就返回
        }
    }

    return primary_keys;
}

//// 根据表名和字段名查找对应的B树索引
//bool Record::get_btree_for_field(const std::string& table_name, const std::string& field_name,
//    BTree*& btree, const IndexBlock*& index_block)
//{
//    Table* table = dbManager::getInstance().get_current_database()->getTable(table_name);
//    if (!table) return false;
//
//	std::vector<IndexBlock> indexes = table->getIndexes();
//
//    for (size_t i = 0; i < indexes.size(); ++i) {
//        const IndexBlock& idx = indexes[i];
//
//        // 检查是否包含该字段
//        for (int j = 0; j < idx.field_num; ++j) {
//            if (std::string(idx.field[j]) == field_name) {
//                if (i < table->m_btrees.size() && table->m_btrees[i]) {
//                    btree = table->m_btrees[i].get();
//                    index_block = &idx;
//                    return true;
//                }
//            }
//        }
//    }
//
//    return false;
//}

// 将输入字符串的首字母大写
std::string Record::capitalize(const std::string& str)
{
    if (str.empty()) return str;

    std::string result = str;
    result[0] = std::toupper(result[0]);

    for (size_t i = 1; i < result.length(); ++i) {
        if (result[i - 1] == ' ') {
            result[i] = std::toupper(result[i]);
        }
        else {
            result[i] = std::tolower(result[i]);
        }
    }

    return result;
}

// 匹配LIKE模式
bool Record::matches_like_pattern(const std::string& str, const std::string& pattern)
{
    // 简化的LIKE模式匹配实现
    // 支持 % (匹配0个或多个字符) 和 _ (匹配1个字符)

    // 转换模式为正则表达式
    std::string regex_pattern;
    regex_pattern.reserve(pattern.size() * 2);

    regex_pattern.push_back('^');
    for (char c : pattern) {
        if (c == '%') {
            regex_pattern.append(".*");
        }
        else if (c == '_') {
            regex_pattern.append(".");
        }
        else {
            // 转义正则表达式特殊字符
            if (c == '.' || c == '+' || c == '*' || c == '?' || c == '^' || c == '$' ||
                c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || c == '|' || c == '\\') {
                regex_pattern.push_back('\\');
            }
            regex_pattern.push_back(c);
        }
    }
    regex_pattern.push_back('$');

    std::regex regex(regex_pattern);
    return std::regex_match(str, regex);
}


