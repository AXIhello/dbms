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
    const std::string& table_names,
    const std::string& condition,
    const std::string& group_by,
    const std::string& order_by,
    const std::string& having) {

    std::vector<Record> records;

    // 解析表名（支持多表）
    std::vector<std::string> table_list;
    std::stringstream ss(table_names);
    std::string table;
    while (std::getline(ss, table, ',')) {
        table.erase(0, table.find_first_not_of(" 	"));
        table.erase(table.find_last_not_of(" 	") + 1);
        if (!table_exists(table)) {
            throw std::runtime_error("表 '" + table + "' 不存在。");
        }
        table_list.push_back(table);
    }

    // 读取每个表的字段和记录，带前缀（如果是多表）
    bool is_multi_table = table_list.size() > 1;
    std::vector<std::vector<std::unordered_map<std::string, std::string>>> all_table_records;
    std::unordered_map<std::string, std::string> merged_structure;

    for (const auto& tbl : table_list) {
        std::vector<FieldBlock> fields = read_field_blocks(tbl);
        std::unordered_map<std::string, std::string> structure = read_table_structure_static(tbl);
        std::vector<std::unordered_map<std::string, std::string>> raw = read_records(tbl);

        // 添加前缀
        for (auto& rec : raw) {
            std::unordered_map<std::string, std::string> with_prefix;
            for (const auto& [k, v] : rec) {
                std::string new_key = is_multi_table ? (tbl + "." + k) : k;
                with_prefix[new_key] = v;
            }
            rec = std::move(with_prefix);
        }
        all_table_records.push_back(raw);

        for (const auto& [k, v] : structure) {
            std::string new_key = is_multi_table ? (tbl + "." + k) : k;
            merged_structure[new_key] = v;
        }
    }

    // 笛卡尔积构造组合
    std::function<void(size_t, std::unordered_map<std::string, std::string>, std::vector<std::unordered_map<std::string, std::string>>&)>
        cartesian;
    cartesian = [&](size_t idx, std::unordered_map<std::string, std::string> acc, std::vector<std::unordered_map<std::string, std::string>>& out) {
        if (idx == all_table_records.size()) {
            out.push_back(acc);
            return;
        }
        for (const auto& rec : all_table_records[idx]) {
            auto next = acc;
            next.insert(rec.begin(), rec.end());
            cartesian(idx + 1, next, out);
        }
        };

    std::vector<std::unordered_map<std::string, std::string>> combined_records;
    cartesian(0, {}, combined_records);

    // 设置临时对象用于条件匹配
    Record temp;
    temp.set_table_name(table_list[0]);
    temp.table_structure = merged_structure;
    if (!condition.empty()) {
        temp.parse_condition(condition);
    }

    // 筛选记录
    std::vector<std::unordered_map<std::string, std::string>> filtered;
    for (const auto& rec : combined_records) {
        if (condition.empty() || temp.matches_condition(rec)) {
            filtered.push_back(rec);
        }
    }

    // 分组处理
    std::map<std::string, std::vector<std::unordered_map<std::string, std::string>>> grouped;
    if (!group_by.empty()) {
        for (const auto& rec : filtered) {
            auto it = rec.find(group_by);
            if (it == rec.end()) {
                throw std::runtime_error("GROUP BY 字段 '" + group_by + "' 不存在于某条记录中");
            }
            grouped[it->second].push_back(rec);
        }
    }

    std::vector<std::unordered_map<std::string, std::string>> result;
    auto apply_aggregates = [](const std::string& col, const std::vector<std::unordered_map<std::string, std::string>>& records) -> std::string {
        std::string field = col.substr(col.find("(") + 1, col.length() - col.find("(") - 2);
        if (records.empty() || records[0].find(field) == records[0].end()) return "NULL";

        if (col.find("COUNT(") == 0) {
            return std::to_string(records.size());
        }
        else if (col.find("SUM(") == 0) {
            double sum = 0; for (const auto& r : records) sum += std::stod(r.at(field));
            return std::to_string(sum);
        }
        else if (col.find("AVG(") == 0) {
            double sum = 0; for (const auto& r : records) sum += std::stod(r.at(field));
            return std::to_string(sum / records.size());
        }
        else if (col.find("MAX(") == 0) {
            double max_val = std::stod(records[0].at(field));
            for (const auto& r : records) max_val = std::max(max_val, std::stod(r.at(field)));
            return std::to_string(max_val);
        }
        else if (col.find("MIN(") == 0) {
            double min_val = std::stod(records[0].at(field));
            for (const auto& r : records) min_val = std::min(min_val, std::stod(r.at(field)));
            return std::to_string(min_val);
        }
        return "";
        };

    std::vector<std::string> selected_cols;
    if (columns == "*") {
        for (const auto& [col, _] : merged_structure) {
            selected_cols.push_back(col);
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
            result.push_back(row);
        }
    }
    else if (std::any_of(selected_cols.begin(), selected_cols.end(), [](const std::string& c) {
        return c.find("(") != std::string::npos;
        })) {
        std::unordered_map<std::string, std::string> row;
        for (const auto& col : selected_cols) {
            row[col] = apply_aggregates(col, filtered);
        }
        result.push_back(row);
    }
    else {
        for (const auto& rec : filtered) {
            std::unordered_map<std::string, std::string> row;
            for (const auto& col : selected_cols) {
                row[col] = rec.count(col) ? rec.at(col) : "NULL";
            }
            result.push_back(row);
        }
    }

    if (!order_by.empty()) {
        std::string key = order_by;
        bool desc = false;
        if (key.find(" DESC") != std::string::npos) {
            desc = true;
            key = key.substr(0, key.find(" DESC"));
        }

        std::sort(result.begin(), result.end(), [&](const auto& a, const auto& b) {
            if (!a.count(key) || !b.count(key)) return false;
            return desc ? a.at(key) > b.at(key) : a.at(key) < b.at(key);
            });
    }

    // HAVING 支持
    if (!having.empty()) {
        Record having_temp;
        having_temp.set_table_name(table_list[0]);
        having_temp.table_structure = merged_structure;
        having_temp.parse_condition(having);
        std::vector<std::unordered_map<std::string, std::string>> filtered_result;
        for (const auto& row : result) {
            if (having_temp.matches_condition(row)) {
                filtered_result.push_back(row);
            }
        }
        result = std::move(filtered_result);
    }

    for (const auto& row : result) {
        Record rec;
        rec.set_table_name(table_list[0]);
        for (const auto& col : selected_cols) {
            rec.add_column(col);
            rec.add_value(row.count(col) ? row.at(col) : "NULL");
        }
        records.push_back(rec);
    }

    return records;
}
