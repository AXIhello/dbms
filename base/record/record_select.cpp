#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "Record.h"
#include "manager/parse.h"
#include "ui/output.h"

#include <regex>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <algorithm>


std::vector<Record> Record::select(const std::string& columns, const std::string& table_name, const std::string& condition) {
    std::vector<Record> results;

    // 检查表是否存在
    if (!table_exists(table_name)) {
        throw std::runtime_error("表 '" + table_name + "' 不存在。");
    }

    // 读取字段定义（用于列名/类型验证）
    std::vector<FieldBlock> fields = read_field_blocks(table_name);
    std::unordered_map<std::string, std::string> table_structure = read_table_structure_static(table_name);

    // 构造所有列名列表
    std::vector<std::string> all_columns;
    for (const auto& field : fields) {
        all_columns.push_back(field.name);
    }

    // 确定查询的列
    std::vector<std::string> selected_columns;
    if (columns == "*") {
        selected_columns = all_columns;
    }
    else {
        selected_columns = parse_column_list(columns);
        for (const auto& col : selected_columns) {
            if (table_structure.find(col) == table_structure.end()) {
                throw std::runtime_error("字段 '" + col + "' 不存在于表 '" + table_name + "' 中。");
            }
        }
    }

    // 创建一个临时对象处理条件
    Record temp_record;
    temp_record.set_table_name(table_name);
    temp_record.table_structure = table_structure;

    if (!condition.empty()) {
        temp_record.parse_condition(condition);
    }

    // 使用通用读取方法获取所有记录（支持NULL标识）
    std::vector<std::unordered_map<std::string, std::string>> raw_records = read_records(table_name);

    // 遍历并筛选满足条件的记录
    for (const auto& raw_record : raw_records) {
        if (!condition.empty() && !temp_record.matches_condition(raw_record)) {
            continue;
        }

        // 构造结果记录
        Record result_record;
        result_record.set_table_name(table_name);

        for (const auto& col : selected_columns) {
            result_record.add_column(col);
            if (raw_record.count(col)) {
                result_record.add_value(raw_record.at(col));
            }
            else {
                result_record.add_value("NULL");
            }
        }

        results.push_back(result_record);
    }

    return results;
}
