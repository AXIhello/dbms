#include "table.h"
#include <iostream>
#include <ctime>
#include "manager/parse.h"
#include <cstring>
#include <iomanip>
#include"manager/dbManager.h"

//表完整性文件操作
void Table::loadIntegrityBinary() {
    // 打开 .tic 文件进行二进制读取
    std::ifstream in(m_tic, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("无法打开完整性文件: " + m_tic);
    }

    // 检查文件是否为空，若为空则直接返回
    in.seekg(0, std::ios::end);
    if (in.tellg() == 0) {
        in.close();
        return;  // 如果文件为空，直接返回
    }

    // 清空已有的约束数据
    m_constraints.clear();

    // 读取每个 ConstraintBlock 到 m_constraints 中
    ConstraintBlock constraint;
    while (in.read(reinterpret_cast<char*>(&constraint), sizeof(ConstraintBlock))) {
        m_constraints.push_back(constraint);
    }

    // 关闭文件
    in.close();
}

void Table::saveIntegrityBinary() {
    std::ofstream out(m_tic, std::ios::binary);
    if (!out.is_open()) {
        throw std::runtime_error("无法保存完整性文件: " + m_tableName + " .tic ");
    }

    // 遍历字段和对应的约束
    for (int i = 0; i < m_constraints.size(); ++i) {
        // 直接保存有效的约束
        out.write(reinterpret_cast<const char*>(&m_constraints[i]), sizeof(ConstraintBlock));
    }

    out.close();
}

void Table::addForeignKey(const std::string& constraintName,
    const std::string& foreignKeyField,
    const std::string& referenceTable,
    const std::string& referenceField) {
    ConstraintBlock cb{};
    cb.type = 2;  // 外键类型编号

    // 设置约束名
    strncpy_s(cb.name, constraintName.c_str(), sizeof(cb.name) - 1);

    // 检查外键字段是否存在于当前表中
    bool fieldExists = false;
    for (const FieldBlock& f : m_fields) {
        if (f.name == foreignKeyField) {
            fieldExists = true;
            break;
        }
    }
    if (!fieldExists) {
        throw std::runtime_error("外键字段不存在于当前表中: " + foreignKeyField);
    }
    //检查引用表和字段是否存在(字段检查未完成）
    Database* db = dbManager::getInstance().getCurrentDatabase();
    // 确保引用表已加载到内存中
    Table* refTable = db->getTable(referenceTable);
    if (!refTable) {
        throw std::runtime_error("无法加载引用表: " + referenceTable);
    }

    // 检查引用字段是否存在
    std::vector<std::string> refColNames = refTable->getFieldNames();
    bool m_fieldExists = false;
    for (const std::string& colName : refColNames) {
        if (colName == referenceField) {
            m_fieldExists = true;
            break;
        }
    }

    if (!m_fieldExists) {
        throw std::runtime_error("引用字段不存在于表 " + referenceTable + ": " + referenceField);
    }

    // 设置字段名
    strncpy_s(cb.field, foreignKeyField.c_str(), sizeof(cb.field) - 1);

    // 设置参数为 引用表.字段
    std::string ref = referenceTable + "." + referenceField;
    strncpy_s(cb.param, ref.c_str(), sizeof(cb.param) - 1);

    // 添加约束
    m_constraints.push_back(cb);

    // 保存约束和元数据
    saveIntegrityBinary();
    m_lastModifyTime = std::time(nullptr);
    saveMetadataBinary();
}

