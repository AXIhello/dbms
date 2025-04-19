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

    if (!table_exists(table_name)) {
        throw std::runtime_error("表 '" + table_name + "' 不存在。");
    }

    std::vector<FieldBlock> fields = read_field_blocks(table_name);
    std::unordered_map<std::string, std::string> table_structure = read_table_structure_static(table_name);

    std::vector<std::string> all_columns;
    for (const auto& field : fields) {
        all_columns.push_back(field.name);
    }

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

    Record temp;
    temp.set_table_name(table_name);
    temp.table_structure = table_structure;
    if (!condition.empty()) {
        temp.parse_condition(condition);
    }

    std::ifstream infile(table_name + ".trd", std::ios::binary);
    if (!infile) {
        throw std::runtime_error("无法打开数据文件进行查询。");
    }

    while (!infile.eof()) {
        std::unordered_map<std::string, std::string> record_data;
        std::streampos record_start = infile.tellg();

        for (const auto& field : fields) {
            record_data[field.name] = read_field(infile, field);
        }

        if (infile.eof()) break;

        if (!condition.empty() && !temp.matches_condition(record_data)) {
            continue;
        }

        Record result_record;
        result_record.set_table_name(table_name);
        for (const auto& col : selected_columns) {
            result_record.add_column(col);
            result_record.add_value(record_data.count(col) ? record_data.at(col) : "NULL");
        }

        results.push_back(result_record);
    }

    infile.close();
    return results;
}
