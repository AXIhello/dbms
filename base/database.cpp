//建表语句待完善；
#include "database.h"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <filesystem>
#include "manager/dbManager.h"

// 构造函数：加载数据库√
Database::Database(const std::string& db_name) {
    m_db_name = db_name;
    m_db_path = dbManager::basePath + "/data/" + db_name + "/" + db_name;
    std::cout << "加载数据库：" << m_db_path << std::endl;
    loadTable(m_db_path + ".db");
}


// 析构函数：保存数据库√
Database::~Database() {
    std::cout << "保存数据库： " << m_db_name << std::endl;
    saveTable(m_db_name + ".db");
}

// 创建新表√
void Database::createTable(const std::string& table_name, const std::vector<Table::Column>& columns) {
    if (m_tables.find(table_name) != m_tables.end()) {
        throw std::runtime_error("表 " + table_name + " 已存在");
    }

    Table* new_table = new Table(m_db_name, table_name);
    new_table->initializeNew();

    for (const auto& col : columns) {
        new_table->addCol(col);
    }

    new_table->saveDefine();
    new_table->saveMetadata();

    m_tables[table_name] = new_table;

    std::cout << "表 " << table_name << " 创建成功" << std::endl;
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
    std::vector<std::string> tableNames;
    
    return tableNames;
}
// 从文件加载数据库数据
void Database::loadTable(const std::string& table_name) {
    std::string file_path = m_db_path + ".db";

    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        std::cerr << "加载数据库文件： " << file_path << " 失败" << std::endl;
        return;
    }

    size_t tableCount = 0;
    file.read(reinterpret_cast<char*>(&tableCount), sizeof(tableCount));

    for (size_t i = 0; i < tableCount; ++i) {
        size_t len = 0;
        file.read(reinterpret_cast<char*>(&len), sizeof(len));

        std::string tableName(len, '\0');
        file.read(&tableName[0], len);

        Table* table = new Table(m_db_name, tableName);
        table->loadDefine(); // 假设你有这个方法
        m_tables[tableName] = table;
    }

    std::cout << "数据库结构加载成功，共 " << tableCount << " 张表" << std::endl;
}


// 保存表数据到文件
void Database::saveTable(const std::string& table_name) {
    // 拼接完整路径：<base>/data/<dbName>/<dbName>.db
    std::string file_path = m_db_path + ".db";

    std::ofstream file(file_path, std::ios::binary);
    if (!file) {
        std::cerr << "保存数据库文件： " << file_path << " 失败" << std::endl;
        return;
    }

    // 保存数据库表结构：这里只是演示，写入所有表名
    size_t tableCount = m_tables.size();
    file.write(reinterpret_cast<const char*>(&tableCount), sizeof(tableCount));

    for (const auto& pair : m_tables) {
        const std::string& name = pair.first;
        size_t len = name.size();
        file.write(reinterpret_cast<const char*>(&len), sizeof(len));
        file.write(name.c_str(), len);
    }

    std::cout << "数据库结构已保存至 " << file_path << std::endl;
}


std::string Database::getDBPath() const
{
	return m_db_path;
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
