//TODO: ①createUserDatabase()待修改，从.db文件中读取数据，而非直接根据文件路径判断
#include "dbManager.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include<Windows.h>
#include "base/user.h"

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
        create_system_db();
    }

    load_system_db_info();

    // 创建存储目录
    if (!fs::exists(basePath + "/data")) {
        fs::create_directory(basePath + "/data");
        std::cout << "创建数据存储目录: " << basePath + "/data" << std::endl;
    }
}

dbManager::~dbManager() {
    
}


// 初始化系统数据库（创建并写入 ruanko.db 文件）
void dbManager::create_system_db() {
    std::string sysDBPath = basePath + "/" + systemDBFile;
   
    std::ofstream sysDBFile(sysDBPath, std::ios::binary);
    if (!sysDBFile) {
        throw std::runtime_error("无法创建系统数据库文件 ruanko.db");
    }
 //   1
    sysDBFile.close();
}

void dbManager::save_database_info(const std::string& dbName, const std::string& dbPath, const std::string& abledUsername) {
    std::string sysDBPath = basePath + "/" + systemDBFile;
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

    // 设置创建者用户名
    strncpy_s(dbInfo.abledUsername, abledUsername.c_str(), sizeof(dbInfo.abledUsername) - 1);//√

    sysDBFile.write(reinterpret_cast<const char*>(&dbInfo), sizeof(DatabaseBlock));
    sysDBFile.close();
}

void dbManager::remove_database_info(const std::string& db_name)
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


void dbManager::load_system_db_info() {
    std::string sysDBPath = basePath + "/" + systemDBFile;
    std::ifstream sysDBFile(sysDBPath, std::ios::binary);
    if (!sysDBFile) {
        throw std::runtime_error("无法读取系统数据库文件 ruanko.db");
    }

    DatabaseBlock dbInfo;
    int count = 0;
    while (sysDBFile.read(reinterpret_cast<char*>(&dbInfo), sizeof(DatabaseBlock))) {
        std::string dbName(dbInfo.dbName, strnlen(dbInfo.dbName, sizeof(dbInfo.dbName)));
        std::string dbPath(dbInfo.filepath, strnlen(dbInfo.filepath, sizeof(dbInfo.filepath)));
        //std::string typeStr = dbInfo.type ? Utf8ToGbk1("用户数据库") : Utf8ToGbk1("系统数据库");
        std::string typeStr = dbInfo.type ? ("用户数据库") : ("系统数据库");

        // 确保 crtime 有效
        std::time_t t = dbInfo.crtime;
        if (t < 0 || t > std::time_t(2147483647)) { // 假设最大时间戳是 2147483647（2011年 8月 13日）
            std::cerr << "Invalid timestamp: " << t << ", skipping this record." << std::endl;
            continue; // 跳过无效时间戳的记录
        }

        char timebuf[64] = { 0 };
        std::tm tm_buf;
        if (localtime_s(&tm_buf, &t) != 0) {  // 如果 localtime_s 失败，跳过这条记录
            std::cerr << "localtime_s failed for timestamp: " << t << std::endl;
            continue;
        }

        std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm_buf);
        ++count;
    }

    sysDBFile.close();

    if (count == 0) {
        //std::cout << Utf8ToGbk1("系统数据库文件中没有任何数据库记录。") << std::endl;
        std::cout << "系统数据库文件中没有任何数据库记录。" << std::endl;
    }
}
// 创建数据库
void dbManager::create_user_db(const std::string& db_name) {
    if (db_name.length() > 128) {
        throw std::runtime_error("数据库名过长（不能超过128个字符）: " + db_name);
    }

    if (database_exists_on_disk(db_name)) {
        throw std::runtime_error("数据库 '" + db_name + "' 已存在！");
    }

    std::string db_path = basePath + "/data/" + db_name; // 到数据库文件夹为止
    create_database_folder(db_name);
    create_database_files(db_name);

    // 获取当前用户的用户名
    user u;
    std::string abledUsername = u.getCurrentUser().username;

    save_database_info(db_name, db_path, abledUsername);
    qDebug() << QString::fromStdString(abledUsername);

}


void dbManager::delete_user_db(const std::string& db_name) {
 
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
	remove_database_info(db_name);


    // 删除数据库文件夹及文件
    delete_database_folder(db_name);

    std::cout << "数据库 " << db_name << " 已删除！" << std::endl;
}

