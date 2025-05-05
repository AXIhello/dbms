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


// 修改版 select 方法
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
    std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> filtered;
    std::unordered_map<std::string, std::string> combined_structure;

    if (join_info && !join_info->tables.empty()) {
        std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> result = read_records(join_info->tables[0]);
        for (auto& [row_id, rec] : result) {
            std::unordered_map<std::string, std::string> prefixed;
            for (const auto& [k, v] : rec) {
                std::string full_key = join_info->tables[0] + "." + k;
                prefixed[full_key] = v;
                combined_structure[full_key] = read_table_structure_static(join_info->tables[0]).at(k);
            }
            rec = std::move(prefixed);
        }

        for (size_t i = 1; i < join_info->tables.size(); ++i) {
            auto right_records = read_records(join_info->tables[i]);
            for (auto& [row_id, rec] : right_records) {
                std::unordered_map<std::string, std::string> prefixed;
                for (const auto& [k, v] : rec) {
                    std::string full_key = join_info->tables[i] + "." + k;
                    prefixed[full_key] = v;
                    combined_structure[full_key] = read_table_structure_static(join_info->tables[i]).at(k);
                }
                rec = std::move(prefixed);
            }

            std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> new_result;
            for (const auto& [row_r1, r1] : result) {
                for (const auto& [row_r2, r2] : right_records) {
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
                        std::unordered_map<std::string, std::string> combined = r1;
                        combined.insert(r2.begin(), r2.end());
                        new_result.emplace_back(0, std::move(combined));
                    }
                }
            }
            result = std::move(new_result);
        }
        filtered = std::move(result);
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
    std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> condition_filtered;

    // 1. 如果没有任何条件，直接返回所有记录
    if (condition.empty()) {
        condition_filtered = filtered;
    }
    else {
        // 2. 拆解表达式
        parse_condition_expressions(condition, expressions, logic_ops);

        std::vector<std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>>> index_results;
        // 注意：这里每一个indexed对应一个表达式的结果
        std::vector<size_t> fallback_indices;

        for (size_t i = 0; i < expressions.size(); ++i) {
            auto [col, op, val] = parse_single_condition(expressions[i]);
            if (!col.empty()) {
                if (has_index(table_name, col)) {
                    auto indexed = read_by_index(table_name, col, op, val);
                    index_results.push_back(indexed); // 每个条件一个结果集
                }
                else {
                    fallback_indices.push_back(i);
                }
            }
        }

        // 4. 用索引合并初步筛选结果（如果有）
        std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> base =
            index_results.empty() ? filtered : merge_index_results(index_results, logic_ops);

        // 5. 对 base 中的数据进一步检查不支持索引的条件
        for (const auto& [row_id, rec] : base) {
            if (check_remaining_conditions(rec, expressions, fallback_indices, logic_ops, temp, join_info && !join_info->tables.empty())) {
                condition_filtered.emplace_back(row_id, rec);
            }
        }
    }

    // 处理 SELECT 字段
    std::vector<std::string> selected_cols;
    if (columns == "*") {
        if (!condition_filtered.empty()) {
            for (const auto& [k, _] : condition_filtered[0].second) {
                selected_cols.push_back(k);
            }
        }
    }
    else {
        selected_cols = parse_column_list(columns);
    }

    for (const auto& [row_id, rec_map] : condition_filtered) {
        Record rec;
        rec.set_table_name(table_name);
        for (const auto& col : selected_cols) {
            auto it = rec_map.find(col);
            std::string val = (it != rec_map.end()) ? it->second : "NULL";

            auto type_it = combined_structure.find(col);
            if (type_it == combined_structure.end() && !join_info && !rec_map.empty()) {
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
                if (val == "1") val = "TRUE";
                else if (val == "0") val = "FALSE";
            }

            rec.add_column(col);
            rec.add_value(val);
        }
        records.push_back(rec);
    }

    return records;
}
