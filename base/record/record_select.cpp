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

//// 修改版 select 方法，支持隐式连接
//std::vector<Record> Record::select(
//    const std::string& columns,
//    const std::string& table_name,
//    const std::string& condition,
//    const std::string& group_by,
//    const std::string& order_by,
//    const std::string& having,
//    const JoinInfo* join_info)
//{
//    std::vector<Record> records;
//    std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> filtered;
//    std::unordered_map<std::string, std::string> combined_structure;
//
//    // 检查是否包含隐式连接（逗号分隔的表名）
//    bool has_implicit_join = table_name.find(',') != std::string::npos;
//    std::vector<std::string> tables;
//
//    if (has_implicit_join) {
//        // 解析逗号分隔的表名
//        std::istringstream ss(table_name); 
//        std::string table;
//        while (std::getline(ss, table, ',')) {
//            // 去除前后空格
//            table.erase(0, table.find_first_not_of(" \t"));
//            table.erase(table.find_last_not_of(" \t") + 1);
//            if (!table.empty()) {
//                tables.push_back(table);
//            }
//        }
//    }
//
//    if (join_info && !join_info->tables.empty()) {
//        // 处理显式JOIN的代码保持不变
//        std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> result = read_records(join_info->tables[0]);
//        for (auto& [row_id, rec] : result) {
//            std::unordered_map<std::string, std::string> prefixed;
//            auto table_structure = read_table_structure_static(join_info->tables[0]);
//
//            for (const auto& [k, v] : rec) {
//                std::string full_key = join_info->tables[0] + "." + k;
//                prefixed[full_key] = v;
//
//                // 使用find代替at，检查键是否存在
//                auto structure_it = table_structure.find(k);
//                if (structure_it != table_structure.end()) {
//                    combined_structure[full_key] = structure_it->second;
//                }
//                else {
//                    // 容错：如果结构信息不存在，设置为默认值
//                    combined_structure[full_key] = "TEXT";
//                }
//            }
//            rec = std::move(prefixed);
//        }
//
//        for (size_t i = 1; i < join_info->tables.size(); ++i) {
//            auto right_records = read_records(join_info->tables[i]);
//            auto table_structure = read_table_structure_static(join_info->tables[i]);
//
//            for (auto& [row_id, rec] : right_records) {
//                std::unordered_map<std::string, std::string> prefixed;
//                for (const auto& [k, v] : rec) {
//                    std::string full_key = join_info->tables[i] + "." + k;
//                    prefixed[full_key] = v;
//
//                    // 使用find代替at，检查键是否存在
//                    auto structure_it = table_structure.find(k);
//                    if (structure_it != table_structure.end()) {
//                        combined_structure[full_key] = structure_it->second;
//                    }
//                    else {
//                        // 容错：如果结构信息不存在，设置为默认值
//                        combined_structure[full_key] = "TEXT";
//                    }
//                }
//                rec = std::move(prefixed);
//            }
//
//            std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> new_result;
//            for (const auto& [row_r1, r1] : result) {
//                for (const auto& [row_r2, r2] : right_records) {
//                    bool match = true;
//                    for (const auto& join : join_info->joins) {
//                        bool relevant =
//                            (join.left_table == join_info->tables[i - 1] && join.right_table == join_info->tables[i]) ||
//                            (join.left_table == join_info->tables[i] && join.right_table == join_info->tables[i - 1]);
//                        if (!relevant) continue;
//
//                        for (const auto& [left_col, right_col] : join.conditions) {
//                            std::string left_field = join.left_table + "." + left_col;
//                            std::string right_field = join.right_table + "." + right_col;
//
//                            auto left_it = r1.find(left_field);
//                            auto right_it = r2.find(right_field);
//
//                            // 判断键是否存在，并比较值
//                            if (left_it == r1.end() || right_it == r2.end() ||
//                                left_it->second != right_it->second) {
//                                match = false;
//                                break;
//                            }
//                        }
//                        if (!match) break;
//                    }
//
//                    if (match) {
//                        std::unordered_map<std::string, std::string> combined = r1;
//                        combined.insert(r2.begin(), r2.end());
//                        new_result.emplace_back(0, std::move(combined));
//                    }
//                }
//            }
//            result = std::move(new_result);
//        }
//        filtered = std::move(result);
//    }
//    else if (has_implicit_join) {
//        // 处理隐式连接
//        for (const auto& table : tables) {
//            if (!table_exists(table)) {
//                throw std::runtime_error("表 '" + table + "' 不存在。");
//            }
//        }
//
//        // 获取第一个表的记录并添加前缀
//        std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> result = read_records(tables[0]);
//        auto table_structure = read_table_structure_static(tables[0]);
//
//        for (auto& [row_id, rec] : result) {
//            std::unordered_map<std::string, std::string> prefixed;
//            for (const auto& [k, v] : rec) {
//                std::string full_key = tables[0] + "." + k;
//                prefixed[full_key] = v;
//
//                auto structure_it = table_structure.find(k);
//                if (structure_it != table_structure.end()) {
//                    combined_structure[full_key] = structure_it->second;
//                }
//                else {
//                    combined_structure[full_key] = "TEXT";
//                }
//            }
//            rec = std::move(prefixed);
//        }
//
//        // 处理其他表（类似JOIN逻辑但没有JOIN条件）
//        for (size_t i = 1; i < tables.size(); ++i) {
//            auto right_records = read_records(tables[i]);
//            auto table_structure = read_table_structure_static(tables[i]);
//
//            for (auto& [row_id, rec] : right_records) {
//                std::unordered_map<std::string, std::string> prefixed;
//                for (const auto& [k, v] : rec) {
//                    std::string full_key = tables[i] + "." + k;
//                    prefixed[full_key] = v;
//
//                    auto structure_it = table_structure.find(k);
//                    if (structure_it != table_structure.end()) {
//                        combined_structure[full_key] = structure_it->second;
//                    }
//                    else {
//                        combined_structure[full_key] = "TEXT";
//                    }
//                }
//                rec = std::move(prefixed);
//            }
//
//            // 创建笛卡尔积（所有可能的组合）
//            std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> new_result;
//            for (const auto& [row_r1, r1] : result) {
//                for (const auto& [row_r2, r2] : right_records) {
//                    // 对于隐式连接，先创建笛卡尔积，后面通过WHERE条件过滤
//                    std::unordered_map<std::string, std::string> combined = r1;
//                    combined.insert(r2.begin(), r2.end());
//                    new_result.emplace_back(0, std::move(combined));
//                }
//            }
//            result = std::move(new_result);
//        }
//        filtered = std::move(result);
//    }
//    else {
//        // 单表查询
//        if (!table_exists(table_name)) {
//            throw std::runtime_error("表 '" + table_name + "' 不存在。");
//        }
//        filtered = read_records(table_name);
//        combined_structure = read_table_structure_static(table_name);
//    }
//
//    // 处理 WHERE 条件
//    Record temp;
//    temp.set_table_name(has_implicit_join ? tables[0] : table_name);
//    temp.table_structure = combined_structure;
//    if (!condition.empty()) temp.parse_condition(condition);
//
//    std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> condition_filtered;
//
//    // 处理条件
//    if (condition.empty()) {
//        condition_filtered = filtered;
//    }
//    else {
//        // 对于隐式连接或含WHERE条件的查询，使用matches_condition进行过滤
//        for (const auto& [row_id, rec] : filtered) {
//            try {
//                if (temp.matches_condition(rec, true)) {
//                    condition_filtered.emplace_back(row_id, rec);
//                }
//            }
//            catch (const std::exception& e) {
//                // 处理可能的异常，比如字段名未找到等
//                // 可以选择跳过这条记录或抛出异常
//                // std::cerr << "条件匹配失败: " << e.what() << std::endl;
//                continue;
//            }
//        }
//    }
//
//    // 处理 SELECT 字段
//    std::vector<std::string> selected_cols;
//    if (columns == "*") {
//        if (!condition_filtered.empty()) {
//            for (const auto& [k, _] : condition_filtered[0].second) {
//                selected_cols.push_back(k);
//            }
//        }
//    }
//    else {
//        selected_cols = parse_column_list(columns);
//    }
//
//    for (const auto& [row_id, rec_map] : condition_filtered) {
//        Record rec;
//        rec.set_table_name(has_implicit_join ? "multiple_tables" : table_name);
//        for (const auto& col : selected_cols) {
//            auto it = rec_map.find(col);
//            std::string val = (it != rec_map.end()) ? it->second : "NULL";
//
//            // 如果col没有表前缀，尝试查找完整的键
//            if (it == rec_map.end() && col.find('.') == std::string::npos) {
//                // 遍历所有键，查找匹配的后缀
//                for (const auto& [k, v] : rec_map) {
//                    size_t dot_pos = k.find('.');
//                    if (dot_pos != std::string::npos) {
//                        std::string suffix = k.substr(dot_pos + 1);
//                        if (suffix == col) {
//                            val = v;
//                            break;
//                        }
//                    }
//                }
//            }
//
//            auto type_it = combined_structure.find(col);
//            // 如果没有找到类型信息，尝试查找完整的键
//            if (type_it == combined_structure.end() && col.find('.') == std::string::npos) {
//                for (const auto& [k, t] : combined_structure) {
//                    size_t dot_pos = k.find('.');
//                    if (dot_pos != std::string::npos) {
//                        std::string suffix = k.substr(dot_pos + 1);
//                        if (suffix == col) {
//                            type_it = combined_structure.find(k);
//                            break;
//                        }
//                    }
//                }
//            }
//
//            // 安全处理BOOL类型转换
//            if (type_it != combined_structure.end() && type_it->second == "BOOL") {
//                if (val == "1") val = "TRUE";
//                else if (val == "0") val = "FALSE";
//            }
//
//            rec.add_column(col);
//            rec.add_value(val);
//        }
//        records.push_back(rec);
//    }
//
//    return records;
//}

