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

    if (!table_exists(tableName)) throw std::runtime_error("表不存在");

    std::vector<FieldBlock> fields = read_field_blocks(table_name);
    this->table_structure = read_table_structure_static(table_name);
    if (!condition.empty()) parse_condition(condition);

    std::unordered_map<std::string, std::string> updates;
    std::istringstream ss(setClause);
    std::string pair;
    while (std::getline(ss, pair, ',')) {
        size_t eq = pair.find('=');
        if (eq == std::string::npos) throw std::runtime_error("SET语法错误");

        std::string col = pair.substr(0, eq);
        std::string val = pair.substr(eq + 1);
        col.erase(0, col.find_first_not_of(" \t")); col.erase(col.find_last_not_of(" \t") + 1);
        val.erase(0, val.find_first_not_of(" \t")); val.erase(val.find_last_not_of(" \t") + 1);

        if (table_structure.find(col) == table_structure.end())
            throw std::runtime_error("字段不存在: " + col);
        if (!is_valid_type(val, table_structure[col]))
            throw std::runtime_error("值类型不匹配: " + col + " = " + val);

        updates[col] = val;
    }

    if (updates.empty()) throw std::runtime_error("没有需要更新的字段");

    std::ifstream infile(table_name + ".trd", std::ios::binary);
    std::ofstream outfile(table_name + ".tmp", std::ios::binary);
    if (!infile || !outfile) throw std::runtime_error("无法打开文件");

    std::unordered_map<std::string, std::string> record_data;
    int updated = 0;

    while (read_single_record(infile, fields, record_data)) {
        bool should_update = condition.empty() || matches_condition(record_data);

        if (should_update) {
            // 应用更新
            for (const auto& [col, val] : updates) {
                record_data[col] = val;
            }

            std::vector<std::string> cols, vals;
            for (const auto& [k, v] : record_data) {
                cols.push_back(k);
                vals.push_back(v);
            }

            std::vector<ConstraintBlock> constraints = read_constraints(table_name);
            if (!check_constraints(cols, vals, constraints)) {
                throw std::runtime_error("违反表约束");
            }

            // 重要：将约束检查后可能修改的值同步回record_data
            for (size_t i = 0; i < cols.size(); ++i) {
                record_data[cols[i]] = vals[i];
            }

            updated++;
        }

        // 写入记录（原始的或更新后的）
        for (const auto& field : fields) {
            write_field(outfile, field, record_data[field.name]);
        }
    }

    infile.close();
    outfile.close();

    if (std::remove((table_name + ".trd").c_str()) != 0) {
        throw std::runtime_error("无法删除原表文件");
    }

    if (std::rename((table_name + ".tmp").c_str(), (table_name + ".trd").c_str()) != 0) {
        throw std::runtime_error("无法重命名临时文件");
    }

    std::cout << "成功更新了 " << updated << " 条记录。" << std::endl;
}

