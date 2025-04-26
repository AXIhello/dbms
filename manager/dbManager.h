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
#include <shared_mutex>
#include <mutex>


namespace fs = std::filesystem;



class dbManager {
public:
    static dbManager& getInstance(); // 获取全局唯一实例
	
    // 禁用拷贝构造函数和赋值运算符
    dbManager(const dbManager&) = delete;
    dbManager& operator=(const dbManager&) = delete;


    Database* get_current_database();
    std::mutex dbMutex;             // 互斥锁用于保护数据库切换（写操作）
    std::shared_mutex dbSharedMutex; // 读写锁用于保护数据库读取（读操作）

    //通过文件夹来获取当前的数据
	std::vector<std::string> get_database_list_by_db(); // 通过数据库文件获取数据库列表

    //
    Database* get_database_by_name(const std::string& dbName);

	static std::string basePath;  // 根目录

    void create_database_folder(const std::string& db_name);  // 创建数据库文件夹
    void create_database_files(const std::string& db_name);  // 创建数据库文件（tb, log）
    void delete_database_folder(const std::string& db_name);  // 删除数据库文件夹

    void save_database_info(const std::string& dbName, const std::string& dbPath);  // 保存数据库信息到系统数据库文件
    void remove_database_info(const std::string& db_name);  // 从系统数据库文件中移除数据库信息

    void create_system_db();  // 创建系统数据库文件
    void load_system_db_info();  // 加载系统数据库信息

    bool database_exists_on_disk(const std::string& db_name);  // 检查数据库是否在磁盘文件中存在
	bool database_exists_in_db(const std::string& db_name);  // 检查数据库是否在ruanku.db中存在
    // 数据库连接
   

    void create_user_db(const std::string& db_name);  
    void delete_user_db(const std::string& db_name); 
    
    void useDatabase(const std::string& db_name);  // 切换数据库

    
    bool isConnected();//待扩展与实现；

    void clearCache();


private:
    dbManager();  // DBMS_ROOT根目录
    ~dbManager();
    
    
   
	
private:
    //std::string basePath;  // 根目录
    const std::string systemDBFile = "ruanko.db";  // 系统数据库文件名(后期可更改）
    
    Database* currentDB = nullptr;
    
    std::string currentDBName;
    
    std::unordered_map<std::string, Database*> dbCache;  //缓存池


};

#endif // DBMANAGER_H
