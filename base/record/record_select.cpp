#include "parse/parse.h"
#include "Record.h"
#include "ui/output.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>

// 是否存在该字段的索引文件
bool has_index(const std::string& table, const std::string& column) {
    std::string path = "data/index/" + table + "_" + column + ".idx";
    return std::filesystem::exists(path);
}

// 拆分复合条件表达式
void parse_condition_expressions(const std::string& cond, std::vector<std::string>& expressions, std::vector<std::string>& logic_ops) {
    std::regex logic_split(R"(\s+(AND|OR)\s+)", std::regex::icase);
    std::sregex_token_iterator token_iter(cond.begin(), cond.end(), logic_split, { -1, 1 });
    std::sregex_token_iterator end;
    bool is_logic = false;
    for (; token_iter != end; ++token_iter) {
        std::string token = token_iter->str();
        if (is_logic) {
            std::transform(token.begin(), token.end(), token.begin(), ::toupper);
            logic_ops.push_back(token);
        }
        else {
            expressions.push_back(token);
        }
        is_logic = !is_logic;
    }
}

// 解析形如 "col >= value" 的表达式
std::tuple<std::string, std::string, std::string> parse_single_condition(const std::string& expr) {
    std::regex expr_regex(R"(^\s*(\w+(?:\.\w+)?)\s*(=|!=|>=|<=|>|<)\s*(.+?)\s*$)");
    std::smatch m;
    if (!std::regex_match(expr, m, expr_regex)) return { "", "", "" };
    return { m[1], m[2], m[3] };
}

// 合并多个索引查找结果
std::vector<std::unordered_map<std::string, std::string>> merge_index_results(
    const std::vector<std::vector<std::unordered_map<std::string, std::string>>>& results,
    const std::vector<std::string>& logic_ops
) {
    if (results.empty()) return {};
    auto merged = results[0];
    for (size_t i = 1; i < results.size(); ++i) {
        std::set<std::string> keys1, keys2;
        for (const auto& r : merged) keys1.insert(r.at("__pk"));
        for (const auto& r : results[i]) keys2.insert(r.at("__pk"));
        std::set<std::string> result_keys;
        if (logic_ops[i - 1] == "AND") {
            std::set_intersection(keys1.begin(), keys1.end(), keys2.begin(), keys2.end(),
                std::inserter(result_keys, result_keys.begin()));
        }
        else {
            std::set_union(keys1.begin(), keys1.end(), keys2.begin(), keys2.end(),
                std::inserter(result_keys, result_keys.begin()));
        }
        std::map<std::string, std::unordered_map<std::string, std::string>> pk_to_rec;
        for (const auto& r : merged) if (result_keys.count(r.at("__pk"))) pk_to_rec[r.at("__pk")] = r;
        for (const auto& r : results[i]) if (result_keys.count(r.at("__pk"))) pk_to_rec[r.at("__pk")] = r;
        merged.clear();
        for (const auto& [_, rec] : pk_to_rec) merged.push_back(rec);
    }
    return merged;
}

// 检查未使用索引的表达式是否满足
bool check_remaining_conditions(
    const std::unordered_map<std::string, std::string>& rec,
    const std::vector<std::string>& expressions,
    const std::vector<size_t>& fallback_indices,
    const std::vector<std::string>& logic_ops,
    const Record& checker,
    bool use_prefix
) {
    if (fallback_indices.empty()) return true;
    bool result = Record::evaluate_fallback_conditions(rec, use_prefix, fallback_indices, expressions, logic_ops);
    return result;
}

bool Record::evaluate_fallback_conditions(
    const std::unordered_map<std::string, std::string>& rec,
    bool use_prefix,
    const std::vector<size_t>& fallback_indices,
    const std::vector<std::string>& expressions,
    const std::vector<std::string>& logic_ops)
{
    if (fallback_indices.empty()) return true;

    bool result = evaluate_single_expression(rec, expressions[fallback_indices[0]], use_prefix);

    for (size_t i = 1; i < fallback_indices.size(); ++i) {
        bool current = evaluate_single_expression(rec, expressions[fallback_indices[i]], use_prefix);
        const std::string& logic = logic_ops[fallback_indices[i] - 1]; // 与前一表达式的逻辑关系

        if (logic == "AND") {
            result = result && current;
        }
        else if (logic == "OR") {
            result = result || current;
        }
    }

    return result;
}

bool evaluate_single_expression(
    const std::unordered_map<std::string, std::string>& rec,
    const std::string& expression,
    bool use_prefix)
{
    std::string field, op, value;
    if (!Record::can_use_index(expression, field, value, op)) return false;

    auto it = rec.find(field);
    if (it == rec.end()) return false;

    std::string record_val = it->second;
    if (use_prefix && record_val.size() >= value.size()) {
        record_val = record_val.substr(0, value.size());
    }

    try {
        double a = std::stod(record_val);
        double b = std::stod(value);
        if (op == "=") return a == b;
        if (op == "!=") return a != b;
        if (op == ">") return a > b;
        if (op == "<") return a < b;
        if (op == ">=") return a >= b;
        if (op == "<=") return a <= b;
    }
    catch (...) {
        if (op == "=") return record_val == value;
        if (op == "!=") return record_val != value;
    }

    return false;
}


