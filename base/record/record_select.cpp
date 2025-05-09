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

    // ==================== 1️⃣  表读取处理 ====================
    std::vector<std::string> tables;
    if (join_info && !join_info->tables.empty()) {
        tables = join_info->tables;
    }
    else {
        if (table_name.find(',') != std::string::npos) {
            std::istringstream ss(table_name);
            std::string table;
            while (std::getline(ss, table, ',')) {
                table.erase(0, table.find_first_not_of(" \t"));
                table.erase(table.find_last_not_of(" \t") + 1);
                tables.push_back(table);
            }
        }
        else {
            tables.push_back(table_name);
        }
    }

    // ==================== 2️⃣  数据读取 ====================
    if (join_info && !join_info->tables.empty()) {
        // 读取第一个表并添加表名前缀
        auto first_table_records = read_records(join_info->tables[0]);
        std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> result;

        for (const auto& [row_id, rec] : first_table_records) {
            std::unordered_map<std::string, std::string> prefixed;
            for (const auto& [k, v] : rec) {
                std::string full_key = join_info->tables[0] + "." + k;
                prefixed[full_key] = v;
                combined_structure[full_key] = read_table_structure_static(join_info->tables[0]).at(k);
            }
            result.emplace_back(row_id, std::move(prefixed));
        }

        // 连接其他表
        for (size_t i = 1; i < join_info->tables.size(); ++i) {
            auto right_table_records = read_records(join_info->tables[i]);
            std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> right_prefixed;

            // 给右表添加前缀
            for (const auto& [row_id, rec] : right_table_records) {
                std::unordered_map<std::string, std::string> prefixed;
                for (const auto& [k, v] : rec) {
                    std::string full_key = join_info->tables[i] + "." + k;
                    prefixed[full_key] = v;
                    combined_structure[full_key] = read_table_structure_static(join_info->tables[i]).at(k);
                }
                right_prefixed.emplace_back(row_id, std::move(prefixed));
            }

            // 执行JOIN操作
            std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> new_result;
            for (const auto& [row_r1, r1] : result) {
                for (const auto& [row_r2, r2] : right_prefixed) {
                    bool match = true;

                    // 查找并应用相关的JOIN条件
                    for (const auto& join : join_info->joins) {
                        bool relevant =
                            (join.left_table == join_info->tables[i - 1] && join.right_table == join_info->tables[i]) ||
                            (join.left_table == join_info->tables[i] && join.right_table == join_info->tables[i - 1]);

                        if (!relevant) continue;

                        for (const auto& [left_col, right_col] : join.conditions) {
                            std::string left_field = join.left_table + "." + left_col;
                            std::string right_field = join.right_table + "." + right_col;

                            auto left_it = r1.find(left_field);
                            auto right_it = r2.find(right_field);

                            if (left_it == r1.end() || right_it == r2.end() ||
                                left_it->second != right_it->second) {
                                match = false;
                                break;
                            }
                        }
                        if (!match) break;
                    }

                    if (match) {
                        auto combined = r1;
                        combined.insert(r2.begin(), r2.end());
                        new_result.emplace_back(0, std::move(combined));
                    }
                }
            }
            result = std::move(new_result);
        }
        filtered = std::move(result);
    }
    else if (tables.size() == 1) {
        // 单表查询
        if (!table_exists(tables[0])) {
            throw std::runtime_error("表 '" + tables[0] + "' 不存在。");
        }
        filtered = read_records(tables[0]);
        combined_structure = read_table_structure_static(tables[0]);
    }
    else {
        // 隐式连接处理
        auto first_table_records = read_records(tables[0]);
        combined_structure = read_table_structure_static(tables[0]);
        filtered = std::move(first_table_records);

        for (size_t i = 1; i < tables.size(); ++i) {
            if (!table_exists(tables[i])) {
                throw std::runtime_error("表 '" + tables[i] + "' 不存在。");
            }

            auto right_structure = read_table_structure_static(tables[i]);
            auto right_records = read_records(tables[i]);

            // 添加前缀
            std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> right_prefixed;
            for (const auto& [row_id, rec] : right_records) {
                std::unordered_map<std::string, std::string> prefixed;
                for (const auto& [k, v] : rec) {
                    std::string full_key = tables[i] + "." + k;
                    prefixed[full_key] = v;
                    combined_structure[full_key] = right_structure.at(k);
                }
                right_prefixed.emplace_back(row_id, std::move(prefixed));
            }

            // 执行连接
            std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> new_result;
            for (const auto& [row_r1, r1] : filtered) {
                for (const auto& [row_r2, r2] : right_prefixed) {
                    auto combined = r1;
                    combined.insert(r2.begin(), r2.end());
                    new_result.emplace_back(0, std::move(combined));
                }
            }
            filtered = std::move(new_result);

        }
    }

    // ==================== 3️⃣  WHERE 过滤 ====================
    Record temp;
    temp.set_table_name(tables.size() == 1 ? tables[0] : "");
    temp.table_structure = combined_structure;
    if (!condition.empty()) temp.parse_condition(condition);

    std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> condition_filtered;
    for (const auto& [row_id, rec] : filtered) {
        if (condition.empty() || temp.matches_condition(rec, join_info != nullptr || tables.size() > 1)) {
            condition_filtered.emplace_back(row_id, rec);
        }
    }

    // ==================== 4️⃣  GROUP BY 和 聚合函数 ====================
    if (!group_by.empty()) {
        // 按组分类记录
        std::map<std::string, std::vector<std::unordered_map<std::string, std::string>>> grouped;
        for (const auto& [_, rec] : condition_filtered) {
            auto it = rec.find(group_by);
            if (it == rec.end()) {
                throw std::runtime_error("GROUP BY 字段 '" + group_by + "' 不存在于某条记录中");
            }
            std::string key = it->second;
            grouped[key].push_back(rec);
        }

        // 定义应用聚合函数的lambda表达式
        auto apply_aggregates = [](const std::string& col, const std::vector<std::unordered_map<std::string, std::string>>& records) -> std::string {
            // 如果不是聚合函数，直接返回空
            if (col.find("(") == std::string::npos) return "";

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
                    if (it != r.end() && it->second != "NULL")
                        sum += std::stod(it->second);
                }
                return std::to_string(sum);
            }
            else if (col.find("AVG(") == 0) {
                double sum = 0;
                int count = 0;
                for (const auto& r : records) {
                    auto it = r.find(field);
                    if (it != r.end() && it->second != "NULL") {
                        sum += std::stod(it->second);
                        count++;
                    }
                }
                return count > 0 ? std::to_string(sum / count) : "NULL";
            }
            else if (col.find("MAX(") == 0) {
                bool first = true;
                double max_val = 0;
                for (const auto& r : records) {
                    auto it = r.find(field);
                    if (it != r.end() && it->second != "NULL") {
                        double current = std::stod(it->second);
                        if (first || current > max_val) {
                            max_val = current;
                            first = false;
                        }
                    }
                }
                return first ? "NULL" : std::to_string(max_val);
            }
            else if (col.find("MIN(") == 0) {
                bool first = true;
                double min_val = 0;
                for (const auto& r : records) {
                    auto it = r.find(field);
                    if (it != r.end() && it->second != "NULL") {
                        double current = std::stod(it->second);
                        if (first || current < min_val) {
                            min_val = current;
                            first = false;
                        }
                    }
                }
                return first ? "NULL" : std::to_string(min_val);
            }
            return "NULL";
            };

        // 清空之前的记录并添加分组后的结果
        condition_filtered.clear();
        for (const auto& [group_key, group_records] : grouped) {
            std::unordered_map<std::string, std::string> grouped_row;
            grouped_row[group_by] = group_key;

            // 检查选择的列
            std::vector<std::string> agg_cols;
            if (columns == "*") {
                for (const auto& [col, _] : combined_structure) {
                    agg_cols.push_back(col);
                }
            }
            else {
                agg_cols = parse_column_list(columns);
            }

            // 应用聚合函数到每一列
            for (const auto& col : agg_cols) {
                if (col == group_by) continue; // 分组字段直接使用
                if (col.find("(") != std::string::npos) {
                    grouped_row[col] = apply_aggregates(col, group_records);
                }
            }

            condition_filtered.emplace_back(0, grouped_row);
        }
    }

    // ==================== 5️⃣  HAVING 过滤 ====================
    if (!having.empty()) {
        // 定义一个lambda函数用于评估HAVING条件
        auto eval_having = [&](const std::unordered_map<std::string, std::string>& row) -> bool {
            std::regex pattern(R"(\s*(\w+\(.*\)|\w+)\s*(=|!=|>|<|>=|<=)\s*(\S+))");
            std::smatch match;
            if (!std::regex_match(having, match, pattern)) return true;

            std::string col = match[1];
            std::string op = match[2];
            std::string rhs = match[3];
            if (rhs.front() == '\'' && rhs.back() == '\'') {
                rhs = rhs.substr(1, rhs.length() - 2);
            }

            if (!row.count(col)) return false;
            std::string lhs = row.at(col);

            try {
                double lnum = std::stod(lhs);
                double rnum = std::stod(rhs);
                if (op == "=") return lnum == rnum;
                if (op == "!=") return lnum != rnum;
                if (op == ">") return lnum > rnum;
                if (op == "<") return lnum < rnum;
                if (op == ">=") return lnum >= rnum;
                if (op == "<=") return lnum <= rnum;
            }
            catch (...) {
                if (op == "=") return lhs == rhs;
                if (op == "!=") return lhs != rhs;
                if (op == ">") return lhs > rhs;
                if (op == "<") return lhs < rhs;
                if (op == ">=") return lhs >= rhs;
                if (op == "<=") return lhs <= rhs;
            }

            return true;
            };

        // 应用HAVING条件过滤记录
        auto it = std::remove_if(condition_filtered.begin(), condition_filtered.end(),
            [&](const auto& pair) { return !eval_having(pair.second); });
        condition_filtered.erase(it, condition_filtered.end());
    }

    // ==================== 6️⃣  ORDER BY 排序 ====================
    if (!order_by.empty()) {
        std::string key = order_by;
        bool desc = false;
        if (key.find(" DESC") != std::string::npos) {
            desc = true;
            key = key.substr(0, key.find(" DESC"));
        }

        std::sort(condition_filtered.begin(), condition_filtered.end(),
            [&](const auto& a, const auto& b) {
                auto a_it = a.second.find(key);
                auto b_it = b.second.find(key);

                // 处理NULL值
                if (a_it == a.second.end() || a_it->second == "NULL") {
                    return false; // NULL值放在最后
                }
                if (b_it == b.second.end() || b_it->second == "NULL") {
                    return true;  // NULL值放在最后
                }

                // 尝试数值比较
                try {
                    double a_val = std::stod(a_it->second);
                    double b_val = std::stod(b_it->second);
                    return desc ? a_val > b_val : a_val < b_val;
                }
                catch (...) {
                    // 字符串比较
                    return desc ? a_it->second > b_it->second : a_it->second < b_it->second;
                }
            });
    }

    // ==================== 7️⃣  构建结果集 ====================
    std::vector<std::string> selected_cols; // 先声明向量
    if (columns == "*") {
        // 如果有条件过滤后的记录，从第一条记录获取所有列名
        if (!condition_filtered.empty()) {
            for (const auto& [col, _] : condition_filtered[0].second) {
                selected_cols.push_back(col);
            }
            // 确保列名按顺序排列，提高输出一致性
            std::sort(selected_cols.begin(), selected_cols.end());
        }
        // 如果没有记录，则从表结构中获取列名
        else {
            for (const auto& pair : combined_structure) {
                selected_cols.push_back(pair.first);
            }
        }
    }
    else {
        // 如果不是 SELECT *，则解析 columns 字符串
        selected_cols = parse_column_list(columns);
    }

    for (const auto& [_, rec_map] : condition_filtered) {
        Record rec;
        rec.set_table_name(tables.size() == 1 ? tables[0] : "");
        for (const auto& col : selected_cols) {
            auto it = rec_map.find(col);
            std::string val = (it != rec_map.end()) ? it->second : "NULL";
            rec.add_column(col);
            rec.add_value(val);
        }
        records.push_back(rec);
    }

    return records;
}
