#pragma once

#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <map>
#include <fstream>

class Table {
    // Table类实现
};

class database {
public:
    // 构造函数和析构函数
    database(const std::string& db_name);
    ~database();

    // 创建表
    void createTable(const std::string& table_name);

    // 删除表
    void dropTable(const std::string& table_name);

    // 查找表
    Table* getTable(const std::string& table_name);


    // 加载和保存数据库
    void loadDatabase(const std::string& file_path);
    void saveDatabase(const std::string& file_path);

private:
    std::string m_db_name;   // 数据库名称
    std::map<std::string, Table*> m_tables;  // 存储所有表
    //TransactionManager m_transaction_manager;  // 事务管理器
    std::ofstream m_log_file; // 操作日志文件
};

#endif // DATABASE_H
