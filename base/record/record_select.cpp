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
    const std::string& order_by) {

    std::vector<Record> records;

    if (!table_exists(table_name)) {
        throw std::runtime_error("表 '" + table_name + "' 不存在。");
    }

    std::vector<FieldBlock> fields = read_field_blocks(table_name);
    std::unordered_map<std::string, std::string> structure = read_table_structure_static(table_name);
    std::vector<std::unordered_map<std::string, std::string>> raw_records = read_records(table_name);

    Record temp;
    temp.set_table_name(table_name);
    temp.table_structure = structure;
    if (!condition.empty()) {
        temp.parse_condition(condition);
    }

    std::vector<std::unordered_map<std::string, std::string>> filtered;
    for (const auto& rec : raw_records) {
        if (condition.empty() || temp.matches_condition(rec)) {
            filtered.push_back(rec);
        }
    }

    std::map<std::string, std::vector<std::unordered_map<std::string, std::string>>> grouped;
    if (!group_by.empty()) {
        for (const auto& rec : filtered) {
            auto it = rec.find(group_by);
            if (it == rec.end()) {
                throw std::runtime_error("GROUP BY 字段 '" + group_by + "' 不存在于某条记录中");
            }
            std::string key = it->second;
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
            for (const auto& r : records) {
                auto it = r.find(field);
                if (it != r.end()) sum += std::stod(it->second);
            }
            return std::to_string(sum);
        }
        else if (col.find("AVG(") == 0) {
            double sum = 0;
            for (const auto& r : records) {
                auto it = r.find(field);
                if (it != r.end()) sum += std::stod(it->second);
            }
            return std::to_string(sum / records.size());
        }
        else if (col.find("MAX(") == 0) {
            double max_val = std::stod(records[0].at(field));
            for (const auto& r : records) {
                auto it = r.find(field);
                if (it != r.end()) max_val = std::max(max_val, std::stod(it->second));
            }
            return std::to_string(max_val);
        }
        else if (col.find("MIN(") == 0) {
            double min_val = std::stod(records[0].at(field));
            for (const auto& r : records) {
                auto it = r.find(field);
                if (it != r.end()) min_val = std::min(min_val, std::stod(it->second));
            }
            return std::to_string(min_val);
        }
        return "";
        };

    std::vector<std::string> selected_cols;
    if (columns == "*") {
        for (const auto& field : fields) {
            selected_cols.push_back(field.name);
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
                auto it = rec.find(col);
                row[col] = (it != rec.end()) ? it->second : "NULL";
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

    for (const auto& row : result) {
        Record rec;
        rec.set_table_name(table_name);
        for (const auto& col : selected_cols) {
            rec.add_column(col);
            auto it = row.find(col);
            rec.add_value(it != row.end() ? it->second : "NULL");
        }

        records.push_back(rec);
    }

    return records;
}
