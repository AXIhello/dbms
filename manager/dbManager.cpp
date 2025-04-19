//TODO: ①createUserDatabase()待修改，从.db文件中读取数据，而非直接根据文件路径判断
#include "dbManager.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>

namespace fs = std::filesystem;
std::string dbManager::basePath = std::filesystem::current_path().string() + "/DBMS_ROOT";

// 构造函数
dbManager::dbManager() {


    if (!fs::exists(basePath)) {
        fs::create_directory(basePath);
        std::cout << "创建 DBMS 根目录: " << basePath << std::endl;
    }

    // 确保系统数据库文件 ruanko.db 存在
    if (!fs::exists(basePath + "/" + systemDBFile)) {
        createSystemDB();
    }

    loadSystemDBInfo();

    // 创建存储目录
    if (!fs::exists(basePath + "/data")) {
        fs::create_directory(basePath + "/data");
        std::cout << "创建数据存储目录: " << basePath + "/data" << std::endl;
    }
}

dbManager::~dbManager() {
    
}


// 初始化系统数据库（创建并写入 ruanko.db 文件）
void dbManager::createSystemDB() {
    std::string sysDBPath = basePath + "/" + systemDBFile;

    std::ofstream sysDBFile(sysDBPath, std::ios::binary);
    if (!sysDBFile) {
        throw std::runtime_error("无法创建系统数据库文件 ruanko.db");
    }
    // 写入初始数据
	DatabaseBlock systemDB;
    memset(systemDB.dbName, 0, sizeof(systemDB.dbName));
    strcpy_s(systemDB.dbName, "ruanko");  // 赋值数据库名称

    systemDB.type = false;  // false 代表系统数据库

    memset(systemDB.filepath, 0, sizeof(systemDB.filepath));
    strcpy_s(systemDB.filepath, sysDBPath.c_str());  // 赋值文件路径

    systemDB.crtime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());  // 获取当前时间

    // 写入 `ruanko.db` 文件
    sysDBFile.write(reinterpret_cast<char*>(&systemDB), sizeof(DatabaseBlock));
    sysDBFile.close();
}

void dbManager::saveDatabaseInfo(const std::string& dbName, const std::string& dbPath) {
    std::ofstream sysDBFile(basePath + "/" + systemDBFile, std::ios::binary | std::ios::app);
    if (!sysDBFile) {
        throw std::runtime_error("无法打开系统数据库文件 ruanko.db" );
    }

    DatabaseBlock dbInfo;
    memset(&dbInfo, 0, sizeof(DatabaseBlock));  // 清空结构体，避免脏数据

    strncpy_s(dbInfo.dbName, dbName.c_str(), sizeof(dbInfo.dbName) - 1);
    dbInfo.type = true;  // 默认为用户数据库
    strncpy_s(dbInfo.filepath, dbPath.c_str(), sizeof(dbInfo.filepath) - 1);
    dbInfo.crtime = std::time(nullptr);  // 记录当前时间

    sysDBFile.write(reinterpret_cast<const char*>(&dbInfo), sizeof(DatabaseBlock));
    sysDBFile.close();
}

void dbManager::removeDatabaseInfo(const std::string& db_name)
{
        std::ifstream inFile(basePath + "/" + systemDBFile, std::ios::binary);
        if (!inFile) {
            return;
        }

        std::vector<DatabaseBlock> dbRecords;
        DatabaseBlock dbInfo;

        while (inFile.read(reinterpret_cast<char*>(&dbInfo), sizeof(DatabaseBlock))) {
            if (std::string(dbInfo.dbName) != db_name) {
                dbRecords.push_back(dbInfo);  // 保存非目标数据库的记录
            }
        }
        inFile.close();

        // 将更新后的内容写回系统数据库文件
        std::ofstream outFile(basePath + "/" + systemDBFile, std::ios::binary | std::ios::trunc);
        if (!outFile) {
            return;
        }

        for (const auto& record : dbRecords) {
            outFile.write(reinterpret_cast<const char*>(&record), sizeof(DatabaseBlock));
        }
        outFile.close();
}

