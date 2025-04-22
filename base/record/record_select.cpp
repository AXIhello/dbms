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
    const std::string& having) {

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

    auto eval_having = [&](const std::unordered_map<std::string, std::string>& row) -> bool {
        if (having.empty()) return true;

        std::regex cond_regex(R"((\w+\(.*?\)|\w+)\s*(=|!=|>=|<=|>|<)\s*('[^']*'|\d+(\.\d+)?|0|1))");
        std::regex logic_split(R"(\s+(AND|OR)\s+)", std::regex::icase);

        std::sregex_token_iterator cond_it(having.begin(), having.end(), cond_regex), cond_end;
        std::sregex_token_iterator logic_it(having.begin(), having.end(), logic_split, 1);

        std::vector<std::string> conditions;
        std::vector<std::string> logic_ops;

        for (; cond_it != cond_end; ++cond_it) conditions.push_back(cond_it->str());
        for (; logic_it != cond_end; ++logic_it) logic_ops.push_back(logic_it->str());

        auto eval_single = [&](const std::string& cond) -> bool {
            std::smatch m;
            if (!std::regex_match(cond, m, cond_regex)) return false;

            std::string field = m[1];
            std::string op = m[2];
            std::string val = m[3];

            if (!row.count(field)) return false;
            std::string left = row.at(field);
            std::string right = val;

            // 不去除引号，只按原样比较
            try {
                double l = std::stod(left), r = std::stod(right);
                if (op == "=") return l == r;
                if (op == "!=") return l != r;
                if (op == ">") return l > r;
                if (op == "<") return l < r;
                if (op == ">=") return l >= r;
                if (op == "<=") return l <= r;
            }
            catch (...) {
                if (op == "=") return left == right;
                if (op == "!=") return left != right;
                if (op == ">") return left > right;
                if (op == "<") return left < right;
                if (op == ">=") return left >= right;
                if (op == "<=") return left <= right;
            }
            return false;
            };

        if (conditions.empty()) return true;
        bool result = eval_single(conditions[0]);

        for (size_t i = 0; i < logic_ops.size(); ++i) {
            std::string logic = logic_ops[i];
            std::transform(logic.begin(), logic.end(), logic.begin(), ::toupper);
            bool next = eval_single(conditions[i + 1]);

            if (logic == "AND") result = result && next;
            else if (logic == "OR") result = result || next;
        }

        return result;
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
            if (eval_having(row)) {
                result.push_back(row);
            }
        }
    }
    else if (std::any_of(selected_cols.begin(), selected_cols.end(), [](const std::string& c) {
        return c.find("(") != std::string::npos;
        })) {
        std::unordered_map<std::string, std::string> row;
        for (const auto& col : selected_cols) {
            row[col] = apply_aggregates(col, filtered);
        }
        if (eval_having(row)) {
            result.push_back(row);
        }
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
            rec.add_value(row.count(col) ? row.at(col) : "NULL");
        }
        records.push_back(rec);
    }

    return records;
}

