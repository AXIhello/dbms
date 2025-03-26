#pragma once
#include <string>
#include <unordered_map>
#include "database.h"
#include <filesystem>
#include <iostream>
#include <fstream>
namespace fs = std::filesystem;

class dbManager
{
private:
	std::unordered_map<std::string, database*> dbs; //数据库实例
	std::string basePath = "./data/"; //数据库文件存储路径
	std::string metaDataFile = "./data/databases.meta"; //数据库元数据文件
	std::string systemDBName = "sysdb"; // 系统数据库名称

//持久化（磁盘）
	void loadMetaData();
	void saveMetaData();

public:
	dbManager();
	~dbManager();

	void createDatabase(const std::string& db_name); 
	void dropDatabase(const std::string& db_name);

	database* getDatabase(const std::string& db_name); //获取数据库实例
	std::vector<std::string> getDatabaseNames();
};

