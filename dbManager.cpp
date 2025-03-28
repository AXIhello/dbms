#include "dbManager.h"
#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

// 构造函数
dbManager::dbManager() {
    // 默认将 DBMS_ROOT 设置在当前工程文件夹下（便于测试）
    basePath = std::filesystem::current_path().string() + "/DBMS_ROOT";

    if (!fs::exists(basePath)) {
        fs::create_directory(basePath);
        std::cout << "创建 DBMS 根目录: " << basePath << std::endl;
    }

    // 确保系统数据库文件 ruanko.db 存在
    if (!fs::exists(basePath + "/" + systemDBFile)) {
        createSystemDBFile();
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

// 初始化系统数据库（创建 ruanko.db 文件）
void dbManager::createSystemDBFile() {
    std::ofstream sysDBFile(basePath + "/" + systemDBFile, std::ios::binary);
    if (!sysDBFile) {
        std::cerr << "无法创建系统数据库文件 ruanko.db" << std::endl;
        return;
    }

    // 写入初始数据
    sysDBFile << "system_database=1" << std::endl;  // 1 表示系统数据库
    sysDBFile << "database_name=ruanko" << std::endl;
    sysDBFile << "path=" + basePath + "/data" << std::endl;

    sysDBFile.close();
    std::cout << "系统数据库文件 'ruanko.db' 已创建并初始化。" << std::endl;
}

// 加载系统数据库信息
void dbManager::loadSystemDBInfo() {
    std::ifstream sysDBFile(basePath + "/" + systemDBFile, std::ios::binary);
    if (!sysDBFile) {
        std::cerr << "无法读取系统数据库文件 ruanko.db" << std::endl;
        return;
    }

    std::string line;
    while (std::getline(sysDBFile, line)) {
        if (line.find("database_name=") == 0) {
            std::cout << "系统数据库名: " << line.substr(14) << std::endl;
        }
        if (line.find("path=") == 0) {
            std::cout << "数据库存放路径: " << line.substr(5) << std::endl;
        }
    }

    sysDBFile.close();
}

// 创建数据库
void dbManager::createDatabase(const std::string& db_name) {
    std::string dbDir = basePath + "/data/" + db_name;

    if (fs::exists(dbDir)) {
        std::cerr << "数据库 " << db_name << " 已存在！" << std::endl;
        return;
    }

    createDatabaseFolder(db_name);

    createDatabaseFiles(db_name);

    std::cout << "数据库 " << db_name << " 创建成功！" << std::endl;
}


void dbManager::dropDatabase(const std::string& db_name) {
 
    if (db_name == "ruanko") {
        std::cerr << "系统数据库 ruanko 不能被删除！" << std::endl;
        return;
    }

    std::string dbDir = basePath + "/data/" + db_name;

    if (!fs::exists(dbDir)) {
        std::cerr << "数据库 " << db_name << " 不存在！" << std::endl;
        return;
    }

    // 删除数据库文件夹及文件
    deleteDatabaseFolder(db_name);

    std::cout << "数据库 " << db_name << " 已删除！" << std::endl;
}


void dbManager::listDatabases() {
    bool found = false;
    for (const auto& entry : fs::directory_iterator(basePath + "/data")) {
        if (fs::is_directory(entry.path())) {
            std::cout << "数据库: " << entry.path().filename().string() << std::endl;
            found = true;
        }
    }

    if (!found) {
        std::cout << "没有任何数据库存在。" << std::endl;
    }
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
    tbFile << "Table Descriptions for " << db_name << std::endl;
    tbFile.close();

    // 创建日志文件 (.log)
    std::ofstream logFile(dbDir + "/" + db_name + ".log");
    logFile << "Log File for " << db_name << std::endl;
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
