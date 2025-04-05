#ifndef TABLE_H
#define TABLE_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>
#include <sstream>

using namespace std;


class Table {
public:

    struct Column {
        string name; // 列名
        string type; // 列类型
        int size;    // 列大小
        string defaultValue; // 列默认值
    };

    // 构造函数，初始化表的相关信息
    Table(const string& m_db_name,const string& tableName);

    ~Table();

    // 获取表的元数据
    void getTableMetadata();


    // 打印表的元数据
    void printTableMetadata() const;

    // 获取表的创建时间
    string getCreateTimeString() const;

    // 获取表的最后修改时间
    string getLastModifyTimeString() const;

    // 表相关文件的方法（例如保存表的元数据等）
    bool saveMetadata()const;
    bool loadMetadata();

    bool saveDefine()const;
    bool loadDefine();
    void addCol(const Column& col);
    void deleteCol(const string& colName);
    void updateCol(const Column& oldCol, const Column& newCol);

    bool saveIntegrality()const;
    bool loadIntegrality();

    bool saveRecord()const;
    bool loadRecord();
    void addRecord(const string& recordData);
    void deleteRecord(int recordID);
    void updateRecord(int recordID, const string& updatedData);

    bool saveIndex()const;
    bool loadIndex();


	// 判断表是否存在
	bool isTableExist() const;

private:
    string m_db_name;       // 数据库名
    string m_tableName;     // 表名
    time_t m_createTime;    // 表的创建时间
    time_t m_lastModifyTime; // 表的最后修改时间
    int m_fieldCount;       // 表字段数
    int m_recordCount;      // 表记录数
    // 辅助方法：将时间戳转为字符串格式
    string timeToString(time_t time) const;
    /*
    string tdfFilePath; // 表的元数据文件路径 (.tdf)
    string ticFilePath; // 表的索引文件路径 (.tic)
    string trdFilePath; // 表的数据文件路径 (.trd)
    string tidFilePath; // 表的ID文件路径 (.tid)
    */
	vector<Column> m_columns; // 列信息

};

#endif // TABLE_H