// 修改版 select 方法，支持多表索引优化
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

    //将将逗号分隔的多个表名解析出来，并去除前后空格后存入 tables
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

    //join_info 是一个结构体指针，代表显式 JOIN 的信息。

    //join_info->tables是一个表名列表，如[“A”, “B”]。

    if (join_info && !join_info->tables.empty()) {
        // 处理显式JOIN的代码保持不变
        std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> result = read_records(join_info->tables[0]);
        for (auto& [row_id, rec] : result) {
            std::unordered_map<std::string, std::string> prefixed;
            //处理第一张表
            auto table_structure = read_table_structure_static(join_info->tables[0]);

            //为每个字段添加表名前缀
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

        //处理第二张及以后的表的字段名

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

    //wl:以上相同，作用：处理连接，将combined_structure带前缀的字段结构信息保存


    //这段好像没用
    // 
    //// 解析表名，支持显式JOIN和隐式JOIN
    //if (has_implicit_join) {
    //    // 提取逗号分隔的表名
    //    std::istringstream ss(table_name);
    //    std::string table;
    //    while (std::getline(ss, table, ',')) {
    //        // 去除前后空格
    //        table.erase(0, table.find_first_not_of(" \t"));
    //        table.erase(table.find_last_not_of(" \t") + 1);
    //        if (!table.empty()) {
    //            tables.push_back(table);
    //        }
    //    }
    //}
    //else if (join_info && !join_info->tables.empty()) {
    //    // 使用JOIN信息中的表名
    //    tables = join_info->tables;
    //}
    //else {
    //    // 单表查询
    //    tables.push_back(table_name);
    //}


    // 解析条件，提取与各表相关的条件片段
    std::unordered_map<std::string, std::string> table_conditions;
    // 多表间的连接条件
    std::string join_condition;

    if (!condition.empty()) {
        parse_conditions_by_table(condition, tables, table_conditions, join_condition);
    }

    // 为每个表检查是否可以使用索引优化查询
    std::unordered_map<std::string, std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>>> table_records;

    for (const auto& table : tables) {
        bool used_index = false;
        std::vector<RecordPointer> index_record_ptrs;

        // 检查此表是否有索引且条件中包含可索引字段
        if (table_conditions.find(table) != table_conditions.end()) {
            std::string table_condition = table_conditions[table];

            // 提取适合索引的条件部分
            std::vector<IndexCondition> index_conditions;
            extract_index_conditions(table, table_condition, index_conditions);

            // 获取表对象
            Table* table_ptr = dbManager::getInstance().get_current_database()->getTable(table_name);
            if (!table_ptr) continue;

            // 获取表的索引
            std::vector<IndexBlock> indexes = table_ptr->getIndexes();

            // 检查是否有可用索引
            for (const auto& idx_cond : index_conditions) {
                // 遍历表中的所有索引，查找包含该字段的索引
                for (size_t i = 0; i < indexes.size(); ++i) {
                    const IndexBlock& idx_block = indexes[i];
                    bool field_in_index = false;

                    // 检查索引的字段中是否包含当前条件的字段
                    for (int j = 0; j < idx_block.field_num; ++j) {
                        if (std::string(idx_block.field[j]) == idx_cond.field_name) {
                            field_in_index = true;
                            break;
                        }
                    }

                    if (field_in_index) {
                        // 找到了匹配的索引，获取对应的B树
                        BTree* btree = table_ptr->getBTreeByIndexName(idx_block.name);
                        if (btree != nullptr) {
                            used_index = true;

                            // 根据操作符类型执行索引查询
                            switch (idx_cond.op_type) {
                            case OperatorType::EQUALS: {
                                // 等值查询
                                FieldPointer* result = btree->find(idx_cond.value);
                                if (result) {
                                    index_record_ptrs.push_back(result->recordPtr);
                                }
                                break;
                            }
                            case OperatorType::BETWEEN: {
                                // 范围查询
                                std::vector<FieldPointer> range_results;
                                btree->findRange(idx_cond.low_value, idx_cond.high_value, range_results);
                                for (const auto& fp : range_results) {
                                    index_record_ptrs.push_back(fp.recordPtr);
                                }
                                break;
                            }
                            case OperatorType::GREATER_THAN:
                            case OperatorType::GREATER_EQUAL: {
                                // 大于/大于等于查询 - 使用范围查询实现
                                std::vector<FieldPointer> range_results;
                                btree->findRange(idx_cond.value, "zzzzzzzzzzz", range_results); // 使用一个足够大的上限值
                                for (const auto& fp : range_results) {
                                    index_record_ptrs.push_back(fp.recordPtr);
                                }
                                break;
                            }
                            case OperatorType::LESS_THAN:
                            case OperatorType::LESS_EQUAL: {
                                // 小于/小于等于查询 - 使用范围查询实现
                                std::vector<FieldPointer> range_results;
                                btree->findRange("", idx_cond.value, range_results); // 使用空字符串作为下限
                                for (const auto& fp : range_results) {
                                    index_record_ptrs.push_back(fp.recordPtr);
                                }
                                break;
                            }
                            case OperatorType::LIKE: {
                                // LIKE查询 - 对于简单前缀匹配可优化为范围查询
                                if (idx_cond.value.back() == '%' && idx_cond.value.find('%') == idx_cond.value.size() - 1) {
                                    std::string prefix = idx_cond.value.substr(0, idx_cond.value.size() - 1);
                                    std::string upper_bound = prefix;
                                    upper_bound.back()++; // 增加最后一个字符ASCII值

                                    std::vector<FieldPointer> range_results;
                                    btree->findRange(prefix, upper_bound, range_results);
                                    for (const auto& fp : range_results) {
                                        index_record_ptrs.push_back(fp.recordPtr);
                                    }
                                }
                                else {
                                    // 复杂LIKE模式无法优化，退回到全表扫描
                                    used_index = false;
                                }
                                break;
                            }
                            default:
                                // 其他操作符无法使用索引
                                used_index = false;
                                break;
                            }

                            if (used_index) {
                                break; // 已找到可用索引，退出循环
                            }

                        }
                    }
                }
            }

            std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> table_filtered;

            if (used_index && !index_record_ptrs.empty()) {
                // 使用索引找到的行ID获取记录
                for (const auto& ptr : index_record_ptrs) {
                    auto record = fetch_record_by_pointer(table, ptr.row_id);
                    if (!record.second.empty()) {
                        // 检查记录是否满足完整的表条件
                        Record temp_rec;
                        temp_rec.set_table_name(table);
                        if (table_conditions.find(table) != table_conditions.end()) {
                            temp_rec.parse_condition(table_conditions[table]);
                            if (temp_rec.matches_condition(record.second)) {
                                table_filtered.push_back(record);
                            }
                        }
                        else {
                            table_filtered.push_back(record);
                        }
                    }
                }
            }
            else {
                // 无索引或不适合索引，读取所有记录并过滤
                auto all_records = read_records(table);

                // 应用表级条件过滤
                if (table_conditions.find(table) != table_conditions.end()) {
                    Record temp_rec;
                    temp_rec.set_table_name(table);
                    temp_rec.parse_condition(table_conditions[table]);

                    for (const auto& rec : all_records) {
                        if (temp_rec.matches_condition(rec.second)) {
                            table_filtered.push_back(rec);
                        }
                    }
                }
                else {
                    table_filtered = all_records;
                }
            }

            // 对多表查询，为字段添加表前缀
            if (tables.size() > 1) {
                auto table_structure = read_table_structure_static(table);

                for (auto& [row_id, rec] : table_filtered) {
                    std::unordered_map<std::string, std::string> prefixed;
                    for (const auto& [k, v] : rec) {
                        std::string full_key = table + "." + k;
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
            }
            else {
                // 单表查询，使用表结构
                combined_structure = read_table_structure_static(table);
            }

            table_records[table] = std::move(table_filtered);
        }

        // 执行表连接
        if (tables.size() == 1) {
            // 单表查询，直接使用过滤后的记录
            filtered = std::move(table_records[tables[0]]);
        }
        else {
            // 多表查询，需要执行连接
            // 1. 从第一个表开始
            filtered = std::move(table_records[tables[0]]);

            // 2. 连接其他表
            for (size_t i = 1; i < tables.size(); ++i) {
                std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> joined_records;

                // 对于每个左表记录，尝试连接右表记录
                for (const auto& [left_row_id, left_rec] : filtered) {
                    bool found_match = false;

                    for (const auto& [right_row_id, right_rec] : table_records[tables[i]]) {
                        // 检查连接条件
                        bool join_match = true;

                        if (join_info) {
                            // 使用显式JOIN条件
                            for (const auto& join : join_info->joins) {
                                if ((join.left_table == tables[i - 1] && join.right_table == tables[i]) ||
                                    (join.right_table == tables[i - 1] && join.left_table == tables[i])) {

                                    for (const auto& [left_col, right_col] : join.conditions) {
                                        std::string left_field = join.left_table + "." + left_col;
                                        std::string right_field = join.right_table + "." + right_col;

                                        auto left_it = left_rec.find(left_field);
                                        auto right_it = right_rec.find(right_field);

                                        if (left_it == left_rec.end() || right_it == right_rec.end() ||
                                            left_it->second != right_it->second) {
                                            join_match = false;
                                            break;
                                        }
                                    }
                                }
                                if (!join_match) break;
                            }
                        }
                        else if (!join_condition.empty()) {
                            // 使用WHERE中的隐式连接条件
                            Record temp_rec;
                            temp_rec.set_table_name("multi_tables");
                            temp_rec.parse_condition(join_condition);

                            // 合并左右表记录进行条件检查
                            std::unordered_map<std::string, std::string> combined_rec = left_rec;
                            combined_rec.insert(right_rec.begin(), right_rec.end());

                            join_match = temp_rec.matches_condition(combined_rec, true);
                        }
                        else {
                            // 无连接条件，执行笛卡尔积
                            // 隐式连接时默认为true
                        }

                        // 如果连接条件匹配，合并记录
                        if (join_match) {
                            found_match = true;
                            std::unordered_map<std::string, std::string> combined_rec = left_rec;
                            combined_rec.insert(right_rec.begin(), right_rec.end());
                            joined_records.emplace_back(0, std::move(combined_rec));
                        }
                    }
                }

                filtered = std::move(joined_records);
            }
        }

        //以下相同

        // 处理最终的WHERE条件（如果有多表的非连接条件）
        if (!condition.empty()) {
            std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> final_filtered;

            Record temp;
            temp.set_table_name(tables.size() > 1 ? "multi_tables" : tables[0]);
            temp.table_structure = combined_structure;
            temp.parse_condition(condition);

            for (const auto& [row_id, rec] : filtered) {
                try {
                    if (temp.matches_condition(rec, true)) {
                        final_filtered.emplace_back(row_id, rec);
                    }
                }
                catch (const std::exception& e) {
                    continue;
                }
            }

            filtered = std::move(final_filtered);
        }

        // 处理 SELECT 字段
        std::vector<std::string> selected_cols;
        if (columns == "*") {
            if (!filtered.empty()) {
                for (const auto& [k, _] : filtered[0].second) {
                    selected_cols.push_back(k);
                }
            }
        }
        else {
            selected_cols = parse_column_list(columns);
        }

        for (const auto& [row_id, rec_map] : filtered) {
            Record rec;
            rec.set_table_name(tables.size() > 1 ? "multiple_tables" : tables[0]);
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
}

