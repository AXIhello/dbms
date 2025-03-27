#include "dbManager.h"

dbManager::dbManager() {
    // 确保默认存储目录存在
    if (!fs::exists(basePath)) {
        fs::create_directory(basePath);
        std::cout << "数据库存储目录创建: " << basePath << std::endl;
    }
    loadMetaData(); 

    if (dbs.find(systemDBName) == dbs.end()) {
        createDatabase(systemDBName);
	} // 确保系统数据库存在
}

dbManager::~dbManager() {
    for (auto& pair : dbs) {
        delete pair.second; // 释放内存
    }
    saveMetaData(); // 退出时保存数据库列表
}

// 创建数据库
void dbManager::createDatabase(const std::string& db_name) {
    if (dbs.find(db_name) != dbs.end()) {
        std::cerr << "数据库 " << db_name << " 已存在！" << std::endl;
        return;
    }
    if (db_name.length() > 128) {
        std::cerr << "数据库名称过长！" << std::endl;
        return;
    }

    std::string dbFolder = basePath + db_name + "/";  // 数据库文件夹

    std::string dbFilePath;
    if (db_name.find(".db") == std::string::npos) {
        dbFilePath = dbFolder + db_name + ".db";  // 确保只加一次 .db
    }
    else {
        dbFilePath = dbFolder + db_name;
    }

    if (!fs::exists(dbFolder)) {
        fs::create_directory(dbFolder);  // 确保数据库文件夹存在
    }

    // 检查 .db 文件是否存在，不存在则创建
    if (!fs::exists(dbFilePath)) {
        std::ofstream dbFile(dbFilePath);
        if (!dbFile) {
            std::cerr << "无法创建数据库文件: " << dbFilePath << std::endl;
            return;
        }
        dbFile.close();
    }

    dbs[db_name] = new database(dbFilePath);  // 传递完整路径
    std::cout << "数据库 " << db_name << " 创建成功，存储于: " << dbFolder << std::endl;

    saveMetaData(); 
}


// 删除数据库
void dbManager::dropDatabase(const std::string& db_name) {
    auto it = dbs.find(db_name);
    if (it == dbs.end()) {
        std::cerr << "数据库 " << db_name << " 不存在！" << std::endl;
        return;
    }
	if (db_name == "sysdb") {
		std::cerr << "系统数据库不能删除！" << std::endl;
		return;
	}

    delete it->second;  // 删除数据库实例
    dbs.erase(it);  // 从映射中移除

    std::string dbPath = basePath + db_name;
    if (fs::exists(dbPath)) {
        fs::remove_all(dbPath); // 删除数据库文件夹
    }

    std::cout << "数据库 " << db_name << " 已删除！" << std::endl;
    saveMetaData(); 
}

// 获取数据库
database* dbManager::getDatabase(const std::string& db_name) {
    auto it = dbs.find(db_name);
    if (it != dbs.end()) {
        return it->second;
    }
    std::cerr << "数据库 " << db_name << " 不存在！" << std::endl;
    return nullptr;
}



// 获取所有数据库名称
std::vector<std::string> dbManager::getDatabaseNames() {
    std::vector<std::string> dbList;
    for (const auto& pair : dbs) {
        dbList.push_back(pair.first);
    }
    return dbList;
}

// 从磁盘加载数据库元数据
void dbManager::loadMetaData() {
    if (!fs::exists(metaDataFile)) return;

    std::ifstream file(metaDataFile);
    if (!file) {
        std::cerr << "法加载数据库元数据文件！" << std::endl;
        return;
    }

    std::string dbName;
    while (std::getline(file, dbName)) {
        std::string dbFilePath = basePath + dbName + "/" + dbName + ".db";  // **修正路径**
        dbs[dbName] = new database(dbFilePath);
    }
    file.close();
    std::cout << "数据库元数据加载完成！" << std::endl;
}

// 将数据库列表存入磁盘
void dbManager::saveMetaData() {
    std::ofstream file(metaDataFile);
    if (!file) {
        std::cerr << "无法保存数据库元数据！" << std::endl;
        return;
    }

    for (const auto& pair : dbs) {
        file << pair.first << std::endl;
    }
    file.close();
    std::cout << "数据库元数据已更新！" << std::endl;
}
