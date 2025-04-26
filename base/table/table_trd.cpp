#include "table.h"
#include <iostream>
#include <ctime>
#include "manager/parse.h"
#include <base/Record/record.h>
#include <cstring>
#include <iomanip>
#include"manager/dbManager.h"

std::string Table::getDefaultValue(const std::string& fieldName) const {
    for (const auto& constraint : m_constraints) {
        if (constraint.type == 6 && fieldName == constraint.field) { // 6: DEFAULT
            return constraint.param; // 找到了默认值，返回
        }
    }
    return "NULL"; // 没找到，返回"NULL"
}

void Table::updateRecord(std::vector<FieldBlock>& fields) {
    // 检查表是否存在
    if (!isTableExist()) {
        throw std::runtime_error("表 '" + m_tableName + "' 不存在。");
    }

    // 打开 .trd 文件
    std::fstream file(m_trd, std::ios::in | std::ios::out | std::ios::binary);
    if (!file) {
        throw std::runtime_error("无法打开表文件 '" + m_tableName + ".trd' 进行更新。");
    }

    // 读取现有记录
    std::vector<std::vector<std::string>> all_records;
    while (file.peek() != EOF) {
        std::vector<std::string> record_values;
        for (size_t i = 0; i < m_fields.size(); ++i) {
            FieldBlock& field = m_fields[i];
            bool is_null = false;
            file.read(reinterpret_cast<char*>(&is_null), sizeof(bool));

            if (is_null) {
                record_values.push_back("NULL");
                continue;
            }

            std::string value;
            switch (field.type) {
            case 1: { // INTEGER
                int int_val;
                file.read(reinterpret_cast<char*>(&int_val), sizeof(int));
                value = std::to_string(int_val);
                break;
            }
            case 2: { // DOUBLE
                double double_val;
                file.read(reinterpret_cast<char*>(&double_val), sizeof(double));
                value = std::to_string(double_val);
                break;
            }
            case 3: // VARCHAR
            case 4: { // CHAR
                char* buffer = new char[field.param];
                file.read(buffer, field.param);
                value = std::string(buffer, field.param);
                delete[] buffer;
                break;
            }
            case 5: { // DATETIME
                std::time_t timestamp;
                file.read(reinterpret_cast<char*>(&timestamp), sizeof(std::time_t));
                value = std::to_string(timestamp); // 或者转化为日期字符串
                break;
            }
            default:
                throw std::runtime_error("未知字段类型");
            }
            record_values.push_back(value);
        }
        all_records.push_back(record_values);
    }

    // 遍历 fields 和 m_fields，进行字段的删除、新增和更新操作
    for (auto& record : all_records) {
        // 删除字段：检查 fields 中的字段是否不在 m_fields 中
        for (int i = fields.size() - 1; i >= 0; --i) {  // 倒序遍历
            FieldBlock& field = fields[i];

            // 如果 fields 中的字段在 m_fields 中找不到，删除该字段
            auto it = std::find_if(m_fields.begin(), m_fields.end(), [&](const FieldBlock& f) {
                return f.name == field.name;
                });

            if (it == m_fields.end()) {
                // 删除对应的字段值
                record.erase(record.begin() + i);
                m_fields.erase(m_fields.begin() + i); // 删除 m_fields 中的字段
            }
        }

        // 新增字段：检查 m_fields 中是否有 fields 中没有的字段
        for (size_t i = 0; i < m_fields.size(); ++i) {
            FieldBlock& updated_field = m_fields[i];

            // 查找该字段在 fields 中是否已经存在
            auto it = std::find_if(fields.begin(), fields.end(), [&](const FieldBlock& f) {
                return f.name == updated_field.name;
                });

            if (it == fields.end()) {
                // 如果该字段在 fields 中不存在，说明是新增字段
                std::string new_value = "NULL"; // 默认为 NULL

                // 查找是否有 DEFAULT 或 AUTO_INCREMENT 约束
                for (const auto& constraint : m_constraints) {
                    if (constraint.field == updated_field.name) {
                        switch (constraint.type) {
                        case 6: { // DEFAULT 约束
                            new_value = constraint.param; // 使用 DEFAULT 值
                            break;
                        }
                        case 7: { // AUTO_INCREMENT 约束
                            static int auto_increment_value = 1; // 自增值，从1开始
                            new_value = std::to_string(auto_increment_value++);
                            break;
                        }
                        default:
                            break;
                        }
                    }
                }

                // 将默认值或自增值填入记录
                record.push_back(new_value);
                m_fields.push_back(updated_field); // 更新表的字段信息
            }
        }

        //// 更新字段：fields 和 m_fields 中都有的字段需要更新
        for (size_t i = 0; i < fields.size(); ++i) {
            FieldBlock& updated_field = fields[i];

            // 查找 fields 中的字段是否在 m_fields 中存在
            auto it = std::find_if(m_fields.begin(), m_fields.end(), [&](const FieldBlock& f) {
                return std::string(f.name) == updated_field.name;
                });

            if (it != m_fields.end()) {
                // 如果在 m_fields 中找到了对应字段
                size_t field_index = std::distance(m_fields.begin(), it);

                // 获取当前字段的约束
                FieldBlock& old_field = m_fields[field_index];
                bool is_integrity_changed = (old_field.integrities != updated_field.integrities);

                // 如果约束发生变化，则需要进一步检查是 DEFAULT、NOT NULL 或 AUTO_INCREMENT 变化
                if (is_integrity_changed) {
                    // 检查约束变化：DEFAULT、NOT NULL 和 AUTO_INCREMENT
                    std::string new_value; // 获取字段的新值

                    for (const auto& constraint : m_constraints) {
                        if (constraint.field == updated_field.name) {
                            switch (constraint.type) {
                            case 6: { // DEFAULT 约束
                                if (new_value == "NULL" || new_value.empty()) {
                                    new_value = constraint.param; // 使用 DEFAULT 值
                                }
                                break;
                            }
                            case 5: { // NOT NULL 约束
                                if (new_value == "NULL" || new_value.empty()) {
                                    new_value = "0"; // 使用空值
                                }
                                break;
                            }
                            case 7: { // AUTO_INCREMENT 约束
                                static int auto_increment_value = 1; // 假设从 1 开始自增
                                if (new_value == "NULL" || new_value.empty()) {
                                    new_value = std::to_string(auto_increment_value++); // 自增处理
                                }
                                break;
                            }
                            default:
                                break;
                            }
                        }
                    }

                    // 更新字段值
                    record[field_index] = new_value;
                }
            }
        }
    }

    // 清空原文件并重新写入所有更新后的记录
    file.close();
    file.open(m_trd, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!file) {
        throw std::runtime_error("无法重新打开表文件 '" + m_tableName + ".trd' 进行写入。");
    }

    // 重新写入更新后的记录
    for (const auto& record : all_records) {
        for (size_t i = 0; i < m_fields.size(); ++i) {
            const FieldBlock& field = m_fields[i];
            const std::string& value = record[i];

            // 写入NULL标志
            bool is_null = (value == "NULL");
            file.write(reinterpret_cast<const char*>(&is_null), sizeof(bool));

            if (!is_null) {
                switch (field.type) {
                case 1: { // INTEGER
                    int int_val = std::stoi(value);
                    file.write(reinterpret_cast<const char*>(&int_val), sizeof(int));
                    break;
                }
                case 2: { // DOUBLE
                    double double_val = std::stod(value);
                    file.write(reinterpret_cast<const char*>(&double_val), sizeof(double));
                    break;
                }
                case 3: { // VARCHAR
                    char* buffer = new char[field.param];
                    std::memset(buffer, 0, field.param);

                    if (strncpy_s(buffer, field.param, value.c_str(), field.param - 1) != 0) {
                        throw std::runtime_error("字符串复制失败。");
                    }

                    file.write(buffer, field.param);
                    delete[] buffer;
                    break;
                }
                case 4: { // BOOL
                    bool bool_val = (value == "true" || value == "1");
                    file.write(reinterpret_cast<const char*>(&bool_val), sizeof(bool));
                    break;
                }
                case 5: { // DATETIME
                    std::time_t now = std::time(nullptr); // 简化为当前时间
                    file.write(reinterpret_cast<const char*>(&now), sizeof(std::time_t));
                    break;
                }
                default:
                    throw std::runtime_error("未知字段类型");
                }
            }
        }
    }

    file.close();
    std::cout << "记录更新成功。" << std::endl;
}

