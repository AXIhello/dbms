#include "database.h"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <filesystem>

// 构造函数：加载数据库√
Database::Database(const std::string& db_name) : m_db_name(db_name) {
    std::cout << "加载数据库：" << m_db_name << std::endl;
    loadTable(m_db_name + ".db");
}

// 析构函数：保存数据库√
Database::~Database() {
    std::cout << "保存数据库： " << m_db_name << std::endl;
    saveTable(m_db_name + ".db");
}

// 创建新表√
void Database::createTable(const std::string& table_name) {
    if (m_tables.find(table_name) != m_tables.end()) {
        std::cerr << "表 " << table_name << " 已存在" << std::endl;
        return;
    }
    Table* new_table = new Table(m_db_name, table_name);
    m_tables[table_name] = new_table;

    // 保存表的元数据和定义文件
    if (!new_table->saveMetadata()) {
        std::cerr << "保存元数据失败" << std::endl;
    }

    if (!new_table->saveDefine()) {
        std::cerr << "保存表定义失败" << std::endl;
    }

    std::cout << "表 " << table_name << " 已成功创建" << std::endl;
}


// 删除表√
void Database::dropTable(const std::string& table_name) {
    auto it = m_tables.find(table_name);
    if (it == m_tables.end()) {
        std::cerr << "表 " << table_name << " 未找到" << std::endl;
        return;
    }
    delete it->second;//调用Table析构函数
    m_tables.erase(it);
    std::cout << "表 " << table_name << " 已成功删除" << std::endl;
}


// 查找表√
Table* Database::getTable(const std::string& table_name) {
    auto it = m_tables.find(table_name);
    if (it != m_tables.end()) {
        return it->second;
    }
    std::cerr << "表 " << table_name << " 未找到" << std::endl;
    return nullptr;
}

//返回调用的数据库的所有表名
std::vector<std::string> Database::getAllTableNames() const {

}
// 从文件加载数据库数据
void Database::loadTable(const std::string& table_name) {

    std::string file_path = m_db_name + ".db";
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        std::cerr << "加载数据库文件： " << file_path << " 失败" << std::endl;
        return;
    }
    // 加载数据（这里只是一个示例,要把数据表数组加载出来）
    std::cout << "加载数据库文件： " << file_path << " 成功" << std::endl;
}

// 保存表数据到文件
void Database::saveTable(const std::string& table_name) {

    // 拼接文件路径
    std::string file_path = m_db_name + ".db";

    std::ofstream file(file_path, std::ios::binary);
    if (!file) {
        std::cerr << "保存数据库文件： " << file_path << " 失败" << std::endl;
        return;
    }
    // 保存数据（这里只是一个示例，要修改tb文件）
    std::cout << "数据库已保存至 " << file_path << std::endl;
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
