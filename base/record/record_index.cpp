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

// 判断是否可以使用索引条件并提取出字段、操作符和值
bool Record::can_use_index(const std::string& condition, std::string& field_out, std::string& value_out, std::string& op_out) {
    std::regex simple_expr(R"(^\s*(\w+)\s*(=|!=|>|<|>=|<=)\s*'?(\w+)'?\s*$)");
    std::smatch match;
    if (std::regex_match(condition, match, simple_expr)) {
        field_out = match[1];
        op_out = match[2];
        value_out = match[3];
        return true;
    }
    return false;
}

// 尝试使用索引优化的查找逻辑
std::vector<std::unordered_map<std::string, std::string>> Record::try_index_select(
    const std::string& table_name,
    const std::string& condition) {

    std::string field, value, op;
    std::vector<std::unordered_map<std::string, std::string>> result;

    if (!can_use_index(condition, field, value, op)) {
        return {};
    }

    // 读取索引文件名（根据字段）
    std::string index_file = table_name + ".index." + field;
    if (!file_exists(index_file)) return {};

    auto index_map = read_index_map(index_file); // 读取为 multimap 或 map，见下

    if (op == "=") {
        auto range = index_map.equal_range(value);
        for (auto it = range.first; it != range.second; ++it) {
            result.push_back(read_record_by_index(table_name, it->second));
        }
    }
    else if (op == ">") {
        for (auto it = index_map.upper_bound(value); it != index_map.end(); ++it) {
            result.push_back(read_record_by_index(table_name, it->second));
        }
    }
    else if (op == ">=") {
        for (auto it = index_map.lower_bound(value); it != index_map.end(); ++it) {
            result.push_back(read_record_by_index(table_name, it->second));
        }
    }
    else if (op == "<") {
        for (auto it = index_map.begin(); it != index_map.lower_bound(value); ++it) {
            result.push_back(read_record_by_index(table_name, it->second));
        }
    }
    else if (op == "<=") {
        for (auto it = index_map.begin(); it != index_map.upper_bound(value); ++it) {
            result.push_back(read_record_by_index(table_name, it->second));
        }
    }

    return result;
}


std::map<std::string, int> read_index_map(const std::string& filename) {
    std::ifstream fin(filename);
    std::map<std::string, int> index_map;
    std::string key;
    int offset;
    while (fin >> key >> offset) {
        index_map[key] = offset;
    }
    return index_map;
}

bool file_exists(const std::string& filename) {
    std::ifstream f(filename);
    return f.good();
}

std::unordered_map<std::string, std::string> read_record_by_index(const std::string& table_name, int offset) {
    // 自行根据数据存储结构实现偏移读取
    // 示例代码假设每条记录固定大小 or 使用 "," 分割字段
    std::ifstream fin(table_name + ".data");
    fin.seekg(offset);
    std::string line;
    std::getline(fin, line);

    std::unordered_map<std::string, std::string> record;
    auto structure = Record::read_table_structure_static(table_name);
    auto fields = get_fields_from_line(line);
    int i = 0;
    for (const auto& [col, type] : structure) {
        record[col] = (i < fields.size() ? fields[i++] : "NULL");
    }
    return record;
}

std::vector<std::string> get_fields_from_line(const std::string& line) {
    std::stringstream ss(line);
    std::string item;
    std::vector<std::string> result;
    while (std::getline(ss, item, ',')) {
        result.push_back(item);
    }
    return result;
}

std::vector<std::unordered_map<std::string, std::string>>Record::read_by_index(const std::string& table_name,
    const std::string& column,
    const std::string& op,
    const std::string& value)
{
    // 假设你有 read_indexed_records(column, op, value) 实现，这里调用它
    // 否则 fallback 到 read_records 然后过滤（这里是占位逻辑）
    std::vector<std::unordered_map<std::string, std::string>> all = read_records(table_name);

    std::vector<std::unordered_map<std::string, std::string>> result;
    for (const auto& rec : all) {
        auto it = rec.find(column);
        if (it == rec.end()) continue;
        const std::string& val = it->second;

        try {
            double a = std::stod(val);
            double b = std::stod(value);

            if ((op == "=" && a == b) ||
                (op == ">" && a > b) ||
                (op == "<" && a < b) ||
                (op == ">=" && a >= b) ||
                (op == "<=" && a <= b)) {
                result.push_back(rec);
            }
        }
        catch (...) {
            if (op == "=" && val == value) {
                result.push_back(rec);
            }
        }
    }
    return result;
}

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

