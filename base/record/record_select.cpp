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

    std::vector<std::string> tables;
    if (join_info && !join_info->tables.empty()) {
        tables = join_info->tables;
    }
    else {
        std::stringstream ss(table_name);
        std::string token;
        while (std::getline(ss, token, ',')) {
            token.erase(0, token.find_first_not_of(" \t"));
            token.erase(token.find_last_not_of(" \t") + 1);
            tables.push_back(token);
        }
    }

    if (tables.size() == 1) {
        if (!table_exists(tables[0])) {
            throw std::runtime_error("表 '" + tables[0] + "' 不存在。");
        }
        filtered = read_records(tables[0]);
    }
    else {
        std::vector<std::unordered_map<std::string, std::string>> result = read_records(tables[0]);
        for (auto& rec : result) {
            std::unordered_map<std::string, std::string> prefixed;
            for (const auto& [k, v] : rec) {
                prefixed[tables[0] + "." + k] = v;
            }
            rec = prefixed;
        }

        for (size_t i = 1; i < tables.size(); ++i) {
            std::string curr_table = tables[i];
            std::vector<std::unordered_map<std::string, std::string>> curr_records = read_records(curr_table);
            for (auto& rec : curr_records) {
                std::unordered_map<std::string, std::string> prefixed;
                for (const auto& [k, v] : rec) {
                    prefixed[curr_table + "." + k] = v;
                }
                rec = prefixed;
            }

            std::vector<std::unordered_map<std::string, std::string>> new_result;
            for (const auto& r1 : result) {
                for (const auto& r2 : curr_records) {
                    bool match = true;
                    if (join_info) {
                        for (const auto& [left, right] : join_info->join_conditions) {
                            if (r1.at(left) != r2.at(right)) {
                                match = false;
                                break;
                            }
                        }
                    }
                    if (match) {
                        std::unordered_map<std::string, std::string> combined = r1;
                        combined.insert(r2.begin(), r2.end());
                        new_result.push_back(combined);
                    }
                }
            }
            result = new_result;
        }

        filtered = result;
    }

    Record temp;
    if (!tables.empty()) temp.set_table_name(tables[0]);
    temp.parse_condition(condition);

    std::vector<std::unordered_map<std::string, std::string>> condition_filtered;
    for (const auto& rec : filtered) {
        if (condition.empty() || temp.matches_condition(rec)) {
            condition_filtered.push_back(rec);
        }
    }

    std::map<std::string, std::vector<std::unordered_map<std::string, std::string>>> grouped;
    if (!group_by.empty()) {
        for (const auto& rec : condition_filtered) {
            std::string key = rec.at(group_by);
            grouped[key].push_back(rec);
        }
    }

    std::vector<std::unordered_map<std::string, std::string>> result;

    auto apply_aggregates = [](const std::string& col, const std::vector<std::unordered_map<std::string, std::string>>& records) -> std::string {
        std::string field = col.substr(col.find("(") + 1, col.length() - col.find("(") - 2);
        if (records.empty() || records[0].find(field) == records[0].end()) {
            return "NULL";
        }
        if (col.find("COUNT(") == 0) {
            return std::to_string(records.size());
        }
        else if (col.find("SUM(") == 0) {
            double sum = 0;
            for (const auto& r : records) sum += std::stod(r.at(field));
            return std::to_string(sum);
        }
        else if (col.find("AVG(") == 0) {
            double sum = 0;
            for (const auto& r : records) sum += std::stod(r.at(field));
            return std::to_string(sum / records.size());
        }
        else if (col.find("MAX(") == 0) {
            double max_val = std::stod(records[0].at(field));
            for (const auto& r : records) {
                max_val = std::max(max_val, std::stod(r.at(field)));
            }
            return std::to_string(max_val);
        }
        else if (col.find("MIN(") == 0) {
            double min_val = std::stod(records[0].at(field));
            for (const auto& r : records) {
                min_val = std::min(min_val, std::stod(r.at(field)));
            }
            return std::to_string(min_val);
        }
        return "";
        };

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

    if (!group_by.empty()) {
        for (const auto& [key, group_records] : grouped) {
            std::unordered_map<std::string, std::string> row;
            row[group_by] = key;
            for (const auto& col : selected_cols) {
                if (col == group_by) continue;
                row[col] = apply_aggregates(col, group_records);
            }
            if (!having.empty()) {
                temp.parse_condition(having);
                if (!temp.matches_condition(row)) continue;
            }
            result.push_back(row);
        }
    }
    else if (std::any_of(selected_cols.begin(), selected_cols.end(), [](const std::string& c) {
        return c.find("(") != std::string::npos;
        })) {
        std::unordered_map<std::string, std::string> row;
        for (const auto& col : selected_cols) {
            row[col] = apply_aggregates(col, condition_filtered);
        }
        if (having.empty() || (temp.parse_condition(having), temp.matches_condition(row))) {
            result.push_back(row);
        }
    }
    else {
        result = condition_filtered;
    }

    if (!order_by.empty()) {
        std::string key = order_by;
        bool desc = false;
        if (key.find(" DESC") != std::string::npos) {
            desc = true;
            key = key.substr(0, key.find(" DESC"));
        }
        std::sort(result.begin(), result.end(), [&](const auto& a, const auto& b) {
            return desc ? a.at(key) > b.at(key) : a.at(key) < b.at(key);
            });
    }

    for (const auto& row : result) {
        Record rec;
        rec.set_table_name(tables.empty() ? table_name : tables[0]);
        for (const auto& col : selected_cols) {
            rec.add_column(col);
            rec.add_value(row.count(col) ? row.at(col) : "NULL");
        }
        records.push_back(rec);
    }

    return records;
}
