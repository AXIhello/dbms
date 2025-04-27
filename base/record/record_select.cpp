#include "Record.h"
#include "manager/parse.h"
#include "ui/output.h"

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <regex>
#include <map>

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

            // 连接
            std::vector<std::unordered_map<std::string, std::string>> new_result;
            for (const auto& r1 : result) {
                for (const auto& r2 : right_records) {
                    bool match = true;
                    for (const auto& join : join_info->joins) {
                        // 检查连接条件
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

    Record temp;
    temp.set_table_name(table_name);
    temp.table_structure = combined_structure;
    if (!condition.empty()) {
        temp.parse_condition(condition);
    }

    std::vector<std::unordered_map<std::string, std::string>> condition_filtered;
    for (const auto& rec : filtered) {
        if (condition.empty() || temp.matches_condition(rec, join_info && !join_info->tables.empty())) {
            condition_filtered.push_back(rec);
        }
    }

    // 🔥最关键改动！！！选字段必须基于连接后的结果
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

    // 输出
    for (const auto& rec_map : condition_filtered) {
        Record rec;
        rec.set_table_name(table_name);
        for (const auto& col : selected_cols) {
            auto it = rec_map.find(col);
            rec.add_column(col);
            rec.add_value(it != rec_map.end() ? it->second : "NULL");
        }
        records.push_back(rec);
    }

    return records;
}

