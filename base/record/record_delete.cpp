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

int Record::delete_(const std::string& tableName, const std::string& condition) {
    this->table_name = tableName;

    if (!table_exists(this->table_name)) {
        throw std::runtime_error("表 '" + this->table_name + "' 不存在。");
    }

    std::vector<FieldBlock> fields = read_field_blocks(table_name);
    this->table_structure = read_table_structure_static(table_name);

    if (!condition.empty()) parse_condition(condition);

    std::ifstream infile(table_name + ".trd", std::ios::binary);
    std::ofstream outfile(table_name + ".tmp", std::ios::binary);
    if (!infile || !outfile) {
        throw std::runtime_error("无法打开数据文件进行删除操作。");
    }

    int deleted_count = 0;
    std::unordered_map<std::string, std::string> record_data;

    while (read_single_record(infile, fields, record_data)) {
        if (condition.empty() || matches_condition(record_data, false)) {
            if (!check_references_before_delete(table_name, record_data)) {
                throw std::runtime_error("删除操作违反引用完整性约束");
            }
            deleted_count++;
            continue;
        }

        for (const auto& field : fields) {
            write_field(outfile, field, record_data[field.name]);
        }
    }

    infile.close();
    outfile.close();

    std::remove((table_name + ".trd").c_str());
    if (std::rename((table_name + ".tmp").c_str(), (table_name + ".trd").c_str()) != 0) {
        throw std::runtime_error("无法替换原数据文件。");
    }

    return deleted_count;
}
