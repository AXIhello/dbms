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

// 修改版 select 方法，支持隐式连接
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

    // 检查是否包含隐式连接（逗号分隔的表名）
    bool has_implicit_join = table_name.find(',') != std::string::npos;
    std::vector<std::string> tables;

    if (has_implicit_join) {
        // 解析逗号分隔的表名
        std::istringstream ss(table_name);
        std::string table;
        while (std::getline(ss, table, ',')) {
            // 去除前后空格
            table.erase(0, table.find_first_not_of(" \t"));
            table.erase(table.find_last_not_of(" \t") + 1);
            if (!table.empty()) {
                tables.push_back(table);
            }
        }
    }

    if (join_info && !join_info->tables.empty()) {
        // 处理显式JOIN的代码保持不变
        std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> result = read_records(join_info->tables[0]);
        for (auto& [row_id, rec] : result) {
            std::unordered_map<std::string, std::string> prefixed;
            auto table_structure = read_table_structure_static(join_info->tables[0]);

            for (const auto& [k, v] : rec) {
                std::string full_key = join_info->tables[0] + "." + k;
                prefixed[full_key] = v;

                // 使用find代替at，检查键是否存在
                auto structure_it = table_structure.find(k);
                if (structure_it != table_structure.end()) {
                    combined_structure[full_key] = structure_it->second;
                }
                else {
                    // 容错：如果结构信息不存在，设置为默认值
                    combined_structure[full_key] = "TEXT";
                }
            }
            rec = std::move(prefixed);
        }

        for (size_t i = 1; i < join_info->tables.size(); ++i) {
            auto right_records = read_records(join_info->tables[i]);
            auto table_structure = read_table_structure_static(join_info->tables[i]);

            for (auto& [row_id, rec] : right_records) {
                std::unordered_map<std::string, std::string> prefixed;
                for (const auto& [k, v] : rec) {
                    std::string full_key = join_info->tables[i] + "." + k;
                    prefixed[full_key] = v;

                    // 使用find代替at，检查键是否存在
                    auto structure_it = table_structure.find(k);
                    if (structure_it != table_structure.end()) {
                        combined_structure[full_key] = structure_it->second;
                    }
                    else {
                        // 容错：如果结构信息不存在，设置为默认值
                        combined_structure[full_key] = "TEXT";
                    }
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

                            auto left_it = r1.find(left_field);
                            auto right_it = r2.find(right_field);

                            // 判断键是否存在，并比较值
                            if (left_it == r1.end() || right_it == r2.end() ||
                                left_it->second != right_it->second) {
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
    else if (has_implicit_join) {
        // 处理隐式连接
        for (const auto& table : tables) {
            if (!table_exists(table)) {
                throw std::runtime_error("表 '" + table + "' 不存在。");
            }
        }

        // 获取第一个表的记录并添加前缀
        std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> result = read_records(tables[0]);
        auto table_structure = read_table_structure_static(tables[0]);

        for (auto& [row_id, rec] : result) {
            std::unordered_map<std::string, std::string> prefixed;
            for (const auto& [k, v] : rec) {
                std::string full_key = tables[0] + "." + k;
                prefixed[full_key] = v;

                auto structure_it = table_structure.find(k);
                if (structure_it != table_structure.end()) {
                    combined_structure[full_key] = structure_it->second;
                }
                else {
                    combined_structure[full_key] = "TEXT";
                }
            }
            rec = std::move(prefixed);
        }

        // 处理其他表（类似JOIN逻辑但没有JOIN条件）
        for (size_t i = 1; i < tables.size(); ++i) {
            auto right_records = read_records(tables[i]);
            auto table_structure = read_table_structure_static(tables[i]);

            for (auto& [row_id, rec] : right_records) {
                std::unordered_map<std::string, std::string> prefixed;
                for (const auto& [k, v] : rec) {
                    std::string full_key = tables[i] + "." + k;
                    prefixed[full_key] = v;

                    auto structure_it = table_structure.find(k);
                    if (structure_it != table_structure.end()) {
                        combined_structure[full_key] = structure_it->second;
                    }
                    else {
                        combined_structure[full_key] = "TEXT";
                    }
                }
                rec = std::move(prefixed);
            }

            // 创建笛卡尔积（所有可能的组合）
            std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> new_result;
            for (const auto& [row_r1, r1] : result) {
                for (const auto& [row_r2, r2] : right_records) {
                    // 对于隐式连接，先创建笛卡尔积，后面通过WHERE条件过滤
                    std::unordered_map<std::string, std::string> combined = r1;
                    combined.insert(r2.begin(), r2.end());
                    new_result.emplace_back(0, std::move(combined));
                }
            }
            result = std::move(new_result);
        }
        filtered = std::move(result);
    }
    else {
        // 单表查询
        if (!table_exists(table_name)) {
            throw std::runtime_error("表 '" + table_name + "' 不存在。");
        }
        filtered = read_records(table_name);
        combined_structure = read_table_structure_static(table_name);
    }

    // 处理 WHERE 条件
    Record temp;
    temp.set_table_name(has_implicit_join ? tables[0] : table_name);
    temp.table_structure = combined_structure;
    if (!condition.empty()) temp.parse_condition(condition);

    std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> condition_filtered;

    // 处理条件
    if (condition.empty()) {
        condition_filtered = filtered;
    }
    else {
        // 对于隐式连接或含WHERE条件的查询，使用matches_condition进行过滤
        for (const auto& [row_id, rec] : filtered) {
            try {
                if (temp.matches_condition(rec, true)) {
                    condition_filtered.emplace_back(row_id, rec);
                }
            }
            catch (const std::exception& e) {
                // 处理可能的异常，比如字段名未找到等
                // 可以选择跳过这条记录或抛出异常
                // std::cerr << "条件匹配失败: " << e.what() << std::endl;
                continue;
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
        rec.set_table_name(has_implicit_join ? "multiple_tables" : table_name);
        for (const auto& col : selected_cols) {
            auto it = rec_map.find(col);
            std::string val = (it != rec_map.end()) ? it->second : "NULL";

            // 如果col没有表前缀，尝试查找完整的键
            if (it == rec_map.end() && col.find('.') == std::string::npos) {
                // 遍历所有键，查找匹配的后缀
                for (const auto& [k, v] : rec_map) {
                    size_t dot_pos = k.find('.');
                    if (dot_pos != std::string::npos) {
                        std::string suffix = k.substr(dot_pos + 1);
                        if (suffix == col) {
                            val = v;
                            break;
                        }
                    }
                }
            }

            auto type_it = combined_structure.find(col);
            // 如果没有找到类型信息，尝试查找完整的键
            if (type_it == combined_structure.end() && col.find('.') == std::string::npos) {
                for (const auto& [k, t] : combined_structure) {
                    size_t dot_pos = k.find('.');
                    if (dot_pos != std::string::npos) {
                        std::string suffix = k.substr(dot_pos + 1);
                        if (suffix == col) {
                            type_it = combined_structure.find(k);
                            break;
                        }
                    }
                }
            }

            // 安全处理BOOL类型转换
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