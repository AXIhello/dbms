#include "table.h"
#include <iostream>
#include <ctime>
#include "parse/parse.h"
#include <base/Record/record.h>
#include <cstring>
#include <iomanip>
#include <QDebug>
#include"manager/dbManager.h"

std::string Table::getDefaultValue(const std::string& fieldName) const {
    for (const auto& constraint : m_constraints) {
        if (constraint.type == 6 && fieldName == constraint.field) { // 6: DEFAULT
            return constraint.param; // 找到了默认值，返回
        }
    }
    return "NULL"; // 没找到，返回"NULL"
}

// 辅助函数：打印所有记录内容
void Table::print_records(const std::vector<std::unordered_map<std::string, std::string>>& records) {
    std::cout << "\n====== 当前读取到的记录内容 ======" << std::endl;
    size_t index = 0;
    for (const auto& record : records) {
        std::cout << "记录 " << index++ << ": { ";
        for (const auto& [key, value] : record) {
            std::cout << key << ": " << value << ", ";
        }
        std::cout << " }" << std::endl;
    }
    std::cout << "====== 记录总数: " << records.size() << " ======" << std::endl;
}

//void Table::updateRecord(std::vector<FieldBlock>& fields) {
//    // 检查表是否存在
//    if (!isTableExist()) {
//        throw std::runtime_error("表 '" + m_tableName + "' 不存在。");
//    }
//
//    // 打开 .trd 文件
//    std::fstream file(m_trd, std::ios::in | std::ios::out | std::ios::binary);
//    if (!file) {
//        throw std::runtime_error("无法打开表文件 '" + m_tableName + ".trd' 进行更新。");
//    }
//
//    // 读取现有记录
//    std::vector<std::vector<std::string>> all_records;
//    while (file.peek() != EOF) {
//        std::vector<std::string> record_values;
//        for (size_t i = 0; i < m_fields.size(); ++i) {
//            FieldBlock& field = m_fields[i];
//            bool is_null = false;
//            file.read(reinterpret_cast<char*>(&is_null), sizeof(bool));
//
//            if (is_null) {
//                record_values.push_back("NULL");
//                continue;
//            }
//
//            std::string value;
//            switch (field.type) {
//            case 1: { // INTEGER
//                int int_val;
//                file.read(reinterpret_cast<char*>(&int_val), sizeof(int));
//                value = std::to_string(int_val);
//                break;
//            }
//            case 2: { // DOUBLE
//                double double_val;
//                file.read(reinterpret_cast<char*>(&double_val), sizeof(double));
//                value = std::to_string(double_val);
//                break;
//            }
//            case 3: { // VARCHAR
//                char* buffer = new char[current_field.param];
//                file.read(buffer, current_field.param);
//                value = std::string(buffer, current_field.param);
//                delete[] buffer;
//                break;
//            }
//            case 4: { // BOOL
//				bool bool_val;
//				file.read(reinterpret_cast<char*>(&bool_val), sizeof(bool));
//				value = bool_val;
//				break;
//            }
//            case 5: { // DATETIME
//                std::time_t timestamp;
//                file.read(reinterpret_cast<char*>(&timestamp), sizeof(std::time_t));
//                value = std::to_string(timestamp); // 或者转化为日期字符串
//                break;
//            }
//            default:
//                throw std::runtime_error("未知字段类型");
//            }
//            record_values.push_back(value);
//        }
//        all_records.push_back(record_values);
//    }
//
//    // 遍历 fields 和 m_fields，进行字段的删除、新增和更新操作
//    for (auto& record : all_records) {
//        // 删除字段：检查 fields 中的字段是否不在 m_fields 中
//        for (int i = fields.size() - 1; i >= 0; --i) {  // 倒序遍历
//            FieldBlock& field = fields[i];
//
//            // 如果 fields 中的字段在 m_fields 中找不到，删除该字段
//            auto it = std::find_if(m_fields.begin(), m_fields.end(), [&](const FieldBlock& f) {
//                return f.name == field.name;
//                });
//
//            if (it == m_fields.end()) {
//                // 删除对应的字段值
//                record.erase(record.begin() + i);
//                m_fields.erase(m_fields.begin() + i); // 删除 m_fields 中的字段
//            }
//        }
//
//        // 新增字段：检查 m_fields 中是否有 fields 中没有的字段
//        for (size_t i = 0; i < m_fields.size(); ++i) {
//            FieldBlock& updated_field = m_fields[i];
//
//            // 查找该字段在 fields 中是否已经存在
//            auto it = std::find_if(fields.begin(), fields.end(), [&](const FieldBlock& f) {
//                return f.name == updated_field.name;
//                });
//
//            if (it == fields.end()) {
//                // 如果该字段在 fields 中不存在，说明是新增字段
//                std::string new_value = "NULL"; // 默认为 NULL
//
//                // 查找是否有 DEFAULT 或 AUTO_INCREMENT 约束
//                for (const auto& constraint : m_constraints) {
//                    if (constraint.field == updated_field.name) {
//                        switch (constraint.type) {
//                        case 6: { // DEFAULT 约束
//                            new_value = constraint.param; // 使用 DEFAULT 值
//                            break;
//                        }
//                        case 7: { // AUTO_INCREMENT 约束
//                            static int auto_increment_value = 1; // 自增值，从1开始
//                            new_value = std::to_string(auto_increment_value++);
//                            break;
//                        }
//                        default:
//                            break;
//                        }
//                    }
//                }
//
//                // 将默认值或自增值填入记录
//                record.push_back(new_value);
//                m_fields.push_back(updated_field); // 更新表的字段信息
//            }
//        }
//
//        //// 更新字段：fields 和 m_fields 中都有的字段需要更新
//        for (size_t i = 0; i < fields.size(); ++i) {
//            FieldBlock& updated_field = fields[i];
//
//            // 查找 fields 中的字段是否在 m_fields 中存在
//            auto it = std::find_if(m_fields.begin(), m_fields.end(), [&](const FieldBlock& f) {
//                return std::string(f.name) == updated_field.name;
//                });
//
//            if (it != m_fields.end()) {
//                // 如果在 m_fields 中找到了对应字段
//                size_t field_index = std::distance(m_fields.begin(), it);
//
//                // 获取当前字段的约束
//                FieldBlock& old_field = m_fields[field_index];
//                bool is_integrity_changed = (old_field.integrities != updated_field.integrities);
//
//                // 如果约束发生变化，则需要进一步检查是 DEFAULT、NOT NULL 或 AUTO_INCREMENT 变化
//                if (is_integrity_changed) {
//                    // 检查约束变化：DEFAULT、NOT NULL 和 AUTO_INCREMENT
//                    std::string new_value; // 获取字段的新值
//
//                    for (const auto& constraint : m_constraints) {
//                        if (constraint.field == updated_field.name) {
//                            switch (constraint.type) {
//                            case 6: { // DEFAULT 约束
//                                if (new_value == "NULL" || new_value.empty()) {
//                                    new_value = constraint.param; // 使用 DEFAULT 值
//                                }
//                                break;
//                            }
//                            case 5: { // NOT NULL 约束
//                                if (new_value == "NULL" || new_value.empty()) {
//                                    new_value = "0"; // 使用空值
//                                }
//                                break;
//                            }
//                            case 7: { // AUTO_INCREMENT 约束
//                                static int auto_increment_value = 1; // 假设从 1 开始自增
//                                if (new_value == "NULL" || new_value.empty()) {
//                                    new_value = std::to_string(auto_increment_value++); // 自增处理
//                                }
//                                break;
//                            }
//                            default:
//                                break;
//                            }
//                        }
//                    }
//
//                    // 更新字段值
//                    record[field_index] = new_value;
//                }
//            }
//        }
//    }
//
//    // 清空原文件并重新写入所有更新后的记录
//    file.close();
//    file.open(m_trd, std::ios::out | std::ios::trunc | std::ios::binary);
//    if (!file) {
//        throw std::runtime_error("无法重新打开表文件 '" + m_tableName + ".trd' 进行写入。");
//    }
//
//    // 重新写入更新后的记录
//    for (const auto& record : all_records) {
//        for (size_t i = 0; i < m_fields.size(); ++i) {
//            const FieldBlock& field = m_fields[i];
//            const std::string& value = record[i];
//
//            // 写入NULL标志
//            bool is_null = (value == "NULL");
//            file.write(reinterpret_cast<const char*>(&is_null), sizeof(bool));
//
//            if (!is_null) {
//                switch (field.type) {
//                case 1: { // INTEGER
//                    int int_val = std::stoi(value);
//                    file.write(reinterpret_cast<const char*>(&int_val), sizeof(int));
//                    break;
//                }
//                case 2: { // DOUBLE
//                    double double_val = std::stod(value);
//                    file.write(reinterpret_cast<const char*>(&double_val), sizeof(double));
//                    break;
//                }
//                case 3: { // VARCHAR
//                    char* buffer = new char[field.param];
//                    std::memset(buffer, 0, field.param);
//
//                    if (strncpy_s(buffer, field.param, value.c_str(), field.param - 1) != 0) {
//                        throw std::runtime_error("字符串复制失败。");
//                    }
//
//                    file.write(buffer, field.param);
//                    delete[] buffer;
//                    break;
//                }
//                case 4: { // BOOL
//                    bool bool_val = (value == "true" || value == "1");
//                    file.write(reinterpret_cast<const char*>(&bool_val), sizeof(bool));
//                    break;
//                }
//                case 5: { // DATETIME
//                    std::time_t now = std::time(nullptr); // 简化为当前时间
//                    file.write(reinterpret_cast<const char*>(&now), sizeof(std::time_t));
//                    break;
//                }
//                default:
//                    throw std::runtime_error("未知字段类型");
//                }
//            }
//        }
//    }
//
//    file.close();
//    std::cout << "记录更新成功。" << std::endl;
//}

