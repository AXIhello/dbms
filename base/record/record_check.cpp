#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "Record.h"
#include "parse/parse.h"
#include "ui/output.h"

#include <regex>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <filesystem>

inline bool is_null(const std::string& value) {
    return value == "NULL";
}

std::vector<std::string> Record::tokenize(const std::string& expr) {
    std::vector<std::string> tokens;

    // 更新正则表达式：
    // 1️⃣ 匹配类似于 "table1.field = table2.field"
    // 2️⃣ 匹配 "field = 'value'" 或 "field = value"
    // 3️⃣ 匹配 "AND" 或 "OR"
    std::regex re(R"((\w+\.\w+\s*(=|!=|<|>|<=|>=)\s*\w+\.\w+|\w+\.\w+\s*(=|!=|<|>|<=|>=)\s*'[^']*'|\w+\s*(=|!=|<|>|<=|>=)\s*'[^']*'|\w+\s*(=|!=|<|>|<=|>=)\s*\w+|\bAND\b|\bOR\b))");

    std::sregex_iterator iter(expr.begin(), expr.end(), re);
    std::sregex_iterator end;

    // 遍历所有匹配的条件
    while (iter != end) {
        tokens.push_back(iter->str());
        ++iter;
    }

    return tokens;
}


ExpressionNode* Record::build_expression_tree(const std::vector<std::string>& tokens) {
    if (tokens.empty()) return nullptr;

    // 实现 Shunting Yard 算法来处理操作符优先级
    std::stack<std::string> ops;
    std::stack<ExpressionNode*> nodes;

    // 定义操作符优先级 (AND 优先级高于 OR)
    std::unordered_map<std::string, int> precedence = {
        {"AND", 2},
        {"OR", 1}
    };

    for (const auto& token : tokens) {
        if (is_operator(token)) {
            // 处理操作符
            while (!ops.empty() && precedence[ops.top()] >= precedence[token]) {
                std::string op = ops.top();
                ops.pop();

                ExpressionNode* right = nodes.top(); nodes.pop();
                ExpressionNode* left = nodes.top(); nodes.pop();

                ExpressionNode* op_node = new ExpressionNode(op);
                op_node->left = left;
                op_node->right = right;
                nodes.push(op_node);
            }
            ops.push(token);
        }
        else {
            // 处理操作数
            nodes.push(new ExpressionNode(token));
        }
    }

    // 处理剩余的操作符
    while (!ops.empty()) {
        std::string op = ops.top();
        ops.pop();

        ExpressionNode* right = nodes.top(); nodes.pop();
        ExpressionNode* left = nodes.top(); nodes.pop();

        ExpressionNode* op_node = new ExpressionNode(op);
        op_node->left = left;
        op_node->right = right;
        nodes.push(op_node);
    }

    return nodes.empty() ? nullptr : nodes.top();
}

// 修改 evaluate_node 函数以正确处理条件表达式
bool evaluate_node(ExpressionNode* node, const std::unordered_map<std::string, std::string>& record) {
    if (!node) return false;

    if (!Record::is_operator(node->value)) {
        std::regex condition_regex(R"((\w+)\s*(=|!=|<|>|<=|>=)\s*('.*?'|\w+))");
        std::smatch match;

        if (std::regex_match(node->value, match, condition_regex)) {
            std::string field = match[1];
            std::string op = match[2];
            std::string value = match[3];

            // 检查字段是否存在于记录中
            if (record.find(field) == record.end()) return false;
            std::string actual_value = record.at(field);

            // 保留引号进行比较
            if (op == "=") {
                    return actual_value == value;
            }
            if (op == "!=") return actual_value != value;

            // 对于数值比较，尝试转换为数字
            try {
                double actual_num = std::stod(actual_value);
                double value_num = std::stod(value);

                if (op == "<") return actual_num < value_num;
                if (op == ">") return actual_num > value_num;
                if (op == "<=") return actual_num <= value_num;
                if (op == ">=") return actual_num >= value_num;
            }
            catch (const std::exception&) {
                // 如果转换失败，使用字符串比较
                if (op == "<") return actual_value < value;
                if (op == ">") return actual_value > value;
                if (op == "<=") return actual_value <= value;
                if (op == ">=") return actual_value >= value;
            }
        }
        return false;
    }

    // 递归计算
    bool left_result = evaluate_node(node->left, record);
    bool right_result = evaluate_node(node->right, record);

    if (node->value == "AND") return left_result && right_result;
    if (node->value == "OR") return left_result || right_result;

    return false;
}

