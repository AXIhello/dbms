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

// 读取约束信息
std::vector<ConstraintBlock> Record::read_constraints(const std::string& table_name) {
    std::vector<ConstraintBlock> constraints;
    std::string tic_filename = table_name + ".tic";
    std::ifstream file(tic_filename, std::ios::binary);

    if (!file) {
        // 如果约束文件不存在，返回空约束列表
        return constraints;
    }

    // 读取约束块
    ConstraintBlock constraint;
    while (file.read(reinterpret_cast<char*>(&constraint), sizeof(ConstraintBlock))) {
        constraints.push_back(constraint);
    }

    file.close();
    return constraints;
}

// 检查所有约束
bool Record::check_constraints(const std::vector<std::string>& columns,
    const std::vector<std::string>& values,
    const std::vector<ConstraintBlock>& constraints) {
    // 创建列名到值的映射
    std::unordered_map<std::string, std::string> column_values;
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i < values.size()) {
            column_values[columns[i]] = values[i];
        }
    }

    // 检查每个约束
    for (const auto& constraint : constraints) {
        // 获取约束对应的字段名
        std::string field_name = constraint.field;

        // 如果字段不在插入/更新的列中，则跳过此约束检查（除非是NOT NULL约束）
        if (column_values.find(field_name) == column_values.end() &&
            constraint.type != 5) { // 5 代表 NOT NULL 约束
            continue;
        }

        // 获取字段值
        std::string field_value = column_values[field_name];

        // 根据约束类型进行检查
        bool constraint_satisfied = false;

        switch (constraint.type) {
        case 1: // 主键
            constraint_satisfied = check_primary_key_constraint(constraint, field_value);
            break;
        case 2: // 外键
            constraint_satisfied = check_foreign_key_constraint(constraint, field_value);
            break;
        case 3: // CHECK约束
            constraint_satisfied = check_check_constraint(constraint, field_value);
            break;
        case 4: // UNIQUE约束
            constraint_satisfied = check_unique_constraint(constraint, field_value);
            break;
        case 5: // NOT NULL约束
            constraint_satisfied = check_not_null_constraint(constraint, field_value);
            break;
        case 6: // DEFAULT约束
            constraint_satisfied = check_default_constraint(constraint, field_value);
            // 如果使用了默认值，更新column_values中的值
            if (constraint_satisfied) {
                column_values[field_name] = field_value;
            }
            break;
        case 7: // 自增
            constraint_satisfied = check_auto_increment_constraint(constraint, field_value);
            // 如果使用了自增值，更新column_values中的值
            if (constraint_satisfied) {
                column_values[field_name] = field_value;
            }
            break;
        default:
            constraint_satisfied = true; // 未知约束类型，默认通过
        }

        if (!constraint_satisfied) {
            // 约束检查失败
            std::string constraint_name = constraint.name;
            std::cerr << "约束违反: " << constraint_name << " 不满足";
            return false;
        }
    }

    // 将可能修改过的值（如DEFAULT或AUTO_INCREMENT）更新回values向量
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i < values.size() && column_values.find(columns[i]) != column_values.end()) {
            const_cast<std::vector<std::string>&>(values)[i] = column_values[columns[i]];
        }
    }

    return true;
}