// 加载系统数据库信息(未完成。应该从.db文件中读取,但不知道有什么用……)
void dbManager::loadSystemDBInfo() {
    std::ifstream sysDBFile(basePath + "/" + systemDBFile, std::ios::binary);
    if (!sysDBFile) {
        throw std::runtime_error("无法读取系统数据库文件 ruanko.db");
    }


    sysDBFile.close();
}

// 创建数据库
void dbManager::createUserDatabase(const std::string& db_name) {
    if (db_name.length() > 128) {
        throw std::runtime_error("数据库名过长（不能超过128个字符）: " + db_name);
    }

    if (databaseExists(db_name)) {
        throw std::runtime_error("数据库 '" + db_name + "' 已存在！");
    }

    std::string db_path = basePath + "/data/" + db_name; // 到数据库文件夹为止
    createDatabaseFolder(db_name);
    createDatabaseFiles(db_name);
    saveDatabaseInfo(db_name, db_path);
}


void dbManager::dropDatabase(const std::string& db_name) {
 
    if (db_name == "ruanko") {
        throw std::runtime_error( "系统数据库 ruanko 不能被删除！" );
    }

    std::string dbDir = basePath + "/data/" + db_name;

    if (!fs::exists(dbDir)) {
       throw std::runtime_error( "数据库 " + db_name + " 不存在！" );
    }
    
	if (db_name == currentDBName) {
		throw std::runtime_error("当前数据库 '" + db_name + "' 正在使用中，无法删除！");
	}
 

    //从.db文件中删除
	removeDatabaseInfo(db_name);


    // 删除数据库文件夹及文件
    deleteDatabaseFolder(db_name);

    std::cout << "数据库 " << db_name << " 已删除！" << std::endl;
}



std::vector<std::string> dbManager::getDatabaseList() {
    std::vector<std::string> databases;

    for (const auto& entry : fs::directory_iterator(basePath + "/data")) {
        if (fs::is_directory(entry.path())) {
            databases.push_back(entry.path().filename().string());
        }
    }

    return databases;
}




// 创建数据库文件夹
void dbManager::createDatabaseFolder(const std::string& db_name) {
    std::string dbDir = basePath + "/data/" + db_name;
    fs::create_directory(dbDir);
}

// 创建数据库文件（tb, log）
void dbManager::createDatabaseFiles(const std::string& db_name) {
    std::string dbDir = basePath + "/data/" + db_name;

    // 创建表描述文件 (.tb)
    std::ofstream tbFile(dbDir + "/" + db_name + ".tb");
    tbFile.close();

    // 创建日志文件 (.log)
    std::ofstream logFile(dbDir + "/" + db_name + ".log");
    logFile.close();
}

// 删除数据库文件夹及文件
void dbManager::deleteDatabaseFolder(const std::string& db_name) {
    std::string dbDir = basePath + "/data/" + db_name;

    // 删除所有表文件
    for (const auto& entry : fs::directory_iterator(dbDir)) {
        fs::remove(entry.path());
    }

    // 删除数据库文件夹
    fs::remove_all(dbDir);
}

dbManager& dbManager::getInstance() {
    static dbManager instance;
    return instance;
}

void dbManager::useDatabase(const std::string& db_name) {
    if (!databaseExists(db_name)) {
        throw std::runtime_error("数据库 '" + db_name + "' 不存在！");
    }

    if (currentDB) {
        delete currentDB; //卸载再加载
    }

    currentDB = new Database(db_name);  // Database 构造时自动加载
    currentDBName = db_name;
}


Database* dbManager::getCurrentDatabase() {
    if (!currentDB) {
        throw std::runtime_error("当前未选择数据库");
    }
    return currentDB;
}


bool dbManager::databaseExists(const std::string& dbName) {
    std::string dbPath = basePath + "/data/" + dbName;
    return std::filesystem::exists(dbPath);  // C++17 的方式
}