// 表级约束校验
bool Record::check_table_level_constraint(const ConstraintBlock& constraint, const std::unordered_map<std::string, std::string>& column_values) {
    std::vector<std::string> tokens = tokenize(constraint.param);
    ExpressionNode* root = build_expression_tree(tokens);
    return evaluate_node(root, column_values);
}

std::vector<ConstraintBlock> Record::read_constraints(const std::string& table_name) {
    std::vector<ConstraintBlock> constraints;
    std::string tic_filename = dbManager::getInstance().get_current_database()->getDBPath() + "/" + table_name + ".tic";
    std::ifstream file(tic_filename, std::ios::binary);
    if (!file) return constraints;

    ConstraintBlock constraint;
    while (file.read(reinterpret_cast<char*>(&constraint), sizeof(ConstraintBlock))) {
        constraints.push_back(constraint);
    }
    file.close();
    return constraints;
}

bool Record::check_constraints(const std::vector<std::string>& columns,
    std::vector<std::string>& values,
    const std::vector<ConstraintBlock>& constraints) {
    std::unordered_map<std::string, std::string> column_values;
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i < values.size()) {
            column_values[columns[i]] = values[i];
        }
    }

    for (const auto& constraint : constraints) {
        std::string field_name = constraint.field;

        if (column_values.find(field_name) == column_values.end() && constraint.type != 5&&constraint.type!=3) {
            continue;
        }

        std::string field_value = column_values[field_name];
        bool satisfied = false;

        switch (constraint.type) {
        case 1:
            satisfied = check_primary_key_constraint(constraint, field_value);
            break;
        case 2:
            satisfied = check_foreign_key_constraint(constraint, field_value);
            break;
        case 3:
            if (std::strlen(constraint.field) == 0) {
                // 如果字段为空，表示是表级约束
                satisfied = check_table_level_constraint(constraint, column_values);
            }
            else {
                // 否则是列级约束
                satisfied = check_check_constraint(constraint, field_value);
            }
            break;

        case 4:
            satisfied = check_unique_constraint(constraint, field_value);
            break;
        case 5:
            satisfied = check_not_null_constraint(constraint, field_value);
            break;
        case 6:
            satisfied = check_default_constraint(constraint, field_value);
            if (satisfied) column_values[field_name] = field_value;
            break;
        case 7:
            satisfied = check_auto_increment_constraint(constraint, field_value);
            if (satisfied) column_values[field_name] = field_value;
            break;
        default:
            satisfied = true;
        }

        if (!satisfied) {
            std::cerr << "违反约束: " << constraint.name << " 字段: " << constraint.field << std::endl;
            return false;
        }
    }

    for (size_t i = 0; i < columns.size(); ++i) {
        if (i < values.size() && column_values.find(columns[i]) != column_values.end()) {
            values[i] = column_values[columns[i]];
        }
    }

    return true;
}

bool Record::check_primary_key_constraint(const ConstraintBlock& constraint, const std::string& value) {
    if (is_null(value)) {
        std::cerr << "主键不能为NULL: " << constraint.field << std::endl;
        return false;
    }
    std::string condition = std::string(constraint.field) + " = " + value;
    try {
        if (!select("*", table_name, condition,"","","").empty()) {
            std::cerr << "主键重复: " << constraint.field << " = " << value << std::endl;
            return false;
        }
    }
    catch (...) {
        return false;
    }
    return true;
}

bool Record::check_foreign_key_constraint(const ConstraintBlock& constraint, const std::string& value) {
    std::istringstream ss(constraint.param);
    std::string ref_table, ref_field;
    std::getline(ss, ref_table, '.');
    std::getline(ss, ref_field);

    if (is_null(value)) return true;

    std::string condition = ref_field + " = " + value ;
    try {
        if (select("*", ref_table, condition,"","","").empty()) {
            std::cerr << "外键约束违反: " << constraint.field << " = " << value << std::endl;
            return false;
        }
    }
    catch (...) {
        return false;
    }
    return true;
}

bool Record::check_unique_constraint(const ConstraintBlock& constraint, const std::string& value) {
    if (is_null(value)) return true;
    std::string condition = std::string(constraint.field) + " = " + value ;
    try {
        if (!select("*", table_name, condition,"","","").empty()) {
            std::cerr << "唯一约束违反: " << constraint.field << " = " << value << std::endl;
            return false;
        }
    }
    catch (...) {
        return false;
    }
    return true;
}