// 修改版 select 方法
std::vector<Record> Record::select(
    const std::string& columns,
    const std::string& table_name,
    const std::string& condition,
    const std::string& group_by,
    const std::string& order_by,
    const std::string& having,
    const JoinInfo* join_info)
{
    std::vector<Record> records;
    std::vector<std::unordered_map<std::string, std::string>> filtered;
    std::unordered_map<std::string, std::string> combined_structure;

    if (join_info && !join_info->tables.empty()) {
        // 读第一个表
        std::vector<std::unordered_map<std::string, std::string>> result = read_records(join_info->tables[0]);
        for (auto& rec : result) {
            std::unordered_map<std::string, std::string> prefixed;
            for (const auto& [k, v] : rec) {
                std::string full_key = join_info->tables[0] + "." + k;
                prefixed[full_key] = v;
                combined_structure[full_key] = read_table_structure_static(join_info->tables[0]).at(k);
            }
            rec = prefixed;
        }

        // 连接其他表
        for (size_t i = 1; i < join_info->tables.size(); ++i) {
            std::vector<std::unordered_map<std::string, std::string>> right_records = read_records(join_info->tables[i]);
            for (auto& rec : right_records) {
                std::unordered_map<std::string, std::string> prefixed;
                for (const auto& [k, v] : rec) {
                    std::string full_key = join_info->tables[i] + "." + k;
                    prefixed[full_key] = v;
                    combined_structure[full_key] = read_table_structure_static(join_info->tables[i]).at(k);
                }
                rec = prefixed;
            }

            // 正确连接（每次只使用相关的 JoinPair）
            std::vector<std::unordered_map<std::string, std::string>> new_result;
            for (const auto& r1 : result) {
                for (const auto& r2 : right_records) {
                    bool match = true;
                    for (const auto& join : join_info->joins) {
                        bool relevant =
                            (join.left_table == join_info->tables[i - 1] && join.right_table == join_info->tables[i]) ||
                            (join.left_table == join_info->tables[i] && join.right_table == join_info->tables[i - 1]);
                        if (!relevant) continue;

                        for (const auto& [left_col, right_col] : join.conditions) {
                            std::string left_field = join.left_table + "." + left_col;
                            std::string right_field = join.right_table + "." + right_col;
                            if (r1.find(left_field) == r1.end() || r2.find(right_field) == r2.end() ||
                                r1.at(left_field) != r2.at(right_field)) {
                                match = false;
                                break;
                            }
                        }
                        if (!match) break;
                    }
                    if (match) {
                        auto combined = r1;
                        combined.insert(r2.begin(), r2.end());
                        new_result.push_back(combined);
                    }
                }
            }
            result = new_result;
        }
        filtered = result;
    }
    else {
        if (!table_exists(table_name)) {
            throw std::runtime_error("表 '" + table_name + "' 不存在。");
        }
        filtered = read_records(table_name);
        combined_structure = read_table_structure_static(table_name);
    }

    // 处理 WHERE 条件
    Record temp;
    temp.set_table_name(table_name);
    temp.table_structure = combined_structure;
    if (!condition.empty()) temp.parse_condition(condition);

    std::vector<std::string> expressions;
    std::vector<std::string> logic_ops;
    std::vector<std::unordered_map<std::string, std::string>> condition_filtered;

    if (condition.empty()) {
        // 没有 WHERE 条件，直接使用所有记录
        condition_filtered = filtered;
    }
    else {
        // 有 WHERE 条件，执行解析 + 条件筛选逻辑
        parse_condition_expressions(condition, expressions, logic_ops);

        std::vector<std::vector<std::unordered_map<std::string, std::string>>> index_results;
        std::vector<size_t> fallback_indices;

        for (size_t i = 0; i < expressions.size(); ++i) {
            auto [col, op, val] = parse_single_condition(expressions[i]);
            if (!col.empty()) {
                if (has_index(table_name, col)) {
                    index_results.push_back(read_by_index(table_name, col, op, val));
                }
                else {
                    fallback_indices.push_back(i);
                }
            }
        }

        std::vector<std::unordered_map<std::string, std::string>> base =
            index_results.empty() ? filtered : merge_index_results(index_results, logic_ops);

        for (const auto& rec : base) {
            if (check_remaining_conditions(rec, expressions, fallback_indices, logic_ops, temp, join_info && !join_info->tables.empty())) {
                condition_filtered.push_back(rec);
            }
        }
    }

    // 处理 SELECT 字段
    std::vector<std::string> selected_cols;
    if (columns == "*") {
        if (!condition_filtered.empty()) {
            for (const auto& [k, _] : condition_filtered[0]) {
                selected_cols.push_back(k);
            }
        }
    }
    else {
        selected_cols = parse_column_list(columns);
    }

    // 输出最终结果
    for (const auto& rec_map : condition_filtered) {
        Record rec;
        rec.set_table_name(table_name);
        for (const auto& col : selected_cols) {
            auto it = rec_map.find(col);
            std::string val = (it != rec_map.end()) ? it->second : "NULL";

            // 判断布尔字段并转换显示值
            auto type_it = combined_structure.find(col);
            if (type_it == combined_structure.end() && !join_info && !rec_map.empty()) {
                // 去掉前缀再查找（支持非前缀匹配）
                for (const auto& [k, t] : combined_structure) {
                    size_t dot_pos = k.find('.');
                    std::string suffix = (dot_pos != std::string::npos) ? k.substr(dot_pos + 1) : k;
                    if (suffix == col) {
                        type_it = combined_structure.find(k);
                        break;
                    }
                }
            }

            if (type_it != combined_structure.end() && type_it->second == "BOOL") {
                if (val == "1") val = "true";
                else if (val == "0") val = "false";
            }

            rec.add_column(col);
            rec.add_value(val);
        }
        records.push_back(rec);
    }


    return records;
}
