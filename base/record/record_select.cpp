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
    const JoinInfo* join_info) {

    std::vector<Record> records;
    std::vector<std::unordered_map<std::string, std::string>> filtered;
    std::unordered_map<std::string, std::string> combined_structure;

    if (join_info && !join_info->tables.empty()) {
        // 初始化第一个表
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

        // 处理每一对 join
        for (const auto& join : join_info->joins) {
            std::vector<std::unordered_map<std::string, std::string>> right_records = read_records(join.right_table);
            for (auto& rec : right_records) {
                std::unordered_map<std::string, std::string> prefixed;
                for (const auto& [k, v] : rec) {
                    std::string full_key = join.right_table + "." + k;
                    prefixed[full_key] = v;
                    combined_structure[full_key] = read_table_structure_static(join.right_table).at(k);
                }
                rec = prefixed;
            }

            std::vector<std::unordered_map<std::string, std::string>> new_result;
            for (const auto& r1 : result) {
                for (const auto& r2 : right_records) {
                    bool match = true;
                    for (const auto& [left_field, right_field] : join.conditions) {
                        std::string left_full = join.left_table + "." + left_field;
                        std::string right_full = join.right_table + "." + right_field;

                        if (r1.at(left_full) != r2.at(right_full)) {
                            match = false;
                            break;
                        }
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

    // where筛选
    std::vector<std::unordered_map<std::string, std::string>> condition_filtered;
    for (const auto& rec : filtered) {
        if (condition.empty() || temp.matches_condition(rec, join_info && !join_info->tables.empty())) {
            condition_filtered.push_back(rec);
        }
    }

    // group by分组
    std::map<std::string, std::vector<std::unordered_map<std::string, std::string>>> grouped;
    if (!group_by.empty()) {
        for (const auto& rec : condition_filtered) {
            std::string key = rec.at(group_by);
            grouped[key].push_back(rec);
        }
    }

    std::vector<std::unordered_map<std::string, std::string>> final_result;

    auto apply_aggregates = [](const std::string& col, const std::vector<std::unordered_map<std::string, std::string>>& records) -> std::string {
        std::string field = col.substr(col.find("(") + 1, col.length() - col.find("(") - 2);
        if (records.empty() || records[0].find(field) == records[0].end()) return "NULL";
        if (col.find("COUNT(") == 0) return std::to_string(records.size());
        if (col.find("SUM(") == 0) {
            double sum = 0;
            for (const auto& r : records) sum += std::stod(r.at(field));
            return std::to_string(sum);
        }
        if (col.find("AVG(") == 0) {
            double sum = 0;
            for (const auto& r : records) sum += std::stod(r.at(field));
            return std::to_string(sum / records.size());
        }
        if (col.find("MAX(") == 0) {
            double max_val = std::stod(records[0].at(field));
            for (const auto& r : records) max_val = std::max(max_val, std::stod(r.at(field)));
            return std::to_string(max_val);
        }
        if (col.find("MIN(") == 0) {
            double min_val = std::stod(records[0].at(field));
            for (const auto& r : records) min_val = std::min(min_val, std::stod(r.at(field)));
            return std::to_string(min_val);
        }
        return "";
        };

    std::vector<std::string> selected_cols;
    if (columns == "*") {
        if (!filtered.empty()) {
            for (const auto& [k, _] : filtered[0]) {
                selected_cols.push_back(k);
            }
        }
    }
    else {
        selected_cols = parse_column_list(columns);
    }

    if (!group_by.empty()) {
        for (const auto& [key, group_records] : grouped) {
            std::unordered_map<std::string, std::string> row;
            row[group_by] = key;
            for (const auto& col : selected_cols) {
                if (col == group_by) continue;
                row[col] = apply_aggregates(col, group_records);
            }
            final_result.push_back(row);
        }
    }
    else if (std::any_of(selected_cols.begin(), selected_cols.end(), [](const std::string& c) {
        return c.find("(") != std::string::npos;
        })) {
        std::unordered_map<std::string, std::string> row;
        for (const auto& col : selected_cols) {
            row[col] = apply_aggregates(col, condition_filtered);
        }
        final_result.push_back(row);
    }
    else {
        for (const auto& rec : condition_filtered) {
            std::unordered_map<std::string, std::string> row;
            for (const auto& col : selected_cols) {
                auto it = rec.find(col);
                row[col] = (it != rec.end()) ? it->second : "NULL";
            }
            final_result.push_back(row);
        }
    }

    if (!order_by.empty()) {
        std::string key = order_by;
        bool desc = false;
        if (key.find(" DESC") != std::string::npos) {
            desc = true;
            key = key.substr(0, key.find(" DESC"));
        }
        std::sort(final_result.begin(), final_result.end(), [&](const auto& a, const auto& b) {
            return desc ? a.at(key) > b.at(key) : a.at(key) < b.at(key);
            });
    }

    for (const auto& row : final_result) {
        Record rec;
        rec.set_table_name(table_name);
        for (const auto& col : selected_cols) {
            rec.add_column(col);
            rec.add_value(row.at(col));
        }
        records.push_back(rec);
    }

    return records;
}
