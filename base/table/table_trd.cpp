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
void Table::print_records(const std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>>& records) {
    std::cout << "\n====== 当前读取到的记录内容 ======" << std::endl;
    size_t index = 0;
    for (const auto& [row_id, record_data] : records) {
        std::cout << "记录 " << index++ << " (row_id=" << row_id << "): { ";
        for (const auto& [key, value] : record_data) {
            std::cout << key << ": " << value << ", ";
        }
        std::cout << "}" << std::endl;
    }
    std::cout << "====== 记录总数: " << records.size() << " ======" << std::endl;
}


void Table::updateRecord_add(FieldBlock& new_field) {
    if (!isTableExist()) {
        throw std::runtime_error("表 '" + m_tableName + "' 不存在。");
    }

    auto records = Record::read_records(m_tableName);
    print_records(records);

    std::string default_value = "NULL";
    for (const auto& constraint : m_constraints) {
        if (constraint.type == 6 && constraint.field == new_field.name) {
            default_value = constraint.param;
            break;
        }
    }

    // 添加新字段到每条记录
    for (auto& [row_id, record] : records) {
        auto result = record.insert({ new_field.name, default_value });
        if (!result.second) {
            throw std::runtime_error("字段 '" + std::string(new_field.name) + "' 已存在，不能添加。");
        }
    }

    // 构造新的字段列表（不修改 m_fields）
    std::vector<FieldBlock> updated_fields = m_fields;
    updated_fields.push_back(new_field);

    std::ofstream file(m_trd, std::ios::binary | std::ios::trunc);
    if (!file) {
        throw std::runtime_error("无法打开表文件 '" + m_tableName + ".trd'");
    }

    size_t written_count = 0;
    for (const auto& [row_id, record] : records) {
        char delete_flag = 0;
        file.write(&delete_flag, sizeof(char));

        for (const auto& fieldBlock : updated_fields) {
            const std::string& value = record.at(fieldBlock.name);
            Record::write_field(file, fieldBlock, value);
        }
        ++written_count;
    }

    file.close();
    if (written_count != records.size()) {
        throw std::runtime_error("写入记录数量不一致！");
    }

    std::cout << "字段 '" << new_field.name << "' 添加成功，共更新 " << written_count << " 条记录。" << std::endl;
}

void Table::updateRecord_delete(const std::string& fieldName) {
    if (!isTableExist()) {
        throw std::runtime_error("表 '" + m_tableName + "' 不存在。");
    }

    auto full_records = Record::read_records(m_tableName);

    auto it = std::find_if(m_fields.begin(), m_fields.end(), [&](const FieldBlock& f) {
        return f.name == fieldName;
        });
    if (it == m_fields.end()) {
        throw std::runtime_error("字段 '" + fieldName + "' 不存在。");
    }

    m_fields.erase(it);

    // 从每条记录中移除该字段
    for (auto& [row_id, record] : full_records) {
        record.erase(fieldName);
    }

    std::ofstream file(m_trd, std::ios::binary | std::ios::trunc);
    if (!file) {
        throw std::runtime_error("无法打开文件 '" + m_tableName + ".trd' 进行写入。");
    }

    for (const auto& [row_id, record] : full_records) {
        char delete_flag = 0;
        file.write(&delete_flag, sizeof(char));

        for (const auto& fieldBlock : m_fields) {
            auto it = record.find(fieldBlock.name);
            if (it == record.end()) {
                throw std::runtime_error("字段 '" + fieldName + "' 缺失对应值。");
            }
            Record::write_field(file, fieldBlock, it->second);
        }
    }

    file.close();
    std::cout << "字段 '" << fieldName << "' 删除成功，记录已更新。" << std::endl;
}





