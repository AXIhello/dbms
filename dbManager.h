#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include"database.h"
#include <unordered_map>
#include <chrono>  // 添加此行以包含 std::chrono

namespace fs = std::filesystem;

struct DatabaseBlock {
    char dbName[128];  // 修正数组声明
    bool type;  // 0: 系统数据库；1: 用户数据库
    char filename[259];  // 修正数组声明
    time_t crtime;  // 创建时间
};

class dbManager {
public:
    static dbManager& getInstance(); // 获取全局唯一实例
    Database* getCurrentDatabase();

    void createDatabaseFolder(const std::string& db_name);  // 创建数据库文件夹
    void createDatabaseFiles(const std::string& db_name);  // 创建数据库文件（tb, log）
    void deleteDatabaseFolder(const std::string& db_name);  // 删除数据库文件夹

    void saveDatabaseInfo(const std::string& dbName, const std::string& dbPath);  // 保存数据库信息到系统数据库文件
    void removeDatabaseInfo(const std::string& db_name);  // 从系统数据库文件中移除数据库信息

    void createSystemDB();  // 创建系统数据库文件
    void loadSystemDBInfo();  // 加载系统数据库信息

    bool databaseExists(const std::string& db_name);  // 检查数据库是否存在
    // 数据库连接
   

    void createUserDatabase(const std::string& db_name);  
    void dropDatabase(const std::string& db_name); 
    
    void useDatabase(const std::string& db_name);  // 切换数据库

    //void listDatabases(); 
    std::vector<std::string> getDatabaseList();
    bool isConnected();//待扩展与实现；

private:
    dbManager();  // DBMS_ROOT根目录
    ~dbManager();
    dbManager(const dbManager&) = delete;
    dbManager& operator=(const dbManager&) = delete;

   
	
private:
    std::string basePath;  // 根目录
    const std::string systemDBFile = "ruanko.db";  // 系统数据库文件名(后期可更改）
    Database* currentDB = nullptr;
    std::string currentDBName;

};

#endif // DBMANAGER_H
