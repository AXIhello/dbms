#ifndef TABLE_H
#define TABLE_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>
#include <sstream>
#include <unordered_map>
#include"base/BTree.h"
#include"base/block/tableBlock.h"
#include"base/block/fieldBlock.h"
#include"base/block/constraintBlock.h"
#include"base/block/indexBlock.h"

using namespace std;


class Table {
public:
    Table(const string& m_db_name,const string& tableName);

    ~Table();

    void load();
	void save();
    void initializeNew();

	//获取表名
    string getTableName() const {
        return m_tableName;
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
   
    vector<string> getFieldNames() const;
	//对表操作（添加、删除、更新字段）
	vector<FieldBlock> getFields() const {
		return m_fields;
	}

    FieldBlock* getFieldByName(const std::string& fieldName) const;
    //获取列名


    void addField(const FieldBlock& field);
    void dropField(const std::string fieldName);
	void updateField(const std::string fieldName, const FieldBlock& updatedField);
	void renameField(const std::string oldName, const std::string newName);
    std::vector<std::vector<std::string>> selectAll() const;


    //表完整性文件
    void addConstraint(const std::string& constraintName,
                       const std::string& constraintType,
                       const std::string& constraintBody);

	void addConstraint(const ConstraintBlock& constraint);//原增加完整性约束方法（建表时用）
    //新增约束时ForeignKey专用
    void addForeignKey(const std::string& constraintName,
        const std::string& foreignKeyField,
        const std::string& referenceTable,
        const std::string& referenceField);

	void dropConstraint(const std::string constraintName);
	void updateConstraint(const std::string constraintName, const ConstraintBlock& updatedConstraint);
    void saveIntegrityBinary();
    void loadIntegrityBinary();

    //表记录文件
    bool saveRecord()const;
    bool loadRecord();
    void addRecord(const string& recordData);
    void deleteRecord(int recordID);
    string getDefaultValue(const std::string& fieldName) const;
    void print_records(const std::vector<std::unordered_map<std::string, std::string>>& records);
    //void updateRecord(std::vector<FieldBlock>& fields);
    void updateRecord_add(FieldBlock& field);
    void updateRecord_delete(const std::string& fieldName);
   

    //表索引文件
    void saveIndex();
    void loadIndex();
    void createIndex(const IndexBlock& index);
    void addIndex(const IndexBlock& index);
	void dropIndex(const std::string indexName);
	void updateIndex(const std::string indexName, const IndexBlock& updatedIndex);



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
	vector<ConstraintBlock> m_constraints; // 存储表的完整性约束信息
	vector<IndexBlock> m_indexes; // 存储表的索引信息
    std::vector<std::vector<std::string>> m_records;//表格内容存储
    // 辅助方法：将时间戳转为字符串格式
    string timeToString(time_t time) const;
 public:
     void SetRecords(vector<vector<string>>& records);
};

#endif // TABLE_H
