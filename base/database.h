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

    //从.db文件中加载数据库信息
	void loadDatabase(const std::string& db_name);
    // 创建表
   // void createTable(const std::string& table_name);
	// 创建表（带字段定义）
    void createTable(const std::string& table_name, const std::vector<FieldBlock>& fields);


    // 删除表
    void dropTable(const std::string& table_name);

    // 查找表
    Table* getTable(const std::string& table_name);

    //返回指定数据库的所有表名
    std::vector<std::string> getAllTableNames() const;
    std::string getDBPath() const;

    // 加载和保存数据库 
    void loadTable(const std::string& table_name);
    void saveTable(const std::string& table_name);

	std::string getDBName() {
		return m_db_name;
	}

private:
    std::string m_db_name;   // 数据库名称
    std::string m_db_path;//数据库路径
    int m_db_type;//数据库类型：0 系统 ；1 用户
    //std::string m_db_file = m_db_name + ".tb";
    std::map<std::string, Table*> m_tables;  // 存储所有表
    //TransactionManager m_transaction_manager;  // 事务管理器
    std::ofstream m_log_file; // 操作日志文件
    time_t m_create_time; // 创建时间
};

#endif // DATABASE_H
