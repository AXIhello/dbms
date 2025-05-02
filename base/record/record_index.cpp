#include "Record.h"
#include "parse/parse.h"
#include "ui/output.h"

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <regex>
#include <map>
#include <set>

// 判断是否可以使用索引条件并提取出字段、操作符和值
bool Record::can_use_index(const std::string& condition, std::string& field_out, std::string& value_out, std::string& op_out) {
    std::regex simple_expr(R"(^\s*(\w+)\s*(=|!=|>|<|>=|<=)\s*'?(\w+)'?\s*$)");
    std::smatch match;
    if (std::regex_match(condition, match, simple_expr)) {
        field_out = match[1];
        op_out = match[2];
        value_out = match[3];
        return true;
    }
    return false;
}

// 尝试使用索引优化的查找逻辑
std::vector<std::unordered_map<std::string, std::string>> Record::try_index_select(
    const std::string& table_name,
    const std::string& condition) {

    std::string field, value, op;
    std::vector<std::unordered_map<std::string, std::string>> result;

    if (!can_use_index(condition, field, value, op)) {
        return {};
    }

    // 读取索引文件名（根据字段）
    std::string index_file = table_name + ".index." + field;
    if (!file_exists(index_file)) return {};

    auto index_map = read_index_map(index_file); // 读取为 multimap 或 map，见下

    if (op == "=") {
        auto range = index_map.equal_range(value);
        for (auto it = range.first; it != range.second; ++it) {
            result.push_back(read_record_by_index(table_name, it->second));
        }
    }
    else if (op == ">") {
        for (auto it = index_map.upper_bound(value); it != index_map.end(); ++it) {
            result.push_back(read_record_by_index(table_name, it->second));
        }
    }
    else if (op == ">=") {
        for (auto it = index_map.lower_bound(value); it != index_map.end(); ++it) {
            result.push_back(read_record_by_index(table_name, it->second));
        }
    }
    else if (op == "<") {
        for (auto it = index_map.begin(); it != index_map.lower_bound(value); ++it) {
            result.push_back(read_record_by_index(table_name, it->second));
        }
    }
    else if (op == "<=") {
        for (auto it = index_map.begin(); it != index_map.upper_bound(value); ++it) {
            result.push_back(read_record_by_index(table_name, it->second));
        }
    }

    return result;
}


std::map<std::string, int> read_index_map(const std::string& filename) {
    std::ifstream fin(filename);
    std::map<std::string, int> index_map;
    std::string key;
    int offset;
    while (fin >> key >> offset) {
        index_map[key] = offset;
    }
    return index_map;
}

bool file_exists(const std::string& filename) {
    std::ifstream f(filename);
    return f.good();
}

std::unordered_map<std::string, std::string> read_record_by_index(const std::string& table_name, int offset) {
    // 自行根据数据存储结构实现偏移读取
    // 示例代码假设每条记录固定大小 or 使用 "," 分割字段
    std::ifstream fin(table_name + ".data");
    fin.seekg(offset);
    std::string line;
    std::getline(fin, line);

    std::unordered_map<std::string, std::string> record;
    auto structure = Record::read_table_structure_static(table_name);
    auto fields = get_fields_from_line(line);
    int i = 0;
    for (const auto& [col, type] : structure) {
        record[col] = (i < fields.size() ? fields[i++] : "NULL");
    }
    return record;
}

std::vector<std::string> get_fields_from_line(const std::string& line) {
    std::stringstream ss(line);
    std::string item;
    std::vector<std::string> result;
    while (std::getline(ss, item, ',')) {
        result.push_back(item);
    }
    return result;
}

std::vector<std::unordered_map<std::string, std::string>>Record::read_by_index(const std::string& table_name,
    const std::string& column,
    const std::string& op,
    const std::string& value)
{
    // 假设你有 read_indexed_records(column, op, value) 实现，这里调用它
    // 否则 fallback 到 read_records 然后过滤（这里是占位逻辑）
    std::vector<std::unordered_map<std::string, std::string>> all = read_records(table_name);

    std::vector<std::unordered_map<std::string, std::string>> result;
    for (const auto& rec : all) {
        auto it = rec.find(column);
        if (it == rec.end()) continue;
        const std::string& val = it->second;

        try {
            double a = std::stod(val);
            double b = std::stod(value);

            if ((op == "=" && a == b) ||
                (op == ">" && a > b) ||
                (op == "<" && a < b) ||
                (op == ">=" && a >= b) ||
                (op == "<=" && a <= b)) {
                result.push_back(rec);
            }
        }
        catch (...) {
            if (op == "=" && val == value) {
                result.push_back(rec);
            }
        }
    }
    return result;
}
