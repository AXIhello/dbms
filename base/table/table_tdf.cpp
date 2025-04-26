#include "table.h"
#include <iostream>
#include <ctime>
#include "manager/parse.h"
#include <cstring>
#include <iomanip>
#include"manager/dbManager.h"

//表定义文件操作
// 加载表的定义（待验证）

void Table::loadDefineBinary() {
    std::ifstream in(m_tdf, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("无法打开定义文件: " + m_tdf);
    }

    m_fields.clear();

    while (in.peek() != EOF) {
        FieldBlock field;
        in.read(reinterpret_cast<char*>(&field), sizeof(FieldBlock));
        if (in.gcount() < sizeof(FieldBlock)) break;

        m_fields.push_back(field);
    }

    in.close();
}

void Table::saveDefineBinary() {
    std::ofstream out(m_tdf, std::ios::binary);
    if (!out.is_open()) {
        throw std::runtime_error("无法保存定义文件: " + m_tableName + " .tdf ");
    }

    for (int i = 0; i < m_fields.size(); ++i) {
        FieldBlock& field = m_fields[i];

        // 填充必要的字段信息
        field.order = i; // 设置字段的顺序
        field.mtime = std::time(nullptr); // 设置最后修改时间为当前时间
        field.integrities = 0; //

        // 将当前字段块写入文件
        out.write(reinterpret_cast<const char*>(&field), sizeof(FieldBlock));
    }

    out.close();
}

FieldBlock* Table::getFieldByName(const std::string& fieldName) const{
    for (const auto& field : m_fields) {
        if (field.name == fieldName) {
			FieldBlock* fieldPtr = new FieldBlock(field);
            return fieldPtr;
        }
    }
    throw std::runtime_error("字段 " + fieldName + " 不存在！");
}
//字段操作
void Table::addField(const FieldBlock& field) {
    FieldBlock newField = field;

    // 设置字段顺序为当前字段数
    newField.order = m_fields.size();

    // 设置最后修改时间
    newField.mtime = std::time(nullptr);

    // 更新记录
	//updateRecord_add(newField); 

    // 添加到字段列表
    m_fields.push_back(newField);

    // 更新字段数
    m_fieldCount = m_fields.size();

    // 更新时间戳
    m_lastModifyTime = std::time(nullptr);

    // 保存到定义文件和元数据文件
    saveDefineBinary();
    saveMetadataBinary();
}

//待实现。不管有没有值，都直接删
void Table::dropField(const std::string fieldName) {
    //先查找是否有该字段
    auto fieldNames = getFieldNames();
    if (std::find(fieldNames.begin(), fieldNames.end(), fieldName) == fieldNames.end()) {
        throw std::runtime_error("字段 '" + fieldName + "' 不存在。");
    }
    //
    auto fieldIt = std::find_if(m_fields.begin(), m_fields.end(),
        [&fieldName](const FieldBlock& field) { return field.name == fieldName; });

    if (fieldIt != m_fields.end()) {
        m_fields.erase(fieldIt);  // 从字段容器中移除该字段
    }

    //完整性约束还没更新
    saveDefineBinary(); // 保存到定义文件
    saveMetadataBinary(); // 保存到元数据文件
}

void Table::updateField(const std::string fieldName, const FieldBlock& updatedField) {
    // 查找字段
    auto it = std::find_if(m_fields.begin(), m_fields.end(), [&](const FieldBlock& field) {
        return field.name == fieldName;
        });
    if (it != m_fields.end()) {
        *it = updatedField;
        saveDefineBinary(); // 保存到定义文件
        m_lastModifyTime = std::time(nullptr); // 更新时间戳
    }
}

void Table::renameField(const std::string oldName, const std::string newName) {
    // 查找字段
    auto it = std::find_if(m_fields.begin(), m_fields.end(), [&](const FieldBlock& field) {
        return field.name == oldName;
        });
    if (it != m_fields.end()) {
        strncpy_s(it->name, newName.c_str(), sizeof(it->name) - 1); // 更新字段名
        saveDefineBinary(); // 保存到定义文件
        m_lastModifyTime = std::time(nullptr); // 更新时间戳
    }
}