bool Record::check_not_null_constraint(const ConstraintBlock& constraint, const std::string& value) {
    if (is_null(value)) {
        std::cerr << "非空约束违反: " << constraint.field << std::endl;
        return false;
    }
    return true;
}

bool Record::check_check_constraint(const ConstraintBlock& constraint, const std::string& value) {
    if (is_null(value)) return true;

    std::string check_expr = constraint.param;
    std::string clean_value = value;

    try {
        double numeric_value = std::stod(clean_value);

        // 1. BETWEEN x AND y
        std::regex between_regex(R"(BETWEEN\s+(\d+)\s+AND\s+(\d+))", std::regex_constants::icase);
        std::smatch matches;
        if (std::regex_search(check_expr, matches, between_regex)) {
            double lower = std::stod(matches[1]);
            double upper = std::stod(matches[2]);
            return numeric_value >= lower && numeric_value <= upper;
        }

        // 2. IN (x, y, z, ...)
        std::regex in_regex(R"(IN\s*\(([^)]+)\))", std::regex_constants::icase);
        if (std::regex_search(check_expr, matches, in_regex)) {
            std::unordered_set<std::string> in_values;
            std::stringstream ss(matches[1]);
            std::string item;
            while (std::getline(ss, item, ',')) {
                item.erase(std::remove_if(item.begin(), item.end(), ::isspace), item.end());
                if (!item.empty()) {
                       in_values.insert(item);
                }
            }
            return in_values.find(clean_value) != in_values.end();
        }

        // 3. Comparison operators
        static const std::vector<std::pair<std::regex, std::function<bool(double, double)>>> operators = {
            {std::regex(R"(>=\s*(\d+))"), [](double a, double b) { return a >= b; }},
            {std::regex(R"(<=\s*(\d+))"), [](double a, double b) { return a <= b; }},
            {std::regex(R"(>\s*(\d+))"), [](double a, double b) { return a > b; }},
            {std::regex(R"(<\s*(\d+))"), [](double a, double b) { return a < b; }}
        };

        for (const auto& [regex, op] : operators) {
            if (std::regex_search(check_expr, matches, regex)) {
                double check_val = std::stod(matches[1]);
                return op(numeric_value, check_val);
            }
        }
    }
    catch (...) {
        return false;
    }

    // 如果没有匹配任何约束条件，则默认返回 true
    return true;
}


bool Record::check_default_constraint(const ConstraintBlock& constraint, std::string& value) {
    if (is_null(value)) {
        value = constraint.param;
    }
    return true;
}

bool Record::check_auto_increment_constraint(const ConstraintBlock& constraint, std::string& value) {
    if (is_null(value)) {
        try {
            auto records = select(constraint.field, table_name, "","","","");
            int max_val = 0;
            for (const auto& rec : records) {
                const auto& vals = rec.get_values();
                if (!vals.empty() && !is_null(vals[0])) {
                    int v = std::stoi(vals[0]);
                    max_val = std::max(max_val, v);
                }
            }
            value = std::to_string(max_val + 1);
        }
        catch (...) {
            return false;
        }
    }
    else {
        try {
            std::stoi(value);
        }
        catch (...) {
            std::cerr << "自增字段必须为整数: " << value << std::endl;
            return false;
        }
    }
    return true;
}

bool Record::check_references_before_delete(const std::string& table_name,
    const std::unordered_map<std::string, std::string>& record_data) {
    std::vector<std::string> all_tables;
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::current_path())) {
        if (entry.path().extension() == ".tdf") {
            all_tables.push_back(entry.path().stem().string());
        }
    }

    for (const auto& ref_table : all_tables) {
        if (ref_table == table_name) continue;
        auto constraints = read_constraints(ref_table);
        for (const auto& c : constraints) {
            if (c.type == 2) {
                std::string ref_prefix = table_name + ".";
                if (std::string(c.param).find(ref_prefix) == 0) {
                    std::string ref_field = c.param + strlen(ref_prefix.c_str());
                    if (record_data.find(ref_field) != record_data.end()) {
                        std::string val = record_data.at(ref_field);
                        std::string cond = std::string(c.field) + " = " + val ;
                        try {
                            if (!select("*", ref_table, cond,"","","").empty()) {
                                std::cerr << "引用完整性违反: " << ref_table << " 引用了 " << val << std::endl;
                                return false;
                            }
                        }
                        catch (...) {
                            return false;
                        }
                    }
                }
            }
        }
    }
    return true;
}