void Table::updateRecord_add(FieldBlock& field) {
    // 检查表是否存在
    if (!isTableExist()) {
        throw std::runtime_error("表 '" + m_tableName + "' 不存在。");
    }

    // 打开 .trd 文件
    std::fstream file(m_trd, std::ios::in | std::ios::out | std::ios::binary);
    if (!file) {
        throw std::runtime_error("无法打开表文件 '" + m_tableName + ".trd' 进行更新。");
    }
    file.close(); // 先关掉，接下来重新覆盖写

    // 读取现有记录
    std::vector<std::unordered_map<std::string, std::string>> records;
    records = Record::read_records(m_tableName);

    // 获取新字段的默认值
    std::string defaultValue = getDefaultValue(field.name);

    // 给每条记录新增这个字段
    for (auto& record : records) {
        record[field.name] = defaultValue; // 添加新字段
    }

    // 重新写入所有更新后的记录
    file.open(m_trd, std::ios::out | std::ios::binary | std::ios::trunc); // 覆盖写
    if (!file) {
        throw std::runtime_error("无法重新打开表文件 '" + m_tableName + ".trd' 进行写入。");
    }

    for (const auto& record : records) {
        for (const auto& fieldBlock : m_fields) {
            auto it = record.find(fieldBlock.name);
            if (it == record.end()) {
                throw std::runtime_error("字段 '" + std::string(fieldBlock.name) + "' 缺失对应值。");
            }
            const std::string& value = it->second;

            // 写入 NULL 标志
            bool is_null = (value == "NULL");
            file.write(reinterpret_cast<const char*>(&is_null), sizeof(bool));

            if (!is_null) {
                switch (fieldBlock.type) {
                case 1: { // INT
                    int int_val = std::stoi(value);
                    file.write(reinterpret_cast<const char*>(&int_val), sizeof(int));
                    break;
                }
                case 2: { // DOUBLE
                    double double_val = std::stod(value);
                    file.write(reinterpret_cast<const char*>(&double_val), sizeof(double));
                    break;
                }
                case 3: // VARCHAR
                case 4: { // CHAR
                    char* buffer = new char[fieldBlock.param];
                    std::memset(buffer, 0, fieldBlock.param);
                    if (strncpy_s(buffer, fieldBlock.param, value.c_str(), fieldBlock.param - 1) != 0) {
                        throw std::runtime_error("字符串复制失败。");
                    }
                    file.write(buffer, fieldBlock.param);
                    delete[] buffer;
                    break;
                }
                case 5: { // DATETIME
                    std::time_t timestamp = static_cast<std::time_t>(std::stoll(value));
                    file.write(reinterpret_cast<const char*>(&timestamp), sizeof(std::time_t));
                    break;
                }
                default:
                    throw std::runtime_error("未知字段类型。");
                }
            }
        }
    }

    file.close();
    std::cout << "添加字段 '" << field.name << "' 并更新记录成功。" << std::endl;
}