//仅处理表级check, priamry key, unique约束
void Table::addConstraint(const std::string& constraintName,
    const std::string& constraintType,
    const std::string& constraintBody) {

    ConstraintBlock cb{};
    strncpy_s(cb.name, constraintName.c_str(), sizeof(cb.name) - 1);

    // 解析约束类型并设置约束参数
    if (constraintType == "PRIMARY KEY") {
        cb.type = 1;

        //检验字段是否存在；先分割（如果多个）
        std::vector<std::string> fields;
        std::istringstream ss(constraintBody);
        std::string field;
        while (std::getline(ss, field, ',')) {
            field = Parse::trim(field);  // 去除空格
            fields.push_back(field);
        }

        for (const std::string& f : fields) {
            bool fieldExists = false;
            for (const FieldBlock& field : m_fields) {
                if (field.name == f) {
                    fieldExists = true;
                    break;
                }
            }
            if (!fieldExists) {
                throw std::runtime_error("字段不存在于表中: " + f);
            }
        }
        //field中先存着吧
        strncpy_s(cb.field, constraintBody.c_str(), sizeof(cb.param) - 1);
        strncpy_s(cb.param, constraintBody.c_str(), sizeof(cb.param) - 1);

    }

    else if (constraintType == "CHECK") {
        cb.type = 3;

        std::string trimmedBody = Parse::trim(constraintBody);

        // 2. 查找所有可能的字段
        std::vector<std::string> fields;

        // 正则表达式匹配字段名，假设字段名是由字母、数字或下划线组成
        std::regex fieldRegex(R"(\b([a-zA-Z_][a-zA-Z0-9_]*)\b)");

        auto wordsBegin = std::sregex_iterator(trimmedBody.begin(), trimmedBody.end(), fieldRegex);
        auto wordsEnd = std::sregex_iterator();

        // 提取所有字段名
        for (std::sregex_iterator i = wordsBegin; i != wordsEnd; ++i) {
            std::string field = i->str();
            field = Parse::trim(field);  // 去除字段前后的空格
            if (!field.empty()) {
                fields.push_back(field);
            }
        }

        // 3. 检查字段是否存在于表中
        for (const std::string& field : fields) {
            bool fieldExists = false;
            for (const FieldBlock& fb : m_fields) {
                if (fb.name == field) {
                    fieldExists = true;
                    break;
                }
            }
            if (!fieldExists) {
                throw std::runtime_error("字段不存在于表中: " + field);
            }
        }

        strncpy_s(cb.param, trimmedBody.c_str(), sizeof(cb.param) - 1);

    }
    else if (constraintType == "UNIQUE") {
        cb.type = 4;
        std::string field = Parse::trim(constraintBody);  // 只有一个字段

        // 检查字段是否存在于表中
        bool fieldExists = false;
        for (const FieldBlock& f : m_fields) {
            if (f.name == field) {
                fieldExists = true;
                break;
            }
        }
        if (!fieldExists) {
            throw std::runtime_error("字段不存在于表中: " + field);
        }
        //向tic文件中检查是否已有约束？

        strncpy_s(cb.field, constraintBody.c_str(), sizeof(cb.param) - 1);
        strncpy_s(cb.param, constraintBody.c_str(), sizeof(cb.param) - 1);
    }
    else {
        throw std::runtime_error("未知约束类型 '" + constraintType + "'。");
    }


    m_constraints.push_back(cb);

    // 保存约束到文件
    saveIntegrityBinary();  // 保存到完整性文件
    m_lastModifyTime = std::time(nullptr);  // 更新时间戳
    saveMetadataBinary(); // 保存定义文件和元数据文件
}

void Table::addConstraint(const ConstraintBlock& constraint) {
    // 将约束添加到表的约束列表中
    m_constraints.push_back(constraint);

    // 更新时间戳
    m_lastModifyTime = std::time(nullptr);
    // 将所有约束保存到 .tic 文件中
    saveIntegrityBinary();

    // 保存定义文件和元数据文件
    /*saveDefineBinary();
    saveMetadataBinary();*/
}

void Table::dropConstraint(const std::string constraintName) {
    auto it = std::remove_if(m_constraints.begin(), m_constraints.end(), [&](const ConstraintBlock& constraint) {
        return constraint.name == constraintName;
        });
}

void Table::updateConstraint(const std::string constraintName, const ConstraintBlock& updatedConstraint) {
    // 查找约束
    auto it = std::find_if(m_constraints.begin(), m_constraints.end(), [&](const ConstraintBlock& constraint) {
        return constraint.name == constraintName;
        });
    if (it != m_constraints.end()) {
        *it = updatedConstraint;
        saveIntegrityBinary(); // 保存到完整性文件
        m_lastModifyTime = std::time(nullptr); // 更新时间戳
    }
}