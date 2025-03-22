#include "database.h"
#include <iostream>
#include <fstream>
#include <unordered_map>

// 构造函数：加载数据库
database::database(const std::string& db_name) : m_db_name(db_name) {
    std::cout << "Loading database: " << m_db_name << std::endl;
    loadDatabase(m_db_name + ".db");
}

// 析构函数：保存数据库
database::~database() {
    std::cout << "Saving database: " << m_db_name << std::endl;
    saveDatabase(m_db_name + ".db");
}

// 创建新表
void database::createTable(const std::string& table_name) {
    if (m_tables.find(table_name) != m_tables.end()) {
        std::cerr << "Table " << table_name << " already exists!" << std::endl;
        return;
    }
    Table* new_table = new Table();
    m_tables[table_name] = new_table;
    std::cout << "Table " << table_name << " created successfully." << std::endl;
}

// 删除表
void database::dropTable(const std::string& table_name) {
    auto it = m_tables.find(table_name);
    if (it == m_tables.end()) {
        std::cerr << "Table " << table_name << " not found!" << std::endl;
        return;
    }
    delete it->second;
    m_tables.erase(it);
    std::cout << "Table " << table_name << " dropped successfully." << std::endl;
}

// 查找表
Table* database::getTable(const std::string& table_name) {
    auto it = m_tables.find(table_name);
    if (it != m_tables.end()) {
        return it->second;
    }
    std::cerr << "Table " << table_name << " not found!" << std::endl;
    return nullptr;
}

// 从文件加载数据库数据
void database::loadDatabase(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to load database file: " << file_path << std::endl;
        return;
    }
    // 加载数据（这里只是一个示例）
    std::cout << "Database loaded from " << file_path << std::endl;
}

// 保存数据库数据到文件
void database::saveDatabase(const std::string& file_path) {
    std::ofstream file(file_path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to save database file: " << file_path << std::endl;
        return;
    }
    // 保存数据（这里只是一个示例）
    std::cout << "Database saved to " << file_path << std::endl;
}

/*
void Database::beginTransaction() {
    transaction_manager.beginTransaction();
}

void Database::commitTransaction() {
    transaction_manager.commitTransaction();
}

void Database::rollbackTransaction() {
    transaction_manager.rollbackTransaction();
}
*/