void Table::updateRecord_add(FieldBlock& new_field) {
    // 1. 检查表是否存在
    if (!isTableExist()) {
        throw std::runtime_error("表 '" + m_tableName + "' 不存在。");
    }

    // 2. 检查原表数据是否能正确读取
    std::vector<std::unordered_map<std::string, std::string>> records;
    try {
        records = Record::read_records(m_tableName);
    }
    catch (const std::exception& e) {
        throw std::runtime_error("读取表记录失败：" + std::string(e.what()));
    }
    print_records(records);
    qDebug() << "Step 2: 初始读取 records 完成，记录数量:" << records.size();

    // 3. 找默认值
    std::string default_value = "NULL";
    for (const auto& constraint : m_constraints) {
        if (constraint.type == 6 && constraint.field == new_field.name) {
            default_value = constraint.param;
            break;
        }
    }
    qDebug() << "Step 3: 找到新字段默认值:" << QString::fromStdString(default_value);

    // 4. 给每条记录加上新字段
    for (size_t i = 0; i < records.size(); ++i) {
        auto& record = records[i];

        auto result = record.insert({ std::string(new_field.name), default_value });
        if (!result.second) {
            throw std::runtime_error("字段已存在，不能插入！");
        }
        // 打印每条记录被修改后的内容
        qDebug() << "Step 4: 插入新字段后，第" << i << "条记录内容：";
        for (const auto& pair : record) {
            qDebug() << "    " << QString::fromStdString(pair.first) << ":" << QString::fromStdString(pair.second);
        }
    }

    // 5. 准备更新后的字段列表（注意：不改 m_fields）
    std::vector<FieldBlock> updated_fields = m_fields;
    updated_fields.push_back(new_field);
    qDebug() << "Step 5: updated_fields 准备完成，字段总数:" << updated_fields.size();

    // 6. 打开文件（清空）准备重写
    std::ofstream file(m_trd, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开表文件 '" + m_tableName + ".trd' 进行重写。");
    }

    size_t record_count_written = 0;

    // 7. 逐条写回数据
    for (size_t i = 0; i < records.size(); ++i) {
        const auto& record = records[i];
        qDebug() << "Step 7: 写回第" << i << "条记录：";
        for (const auto& fieldBlock : updated_fields) {
            auto it = record.find(std::string(fieldBlock.name));
            const std::string& value = it->second;
            Record::write_field(file, fieldBlock, value);
            qDebug() << "    字段" << QString::fromStdString(fieldBlock.name)
                << "值" << QString::fromStdString(value);
        }
        ++record_count_written;
    }

    file.flush();
    if (!file) {
        throw std::runtime_error("刷新文件失败，磁盘可能出问题！");
    }
    file.close();
    qDebug() << "Step 7: 全部记录写入完成。写入条数:" << record_count_written;

    // 8. 检查写入记录数量
    if (record_count_written != records.size()) {
        throw std::runtime_error("写入完成，但记录数量不一致！原记录：" + std::to_string(records.size()) +
            "，实际写入：" + std::to_string(record_count_written));
    }
    qDebug() << "Step 8: 校验通过，记录数量一致。";

    std::cout << "添加字段 '" << new_field.name
        << "' 并成功更新了 " << record_count_written
        << " 条记录。" << std::endl;
}



