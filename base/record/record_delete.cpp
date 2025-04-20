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

void Record::delete_(const std::string& tableName, const std::string& condition) {
    this->table_name = tableName;

    if (!table_exists(this->table_name)) {
        throw std::runtime_error("表 '" + this->table_name + "' 不存在。");
    }

    std::vector<FieldBlock> fields = read_field_blocks(table_name);
    this->table_structure = read_table_structure_static(table_name);

    if (!condition.empty()) {
        parse_condition(condition);
    }

    std::ifstream infile(table_name + ".trd", std::ios::binary);
    std::ofstream outfile(table_name + ".tmp", std::ios::binary);
    if (!infile || !outfile) {
        throw std::runtime_error("无法打开数据文件进行删除操作。");
    }

    int deleted_count = 0;

    while (true) {
        std::unordered_map<std::string, std::string> record_data;
        std::streampos start_pos = infile.tellg();
        bool valid_record = true;

        for (const auto& field : fields) {
            if (infile.eof()) {
                valid_record = false;
                break;
            }
            record_data[field.name] = read_field(infile, field);
        }

        if (!valid_record || infile.eof()) break;

        // 判断是否满足删除条件
        bool matches = condition.empty() || matches_condition(record_data);
        if (matches) {
            if (!check_references_before_delete(table_name, record_data)) {
                throw std::runtime_error("删除操作违反引用完整性约束");
            }
            deleted_count++;
            continue;  // 不写入该记录
        }

        // 回到起始位置写入原始记录
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

    std::cout << "成功删除了 " << deleted_count << " 条记录。" << std::endl;
}

