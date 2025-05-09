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

        if (column_values.find(field_name) == column_values.end() && constraint.type != 5) {
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
            satisfied = check_check_constraint(constraint, field_value);
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

    // 1. BETWEEN x AND y
    std::regex between_regex_num(R"(BETWEEN\s+(-?\d+\.?\d*)\s+AND\s+(-?\d+\.?\d*))", std::regex_constants::icase);
    std::regex between_regex_str(R"(BETWEEN\s+'([^']+)'\s+AND\s+'([^']+)')", std::regex_constants::icase);
    std::smatch matches;

    // 处理数值型BETWEEN
    if (std::regex_search(check_expr, matches, between_regex_num)) {
        try {
            double numeric_value = std::stod(clean_value);
            double lower = std::stod(matches[1]);
            double upper = std::stod(matches[2]);
            return numeric_value >= lower && numeric_value <= upper;
        }
        catch (...) {
            return false; // 转换失败则不满足约束
        }
    }

    // 处理字符串型BETWEEN
    if (std::regex_search(check_expr, matches, between_regex_str)) {
        std::string lower = matches[1];
        std::string upper = matches[2];
        return clean_value >= lower && clean_value <= upper;
    }

    // 2. IN (x, y, z, ...)
    std::regex in_regex_num(R"(IN\s*\(\s*([^)]+)\s*\))", std::regex_constants::icase);
    std::regex in_regex_str(R"(IN\s*\(\s*('[^']*'(?:\s*,\s*'[^']*')*)\s*\))", std::regex_constants::icase);

    // 处理数值型IN
    if (std::regex_search(check_expr, matches, in_regex_num)) {
        std::string in_list = matches[1];
        // 检查是否为字符串值列表
        std::regex quoted_values("'[^']+'");
        bool has_quoted = std::regex_search(in_list, quoted_values);

        if (!has_quoted) {
            try {
                double numeric_value = std::stod(clean_value);
                std::unordered_set<double> in_values;
                std::regex num_pattern(R"(-?\d+\.?\d*)");
                std::sregex_iterator iter(in_list.begin(), in_list.end(), num_pattern);
                std::sregex_iterator end;

                while (iter != end) {
                    in_values.insert(std::stod((*iter)[0]));
                    ++iter;
                }

                return in_values.find(numeric_value) != in_values.end();
            }
            catch (...) {
                return false; // 转换失败则不满足约束
            }
        }
    }

    // 处理字符串型IN
    if (std::regex_search(check_expr, matches, in_regex_str)) {
        std::string in_list = matches[1];
        std::unordered_set<std::string> str_values;

        std::regex str_pattern(R"('([^']*)')");
        std::sregex_iterator iter(in_list.begin(), in_list.end(), str_pattern);
        std::sregex_iterator end;

        while (iter != end) {
            str_values.insert((*iter)[1]); // 获取引号内的内容
            ++iter;
        }

        return str_values.find(clean_value) != str_values.end();
    }

    // 3. 比较运算符
    // 处理数值比较
    static const std::vector<std::pair<std::regex, std::function<bool(double, double)>>> num_operators = {
        {std::regex(R"(>=\s*(-?\d+\.?\d*))", std::regex_constants::icase), [](double a, double b) { return a >= b; }},
        {std::regex(R"(<=\s*(-?\d+\.?\d*))", std::regex_constants::icase), [](double a, double b) { return a <= b; }},
        {std::regex(R"(>\s*(-?\d+\.?\d*))", std::regex_constants::icase), [](double a, double b) { return a > b; }},
        {std::regex(R"(<\s*(-?\d+\.?\d*))", std::regex_constants::icase), [](double a, double b) { return a < b; }},
        {std::regex(R"(=\s*(-?\d+\.?\d*))", std::regex_constants::icase), [](double a, double b) { return a == b; }},
        {std::regex(R"(!=\s*(-?\d+\.?\d*))", std::regex_constants::icase), [](double a, double b) { return a != b; }}
    };

    try {
        double numeric_value = std::stod(clean_value);
        for (const auto& [regex, op] : num_operators) {
            if (std::regex_search(check_expr, matches, regex)) {
                double check_val = std::stod(matches[1]);
                return op(numeric_value, check_val);
            }
        }
    }
    catch (...) {
        // 数值转换失败，可能是字符串，继续检查字符串比较
    }

    // 处理字符串比较
    static const std::vector<std::pair<std::regex, std::function<bool(const std::string&, const std::string&)>>> str_operators = {
        {std::regex(R"(>=\s*'([^']*)')", std::regex_constants::icase), [](const std::string& a, const std::string& b) { return a >= b; }},
        {std::regex(R"(<=\s*'([^']*)')", std::regex_constants::icase), [](const std::string& a, const std::string& b) { return a <= b; }},
        {std::regex(R"(>\s*'([^']*)')", std::regex_constants::icase), [](const std::string& a, const std::string& b) { return a > b; }},
        {std::regex(R"(<\s*'([^']*)')", std::regex_constants::icase), [](const std::string& a, const std::string& b) { return a < b; }},
        {std::regex(R"(=\s*'([^']*)')", std::regex_constants::icase), [](const std::string& a, const std::string& b) { return a == b; }},
        {std::regex(R"(!=\s*'([^']*)')", std::regex_constants::icase), [](const std::string& a, const std::string& b) { return a != b; }}
    };

    for (const auto& [regex, op] : str_operators) {
        if (std::regex_search(check_expr, matches, regex)) {
            std::string check_val = matches[1];
            return op(clean_value, check_val);
        }
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