// 检查主键约束
bool Record::check_primary_key_constraint(const ConstraintBlock& constraint, const std::string& value) {
    std::string field_name = constraint.field;

    // 检查值是否为NULL
    if (value.empty() || value == "NULL" || value == "''" || value == "\"\"") {
        std::cerr << "主键约束违反: " << field_name << " 不能为NULL" << std::endl;
        return false;
    }

    // 检查是否唯一
    std::string table_name = this->table_name;
    std::string condition = field_name + " = " + value;

    try {
        std::vector<Record> existing_records = select("*", table_name, condition);
        if (!existing_records.empty()) {
            std::cerr << "主键约束违反: " << field_name << " = " << value << " 已存在" << std::endl;
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "主键约束检查失败: " << e.what() << std::endl;
        return false;
    }

    return true;
}

// 检查外键约束
bool Record::check_foreign_key_constraint(const ConstraintBlock& constraint, const std::string& value) {
    // 从param中提取被引用的表和字段
    std::string param = constraint.param;
    std::istringstream param_stream(param);
    std::string ref_table, ref_field;

    // 格式应为：表名.字段名
    if (!std::getline(param_stream, ref_table, '.') ||
        !std::getline(param_stream, ref_field)) {
        std::cerr << "外键约束格式错误: " << param << std::endl;
        return false;
    }

    // 如果值为NULL，且外键允许NULL，则通过检查
    if (value.empty() || value == "NULL" || value == "''" || value == "\"\"") {
        // 这里可以增加对外键是否允许NULL的检查，暂时都允许
        return true;
    }

    // 检查引用的值是否存在于被引用的表中
    std::string condition = ref_field + " = " + value;

    try {
        std::vector<Record> ref_records = select("*", ref_table, condition);
        if (ref_records.empty()) {
            std::cerr << "外键约束违反: 在表 " << ref_table << " 中没有找到 "
                << ref_field << " = " << value << std::endl;
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "外键约束检查失败: " << e.what() << std::endl;
        return false;
    }

    return true;
}

// 检查唯一约束
bool Record::check_unique_constraint(const ConstraintBlock& constraint, const std::string& value) {
    // 如果值为NULL，唯一约束通常允许多个NULL值
    if (value.empty() || value == "NULL" || value == "''" || value == "\"\"") {
        return true;
    }

    std::string field_name = constraint.field;
    std::string table_name = this->table_name;
    std::string condition = field_name + " = " + value;

    try {
        std::vector<Record> existing_records = select("*", table_name, condition);
        if (!existing_records.empty()) {
            std::cerr << "唯一约束违反: " << field_name << " = " << value << " 已存在" << std::endl;
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "唯一约束检查失败: " << e.what() << std::endl;
        return false;
    }

    return true;
}

// 检查非空约束
bool Record::check_not_null_constraint(const ConstraintBlock& constraint, const std::string& value) {
    if (value.empty() || value == "NULL" || value == "''" || value == "\"\"") {
        std::cerr << "非空约束违反: " << constraint.field << " 不能为NULL" << std::endl;
        return false;
    }
    return true;
}

// 检查CHECK约束
bool Record::check_check_constraint(const ConstraintBlock& constraint, const std::string& value) {
    // 从param中提取CHECK表达式
    std::string check_expr = constraint.param;

    // 简单实现：仅支持一些基本的比较操作
    // 实际上应该使用表达式解析器或正则表达式进行更复杂的检查

    // 如果值为NULL，CHECK约束通常不适用
    if (value.empty() || value == "NULL" || value == "''" || value == "\"\"") {
        return true;
    }

    // 去除值中的引号
    std::string clean_value = value;
    if ((clean_value.front() == '\'' && clean_value.back() == '\'') ||
        (clean_value.front() == '\"' && clean_value.back() == '\"')) {
        clean_value = clean_value.substr(1, clean_value.length() - 2);
    }

    // 匹配 > 操作
    std::regex gt_regex("\\s*>\\s*(\\d+)\\s*");
    std::smatch matches;
    if (std::regex_match(check_expr, matches, gt_regex)) {
        int check_val = std::stoi(matches[1]);
        try {
            int val = std::stoi(clean_value);
            return val > check_val;
        }
        catch (...) {
            return false;
        }
    }

    // 匹配 < 操作
    std::regex lt_regex("\\s*<\\s*(\\d+)\\s*");
    if (std::regex_match(check_expr, matches, lt_regex)) {
        int check_val = std::stoi(matches[1]);
        try {
            int val = std::stoi(clean_value);
            return val < check_val;
        }
        catch (...) {
            return false;
        }
    }

    // 匹配 >= 操作
    std::regex gte_regex("\\s*>=\\s*(\\d+)\\s*");
    if (std::regex_match(check_expr, matches, gte_regex)) {
        int check_val = std::stoi(matches[1]);
        try {
            int val = std::stoi(clean_value);
            return val >= check_val;
        }
        catch (...) {
            return false;
        }
    }

    // 匹配 <= 操作
    std::regex lte_regex("\\s*<=\\s*(\\d+)\\s*");
    if (std::regex_match(check_expr, matches, lte_regex)) {
        int check_val = std::stoi(matches[1]);
        try {
            int val = std::stoi(clean_value);
            return val <= check_val;
        }
        catch (...) {
            return false;
        }
    }

    // 更多复杂的CHECK表达式解析...

    // 默认返回true，表示通过检查
    return true;
}

// 检查DEFAULT约束
bool Record::check_default_constraint(const ConstraintBlock& constraint, std::string& value) {
    // 如果值为NULL或未提供，则使用默认值
    if (value.empty() || value == "NULL" || value == "''" || value == "\"\"") {
        // 从param中获取默认值
        std::string default_value = constraint.param;
        value = default_value;
        return true;
    }
    // 如果已提供值，则不需要使用默认值
    return true;
}

// 检查自增约束
bool Record::check_auto_increment_constraint(const ConstraintBlock& constraint, std::string& value) {
    // 如果值为NULL或未提供，则生成自增值
    if (value.empty() || value == "NULL" || value == "''" || value == "\"\"") {
        std::string field_name = constraint.field;
        std::string table_name = this->table_name;

        // 查找当前最大值
        try {
            std::vector<Record> records = select(field_name, table_name, "");
            int max_val = 0;

            for (const auto& record : records) {
                const std::vector<std::string>& values = record.get_values();
                if (!values.empty()) {
                    try {
                        int curr_val = std::stoi(values[0]);
                        max_val = std::max(max_val, curr_val);
                    }
                    catch (...) {
                        // 忽略非整数值
                    }
                }
            }

            // 自增值为当前最大值+1
            value = std::to_string(max_val + 1);
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "自增值生成失败: " << e.what() << std::endl;
            return false;
        }
    }
    // 如果已提供值，则检查是否为整数
    try {
        std::stoi(value);
        return true;
    }
    catch (...) {
        std::cerr << "自增字段值必须为整数: " << value << std::endl;
        return false;
    }
}

// 检查引用完整性（在删除前检查）
bool Record::check_references_before_delete(const std::string& table_name,
    std::unordered_map<std::string, std::string>& record_data) {
    // 获取系统中所有表
    // 这需要有一个系统表或目录来存储所有表名
    // 简化起见，可以扫描当前目录中的.tdf文件
    std::vector<std::string> all_tables;

    // 获取当前目录下的所有文件
    // 实际中应使用平台相关的目录遍历API
    // 简化实现，仅作示例

    // 检查每个表是否有引用当前要删除的记录的外键
    for (const auto& ref_table : all_tables) {
        if (ref_table == table_name) continue; // 跳过自身

        // 读取引用表的约束
        std::vector<ConstraintBlock> ref_constraints = read_constraints(ref_table);

        for (const auto& constraint : ref_constraints) {
            // 检查是否为外键约束，且引用了当前表
            if (constraint.type == 2) { // 外键约束
                std::string param = constraint.param;
                if (param.find(table_name + ".") == 0) {
                    // 找到了引用当前表的外键约束
                    // 提取被引用的字段名
                    std::string ref_field = param.substr(table_name.length() + 1);

                    // 检查被引用的字段是否在要删除的记录中
                    if (record_data.find(ref_field) != record_data.end()) {
                        // 检查是否有记录引用了要删除的值
                        std::string ref_value = record_data[ref_field];
                        std::string condition = std::string(constraint.field) + " = " + std::string(ref_value);

                        try {
                            std::vector<Record> ref_records = select("*", ref_table, condition);
                            if (!ref_records.empty()) {
                                std::cerr << "引用完整性约束违反: 表 " << ref_table
                                    << " 中的记录引用了要删除的值 " << ref_value << std::endl;
                                return false;
                            }
                        }
                        catch (const std::exception& e) {
                            std::cerr << "引用完整性检查失败: " << e.what() << std::endl;
                            return false;
                        }
                    }
                }
            }
        }
    }

    return true;
}