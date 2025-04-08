#pragma once

#ifndef DATABASE_H
#define DATABASE_H

#include <base/table.h>
#include <string>
#include <map>
#include <fstream>


class Database {
public:
    // 构造函数和析构函数
    Database(const std::string& db_name);
    ~Database();

	//获取所有表名
	std::vector<std::string> getAllTableNames() const {
		std::vector<std::string> tableNames;
        for (const auto& pair : m_tables) {
            tableNames.push_back(pair.first);
        }
		return tableNames;
	}

    // 创建表
    void createTable(const std::string& table_name);

    // 删除表
    void dropTable(const std::string& table_name);

    // 查找表
    Table* getTable(const std::string& table_name);

    // 加载和保存数据库
    void loadTable(const std::string& table_name);
    void saveTable(const std::string& table_name);

private:
    std::string m_db_name;   // 数据库名称
    //std::string m_db_file = m_db_name + ".tb";
    std::map<std::string, Table*> m_tables;  // 存储所有表
    //TransactionManager m_transaction_manager;  // 事务管理器
    std::ofstream m_log_file; // 操作日志文件
};

#endif // DATABASE_H
