//建表语句待完善；
//删表有bug. 目前database中加载所有table; 似乎删除了metaData后无法获取表文件数据，无法删除。只删除了.tdf?
#include "database.h"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <filesystem>
#include "manager/dbManager.h"


// 构造函数：加载数据库√
Database::Database(const std::string& db_name) {
    loadDatabase(db_name);
    loadTables();
}

// 析构函数：保存数据库√
Database::~Database() {
   
}

void Database::loadDatabase(const std::string& db_name)
{
    std::ifstream file(dbManager::basePath + "/ruanko.db", std::ios::binary);
    if (!file) {
        throw std::runtime_error("无法打开 ruanko.db 文件");
    }

    DatabaseBlock info;
    while (file.read(reinterpret_cast<char*>(&info), sizeof(DatabaseBlock))) {
        if (std::string(info.dbName) == db_name) {
			m_db_name = db_name;
            m_db_type = info.type;
            m_db_path = std::string(info.filepath);
            m_create_time = info.crtime;
            return;
        }
    }

    throw std::runtime_error("数据库 " + db_name + " 未在 ruanko.db 中找到");
}

//加载所有表
void Database::loadTables() {
    // 清空之前的表缓存（可选）
    for (auto& pair : m_tables) {
        delete pair.second;
    }
    m_tables.clear();

    std::string tbPath = m_db_path + "/" + m_db_name + ".tb";
    std::ifstream tbFile(tbPath, std::ios::binary);
    if (!tbFile.is_open()) {
        throw std::runtime_error("无法打开表元数据文件: " + tbPath );
    }

    TableBlock block;
    while (tbFile.read(reinterpret_cast<char*>(&block), sizeof(TableBlock))) {
        std::string tableName(block.name);
        Table* table = new Table(m_db_name, tableName);

        try {
            table->loadDefineBinary(); // 加载字段定义
            m_tables[tableName] = table;
        }
        catch (const std::exception& e) {
            delete table;
            throw std::runtime_error("加载表 " + tableName + " 失败: " + std::string(e.what()));
        }
    }

    tbFile.close();
}


// 创建新表√
void Database::createTable(const std::string& table_name, const std::vector<FieldBlock>& fields, const std::vector<ConstraintBlock>& constraints) {
    // 检查表是否已存在
    if (tableExistsOnDisk(table_name)) {
        throw std::runtime_error("表 '" + table_name + "' 已存在。");
    }

    // 创建新的 Table 对象
    Table* new_table = new Table(m_db_name, table_name);
    new_table->initializeNew();
    new_table->setFieldCount(fields.size()); // 设置字段数为传入字段数量

    // 添加字段到表中
    for (const auto& field : fields) {
        new_table->addField(field);  // 每个字段都添加到表中
    }

    // 如果约束列表不为空，处理约束
    if (!constraints.empty()) {
        for (const auto& constraint : constraints) {
            new_table->addConstraint(constraint);  // 每个约束都添加到表中
            new_table->saveIntegrityBinary();
        }
    }

    // 保存表的定义、元数据和完整性信息
    new_table->saveDefineBinary();
    new_table->saveMetadataBinary();
    

    // 将新表添加到表集合中
    m_tables[table_name] = new_table;

    // 输出成功信息
    std::cout << "表 " << table_name << " 创建成功" << std::endl;
}


// 删除表√
void Database::dropTable(const std::string& table_name) {
    auto it = m_tables.find(table_name);
    if (it == m_tables.end()) {
        throw std::runtime_error("表 " + table_name + " 未找到");
    }

    Table* table = it->second;

    try {
        table->deleteTableMetadata();
        table->deleteFilesDisk();// 显式调用删除文件和元数据的方法
    }
    catch (const std::exception& e) {
        throw std::runtime_error("删除表文件失败: " + std::string(e.what()));
    }

    delete table;          // 安全地销毁对象
    m_tables.erase(it);    // 移除表映射

    std::cout << "表 " << table_name << " 已成功删除" << std::endl;
}


    //以下为表未加载到数据库中的情况
	//if (!tableExistsOnDisk(table_name)) {
	//	throw std::runtime_error("表 '" + table_name + "' 不存在于磁盘中。");
	//	return;
	//}
 //   Table* new_table = new Table(m_db_name, table_name);
 //   new_table->deleteTableMetadata();
	//delete new_table; // 调用析构函数删除表对象
    //以下为直接从磁盘中删除的情况



// 获取并加载表
Table* Database::getTable(const std::string& tableName) {
    // 如果表已经加载，则直接返回
    if (m_tables.find(tableName) != m_tables.end()) {
        return m_tables[tableName];
    }

    // 否则，加载表并缓存
    Table* table = new Table(m_db_name, tableName);
    table->loadMetadataBinary();  // 加载表元数据
    table->loadDefineBinary();    // 加载表定义

    // 将加载的表缓存到 m_tables 中
    m_tables[tableName] = table;

    return table;
}


//返回调用的数据库的所有表名
std::vector<std::string> Database::getAllTableNames() const {
    std::vector<std::string> tableNames;

    for (const auto& pair : m_tables) {
        if (pair.second) {
            tableNames.push_back(pair.second->getTableName()); 
        }
    }

    return tableNames;
}

// 从文件加载数据库数据
void Database::loadTable(const std::string& table_name) {
    std::string file_path = m_db_path + ".db";

    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error( "加载数据库文件： "+file_path +" 失败");
        return;
    }

    size_t tableCount = 0;
    file.read(reinterpret_cast<char*>(&tableCount), sizeof(tableCount));

    for (size_t i = 0; i < tableCount; ++i) {
        size_t len = 0;
        file.read(reinterpret_cast<char*>(&len), sizeof(len));

        std::string tableName(len, '\0');
        file.read(&tableName[0], len);

        Table* table = new Table(m_db_name, tableName);
        table->loadDefineBinary(); // 假设你有这个方法
        m_tables[tableName] = table;
    }
}


// 保存表数据到文件
void Database::saveTable(const std::string& table_name) {
    // 拼接完整路径：<base>/data/<dbName>/<dbName>.db
    std::string file_path = m_db_path + ".db";

    std::ofstream file(file_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error( "保存数据库文件： " + file_path + " 失败");
    }

    // 保存数据库表结构：这里只是演示，写入所有表名
    size_t tableCount = m_tables.size();
    file.write(reinterpret_cast<const char*>(&tableCount), sizeof(tableCount));

    for (const auto& pair : m_tables) {
        const std::string& name = pair.first;
        size_t len = name.size();
        file.write(reinterpret_cast<const char*>(&len), sizeof(len));
        file.write(name.c_str(), len);
    }

    std::cout << "数据库结构已保存至 " << file_path << std::endl;
}


std::string Database::getDBPath() const
{
	return m_db_path;
}

bool Database::tableExistsOnDisk(const std::string& table_name) const {
	std::string file_path = m_db_path + "/" + table_name + ".tdf";
	return std::filesystem::exists(file_path);
}
/*
void Database::beginTransaction() {
    transaction_manager.beginTransaction();
}

void Database::commitTransaction() {
    transaction_manager.commitTransaction();
}

void Database::rollbackTransaction() {
    transaction_manager.rollbackTransaction();
}
*/
