#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>

namespace fs = std::filesystem;

class dbManager {
public:
    dbManager();  // 初始化DBMS根目录
    ~dbManager();

    void createDatabase(const std::string& db_name);  
    void dropDatabase(const std::string& db_name); 

    void listDatabases(); 
    void initializeSystemDatabase();  // 初始化系统数据库（ruanko.db）
    bool isConnected();//待扩展与实现；

private:
    std::string basePath;  // 根目录
    const std::string systemDBFile = "ruanko.db";  // 系统数据库文件名(后期可更改）

    void createDatabaseFolder(const std::string& db_name);  // 创建数据库文件夹
    void createDatabaseFiles(const std::string& db_name);  // 创建数据库文件（tb, log）
    void deleteDatabaseFolder(const std::string& db_name);  // 删除数据库文件夹
    
    void createSystemDBFile();  // 创建系统数据库文件
    void loadSystemDBInfo();  // 加载系统数据库信息
};

#endif // DBMANAGER_H