void Table::updateRecord_delete(const std::string& fieldName) {
    // 检查表是否存在
    if (!isTableExist()) {
        throw std::runtime_error("表 '" + m_tableName + "' 不存在。");
    }

    // 读取现有记录
    std::vector<std::unordered_map<std::string, std::string>> records;
    records = Record::read_records(m_tableName);

    // 检查字段是否存在于 m_fields
    auto fieldIt = std::find_if(m_fields.begin(), m_fields.end(), [&](const FieldBlock& f) {
        return fieldName == f.name;
        });
    if (fieldIt == m_fields.end()) {
        throw std::runtime_error("字段 '" + fieldName + "' 不存在，无法删除。");
    }

    // 在所有记录中删除这个字段
    for (auto& record : records) {
        record.erase(fieldName);
    }

    // 同时从 m_fields 中删除这个字段
    m_fields.erase(fieldIt);

    // 重新写入所有更新后的记录
    std::fstream file(m_trd, std::ios::out | std::ios::binary | std::ios::trunc);
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

            // 写 NULL 标志
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
                case 3: { // VARCHAR
                    char* buffer = new char[fieldBlock.param];
                    std::memset(buffer, 0, fieldBlock.param);
                    if (strncpy_s(buffer, fieldBlock.param, value.c_str(), fieldBlock.param - 1) != 0) {
                        throw std::runtime_error("字符串复制失败。");
                    }
                    file.write(buffer, fieldBlock.param);
                    delete[] buffer;
                    break;
                }
                case 4: { // BOOL
                    bool bool_val = (value == "true" || value == "1");
                    file.write(reinterpret_cast<const char*>(&bool_val), sizeof(bool));
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
    std::cout << "删除字段 '" << fieldName << "' 并更新记录成功。" << std::endl;
}