//获取数据库列表（逻辑层）
std::vector<std::string> dbManager::get_database_list_by_db()
{
    std::vector<std::string> databases;
    std::string sysDBPath = basePath + "/" + systemDBFile;
    std::ifstream file(sysDBPath, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("无法打开数据库文件 ruanko.db");
    }

    DatabaseBlock block{};
    //user u;
    while (file.read(reinterpret_cast<char*>(&block), sizeof(block))) {
        if (block.type != 1) continue; // 只处理用户数据库

        const char* currentUser = user::getCurrentUser().username;

        // 如果是 sys 用户，显示所有数据库
        if (strcmp(currentUser, "sys") == 0) {
            databases.emplace_back(block.dbName);
        }
        else {
            // 如果是创建者，或在授权用户名列表中
            if (strcmp(block.abledUsername, currentUser) == 0 ||
                std::string(block.abledUsername).find(currentUser) != std::string::npos) {
                databases.emplace_back(block.dbName);
            }
        }
        //// 只加载当前用户创建的数据库
        //if (block.type == 1 && strcmp(block.abledUsername, user::getCurrentUser().username) == 0) {  // 用户数据库
        //    databases.emplace_back(block.dbName);
        //    //qDebug() << "比对成功: " << block.abledUsername << " <-> " << user::getCurrentUser().username;

        //}
    }
    file.close();
    return databases;
}


// 创建数据库文件夹
void dbManager::create_database_folder(const std::string& db_name) {
    std::string dbDir = basePath + "/data/" + db_name;
    fs::create_directory(dbDir);
}

// 创建数据库文件（tb, log）
void dbManager::create_database_files(const std::string& db_name) {
    std::string dbDir = basePath + "/data/" + db_name;

    // 创建表描述文件 (.tb)
    std::ofstream tbFile(dbDir + "/" + db_name + ".tb");
    tbFile.close();

    // 创建日志文件 (.log)
    std::ofstream logFile(dbDir + "/" + db_name + ".log");
    logFile.close();
}

// 删除数据库文件夹及文件
void dbManager::delete_database_folder(const std::string& db_name) {
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
    //std::lock_guard<std::mutex> lock(dbMutex);  // 互斥锁，独占访问
    if (!database_exists_in_db(db_name)) {
        throw std::runtime_error("数据库 '" + db_name + "' 不存在！");
    }

    LogManager::instance().initialize(db_name);

    if (currentDB) {
        delete currentDB;  // 卸载当前数据库
    }

    currentDB = new Database(db_name);  // Database 构造时自动加载
    currentDBName = db_name;
}



Database* dbManager::get_current_database() {
    std::shared_lock<std::shared_mutex> lock(dbSharedMutex);  // 读锁，允许并发读取
    if (!currentDB) {
        throw std::runtime_error("当前未选择数据库");
    }
    return currentDB;
}

/**
 * @brief用于客户端，获取指定名称的数据库对象（带缓存）.
 * 
 * 如果数据库已加载，则直接从缓存返回；
 * 否则新建一个 Database 对象，存入缓存后返回。
 * 若指定的数据库不存在，则返回 nullptr。
 * 
 * @param dbName 数据库名称
 * @return Database* 对应的数据库对象指针；若数据库不存在，返回 nullptr
 */
Database* dbManager::get_database_by_name(const std::string& dbName) {
    if (!database_exists_in_db(dbName)) return nullptr;

    auto it = dbCache.find(dbName);
    if (it != dbCache.end()) {
        return it->second;
    }

    Database* newDB = new Database(dbName);
    dbCache[dbName] = newDB;
    return newDB;
}

//释放池资源
void dbManager::clearCache() {
    for (auto& pair : dbCache) {
        delete pair.second;
    }
    dbCache.clear();
}



bool dbManager::database_exists_on_disk(const std::string& dbName) {
    std::string dbPath = basePath + "/data/" + dbName;
    return std::filesystem::exists(dbPath);  // C++17 的方式
}

bool dbManager::database_exists_in_db(const std::string& db_name) {
    std::string sysDBPath = basePath + "/" + systemDBFile;
    std::ifstream file(sysDBPath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开数据库文件 ruanko.db");
    }

    DatabaseBlock block;
    while (file.read(reinterpret_cast<char*>(&block), sizeof(DatabaseBlock))) {
        std::string name(block.dbName, strnlen(block.dbName, sizeof(block.dbName)));
        if (name == db_name) {
            file.close();
            return true;
        }
    }
    file.close();
    return false;
}