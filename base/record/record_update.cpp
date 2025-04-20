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

void Record::update(const std::string& tableName, const std::string& setClause, const std::string& condition) {
    this->table_name = tableName;

    if (!table_exists(this->table_name)) {
        throw std::runtime_error("表 '" + this->table_name + "' 不存在。");
    }

    std::vector<FieldBlock> fields = read_field_blocks(table_name);
    this->table_structure = read_table_structure_static(table_name);

    // 解析SET语句
    std::unordered_map<std::string, std::string> update_values;
    std::istringstream ss(setClause);
    std::string pair;
    while (std::getline(ss, pair, ',')) {
        size_t eq_pos = pair.find('=');
        if (eq_pos == std::string::npos) throw std::runtime_error("无效的SET语法: " + pair);

        std::string field = pair.substr(0, eq_pos);
        std::string value = pair.substr(eq_pos + 1);

        // 去除空格
        field.erase(0, field.find_first_not_of(" \t"));
        field.erase(field.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        // 字段存在性与类型判断
        if (table_structure.find(field) == table_structure.end()) {
            throw std::runtime_error("字段 '" + field + "' 不存在于表结构中。");
        }
        if (!is_valid_type(value, table_structure[field])) {
            throw std::runtime_error("字段 '" + field + "' 的值 '" + value + "' 与其类型不匹配");
        }

        update_values[field] = value;
    }

    if (update_values.empty()) {
        throw std::runtime_error("没有需要更新的字段。");
    }

    if (!condition.empty()) {
        parse_condition(condition);
    }

    std::ifstream infile(table_name + ".trd", std::ios::binary);
    std::ofstream outfile(table_name + ".tmp", std::ios::binary);
    if (!infile || !outfile) {
        throw std::runtime_error("无法打开数据文件进行更新操作。");
    }

    int updated_count = 0;

    while (true) {
        std::unordered_map<std::string, std::string> record_data;
        std::streampos start_pos = infile.tellg();
        bool valid = true;

        for (const auto& field : fields) {
            if (infile.eof()) {
                valid = false;
                break;
            }
            record_data[field.name] = read_field(infile, field);
        }

        if (!valid || infile.eof()) break;

        bool matches = condition.empty() || matches_condition(record_data);

        // 执行更新逻辑
        if (matches) {
            for (const auto& [key, val] : update_values) {
                record_data[key] = val;
            }

            std::vector<std::string> cols, vals;
            for (const auto& [k, v] : record_data) {
                cols.push_back(k);
                vals.push_back(v);
            }

            std::vector<ConstraintBlock> constraints = read_constraints(table_name);
            if (!check_constraints(cols, vals, constraints)) {
                throw std::runtime_error("更新后的数据违反表约束");
            }

            updated_count++;
        }

        // 回到原始位置写入更新/原始记录
        infile.seekg(start_pos);
        for (const auto& field : fields) {
            write_field(outfile, field, record_data[field.name]);
        }
    }

    infile.close();
    outfile.close();

    std::remove((table_name + ".trd").c_str());
    if (std::rename((table_name + ".tmp").c_str(), (table_name + ".trd").c_str()) != 0) {
        throw std::runtime_error("无法替换原表数据文件。");
    }

    std::cout << "成功更新了 " << updated_count << " 条记录。" << std::endl;
}


