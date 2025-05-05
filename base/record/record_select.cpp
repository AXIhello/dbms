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
        // 读第一个表
        std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> result = read_records(join_info->tables[0]);
        for (auto& [row_id, rec] : result) {
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
            std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> right_records = read_records(join_info->tables[i]);
            for (auto& [row_id, rec] : right_records) {
                std::unordered_map<std::string, std::string> prefixed;
                for (const auto& [k, v] : rec) {
                    std::string full_key = join_info->tables[i] + "." + k;
                    prefixed[full_key] = v;
                    combined_structure[full_key] = read_table_structure_static(join_info->tables[i]).at(k);
                }
                rec = prefixed;
            }

            // 正确连接（每次只使用相关的 JoinPair）
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
                        new_result.emplace_back(0, std::move(combined)); // 合并后的记录 row_id 设置为 0（可自定义）
                    }
                }
            }
            result = std::move(new_result);
            filtered = result;
        }
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

    if (condition.empty()) {
        // 没有 WHERE 条件，直接使用所有记录
        condition_filtered = filtered;
    }
    else {
        // 有 WHERE 条件，执行解析 + 条件筛选逻辑
        parse_condition_expressions(condition, expressions, logic_ops);

        std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> index_results;
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

        // 这里需要修改，因为现在index_results包含的是pair<uint64_t, unordered_map>
        std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> base =
            index_results.empty() ? filtered : merge_index_results(index_results, logic_ops);

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
            // 修改：现在需要访问pair的第二个元素
            for (const auto& [k, _] : condition_filtered[0].second) {
                selected_cols.push_back(k);
            }
        }
    }
    else {
        selected_cols = parse_column_list(columns);
    }

    // 输出最终结果
    for (const auto& [row_id, rec_map] : condition_filtered) {
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
