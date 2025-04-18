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

    if (!table_exists(table_name)) {
        throw std::runtime_error("表 '" + table_name + "' 不存在。");
    }

    std::vector<FieldBlock> fields = read_field_blocks(table_name);
    this->table_structure = read_table_structure_static(table_name);

    if (!condition.empty()) {
        try {
            parse_condition(condition);
        }
        catch (const std::exception& e) {
            throw std::runtime_error("条件解析错误: " + std::string(e.what()));
        }
    }

    std::string trd_filename = table_name + ".trd";
    std::string tmp_filename = table_name + ".tmp";

    std::ifstream infile(trd_filename, std::ios::binary);
    std::ofstream outfile(tmp_filename, std::ios::binary);
    if (!infile || !outfile) {
        throw std::runtime_error("无法打开数据文件进行删除操作。");
    }

    std::vector<std::unordered_map<std::string, std::string>> records = read_records(table_name);
    int deleted_count = 0;

    for (const auto& record_data : records) {
        if (!condition.empty() && matches_condition(record_data)) {
            if (!check_references_before_delete(table_name, record_data)) {
                throw std::runtime_error("删除操作违反引用完整性约束");
            }
            deleted_count++;
            continue;
        }

        for (const auto& field : fields) {
            auto it = record_data.find(field.name);

            if (it == record_data.end() || it->second == "NULL") {
                char null_flag = 1;
                outfile.write(&null_flag, sizeof(char));

                size_t dummy_size = 0;
                switch (field.type) {
                case 1: dummy_size = sizeof(int); break;
                case 2: dummy_size = sizeof(float); break;
                case 3:
                case 4: dummy_size = field.param; break;
                case 5: dummy_size = sizeof(std::time_t); break;
                }
                std::vector<char> dummy(dummy_size, 0);
                outfile.write(dummy.data(), dummy_size);
            }
            else {
                char null_flag = 0;
                outfile.write(&null_flag, sizeof(char));

                const std::string& val = it->second;
                switch (field.type) {
                case 1: {
                    int v = std::stoi(val);
                    outfile.write(reinterpret_cast<const char*>(&v), sizeof(int));
                    break;
                }
                case 2: {
                    float v = std::stof(val);
                    outfile.write(reinterpret_cast<const char*>(&v), sizeof(float));
                    break;
                }
                case 3:
                case 4: {
                    std::vector<char> buf(field.param, 0);
                    std::memcpy(buf.data(), val.c_str(), std::min((size_t)field.param, val.size()));
                    outfile.write(buf.data(), field.param);
                    break;
                }
                case 5: { // DATETIME
                    std::tm tm = custom_strptime(val, "%Y-%m-%d %H:%M:%S");
                    std::time_t t = std::mktime(&tm);
                    outfile.write(reinterpret_cast<const char*>(&t), sizeof(std::time_t));
                    break;
                }
                }
            }

            size_t field_size = 0;
            switch (field.type) {
            case 1: field_size = sizeof(int); break;
            case 2: field_size = sizeof(float); break;
            case 3:
            case 4: field_size = field.param; break;
            case 5: field_size = sizeof(std::time_t); break;
            }
            size_t padding = (4 - (sizeof(char) + field_size) % 4) % 4;
            char pad[4] = { 0 };
            outfile.write(pad, padding);
        }
    }

    infile.close();
    outfile.close();

    std::remove(trd_filename.c_str());
    if (std::rename(tmp_filename.c_str(), trd_filename.c_str()) != 0) {
        throw std::runtime_error("无法替换原表数据文件");
    }

    std::cout << "成功删除了 " << deleted_count << " 条记录。" << std::endl;
}
