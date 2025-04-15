#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include"base/database.h"
#include <unordered_map>
#include <chrono>  // 添加此行以包含 std::chrono
#include"databaseBlock.h"
#include <unordered_map>
#include <unordered_set>



namespace fs = std::filesystem;



class dbManager {
public:
    static dbManager& getInstance(); // 获取全局唯一实例
    Database* getCurrentDatabase();
	static std::string basePath;  // 根目录

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

    // 用户管理操作
    void createUser(const std::string& username);
    bool grantPermission(const std::string&, const std::string&);
    bool revokePermission(const std::string&, const std::string&);
    bool hasPermission(const std::string& username, const std::string& permission);


private:
    dbManager();  // DBMS_ROOT根目录
    ~dbManager();
    dbManager(const dbManager&) = delete;
    dbManager& operator=(const dbManager&) = delete;

   
	
private:
    //std::string basePath;  // 根目录
    const std::string systemDBFile = "ruanko.db";  // 系统数据库文件名(后期可更改）
    Database* currentDB = nullptr;
    std::string currentDBName;

    std::unordered_map<std::string, std::unordered_set<std::string>> userPermissions;


};

#endif // DBMANAGER_H
