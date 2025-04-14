#ifndef TABLE_H
#define TABLE_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include"fieldBlock.h"
#include <ctime>
#include <sstream>
#include"tableBlock.h"
using namespace std;


class Table {
public:
    Table(const string& m_db_name,const string& tableName);

    ~Table();

    void load();
	void save();
    void initializeNew();

	//获取表名
	string getTableName() const {}

    //获取列名
	vector<string> getColNames() const {
		vector<string> colNames;
		for (const auto& col : m_fields) {
			colNames.push_back(col.name);
		}
		return colNames;
	}


    // 打印表的元数据
    void printTableMetadata() const;

    // 获取表的创建时间
    string getCreateTimeString() const;

    // 获取表的最后修改时间
    string getLastModifyTimeString() const;

    // .tb文件相关
    void saveMetadataBinary();
	void loadMetadataBinary();

    //删除表相关文件
	bool deleteTableMetadata();
    void deleteFilesDisk();

    //.tdf文件相关
    bool saveDefine()const;
    void saveDefineBinary();
    bool loadDefine();
    void loadDefineBinary();
   
	//对表操作（添加、删除、更新字段）
    void addField(const FieldBlock& field);
    void dropField(const std::string fieldName);
	void updateField(const std::string fieldName, const FieldBlock& updatedField);

    //表完整性文件
    bool saveIntegrality()const;
    bool loadIntegrality();

    //表记录文件
    bool saveRecord()const;
    bool loadRecord();
    void addRecord(const string& recordData);
    void deleteRecord(int recordID);
    void updateRecord(int recordID, const string& updatedData);

    //表索引文件
    bool saveIndex()const;
    bool loadIndex();


	// 判断表是否存在
	bool isTableExist() const;

    //工具用
	void setFieldCount(int n) { m_fieldCount = n; }

private:

    string m_db_name;       // 数据库名
    string m_tableName;     // 表名
    int m_recordCount;      // 表记录数
    int m_fieldCount;       // 表字段数
    string m_tb;            //表描述文件路径 
    string m_tdf;           // 表定义文件路径
    string m_tic;           // 表格完整性文件路径
    string m_trd;           // 表格记录文件路径
    string m_tid;           // 表格索引文件路径
    time_t m_createTime;    // 表的创建时间
    time_t m_lastModifyTime; // 表的最后修改时间

	vector<FieldBlock> m_fields; // 存储表的字段信息

    // 辅助方法：将时间戳转为字符串格式
    string timeToString(time_t time) const;

};

#endif // TABLE_H
