#include "table.h"
#include <iostream>
#include <ctime>
#include "parse/parse.h"
#include <cstring>
#include <iomanip>
#include <windows.h>

using namespace std;


// 构造函数，初始化表的相关信息√
Table::Table(const std::string& dbName, const std::string& tableName)
    : m_db_name(dbName), m_tableName(tableName), m_fieldCount(0), m_recordCount(0) {
    m_createTime = time(0);
    m_lastModifyTime = m_createTime;
	m_tb = dbManager::basePath + "/data/" + m_db_name + "/" + m_db_name + ".tb";
	m_tdf = dbManager::basePath + "/data/" + m_db_name + "/" + m_tableName + ".tdf";
    m_tic = dbManager::basePath + "/data/" + m_db_name + "/" + m_tableName + ".tic";
    m_tid = dbManager::basePath + "/data/" + m_db_name + "/" + m_tableName + ".tid";
    m_trd = dbManager::basePath + "/data/" + m_db_name + "/" + m_tableName + ".trd";

	if (isTableExist()) {
		load(); // 加载表的元数据
	}
	else {
	    initializeNew(); // 初始化四个文件
		//save();  // 保存表的元数据
	}
}


void Table::SetRecords(vector<vector<string>>& records)
{
    m_records = records;
}

std::string Gbk(const std::string& utf8)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
    if (len == 0) return "";

    std::wstring wstr(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], len);

    len = WideCharToMultiByte(936, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    if (len == 0) return "";

    std::string gbk(len, 0);
    WideCharToMultiByte(936, 0, wstr.c_str(), -1, &gbk[0], len, NULL, NULL);

    if (!gbk.empty() && gbk.back() == '\0') gbk.pop_back();
    return gbk;
}

std::vector<std::vector<std::string>> Table::selectAll() const {
    std::vector<std::vector<std::string>> records;
    std::ifstream trdFile(m_trd);
    if (!trdFile.is_open()) {
        throw std::runtime_error("无法打开数据文件: " + m_trd);
    }

    std::string line;
    while (std::getline(trdFile, line)) {
        std::vector<std::string> record;
        std::stringstream ss(line);
        std::string value;
        while (std::getline(ss, value, ',')) {
            record.push_back(value);
        }
        records.push_back(record);
    }
    trdFile.close();
    return records;
}
// 析构函数（一般为空）
Table::~Table() {

   
}

void Table::load()
{
    loadMetadataBinary();
    loadDefineBinary();
    //loadRecordBinary();
    loadIntegrityBinary(); // 未实现
    loadIndex();       // 未实现
}

void Table::save() {
	saveMetadataBinary();
	saveDefineBinary();
	//saveRecordBinary();// 未实现
	// saveIntegrality(); // 未实现
	 saveIndex();       
}

void Table::initializeNew() {
    string fullPath = dbManager::basePath + "/data/" + m_db_name + "/" + m_tableName;
    std::vector<std::string> filesToCreate = {
        fullPath + ".tic",
        fullPath + ".trd",
        fullPath + ".tdf",
        fullPath + ".tid"
    };

    for (const auto& file : filesToCreate) {
        std::ofstream outFile(file);
        if (!outFile.is_open()) {
            throw std::runtime_error("无法创建文件: " + file);
        }
        outFile.close();
    }
}


// 打印表的元数据√
void Table::printTableMetadata() const {
    cout << "表名: " << m_tableName << endl;
    cout << "字段数: " << m_fieldCount << endl;
    cout << "记录数: " << m_recordCount << endl;
    cout << "创建时间: " << getCreateTimeString() << endl;
    cout << "最后修改时间: " << getLastModifyTimeString() << endl;
}

// 获取表的创建时间字符串√
string Table::getCreateTimeString() const {
    return timeToString(m_createTime);
}

// 获取表的最后修改时间字符串√
string Table::getLastModifyTimeString() const {
    return timeToString(m_lastModifyTime);
}

// 将时间戳转为字符串格式√
string Table::timeToString(time_t time) const {
    struct tm timeInfo;
    localtime_s(&timeInfo, &time);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeInfo);
    return string(buffer);
}

void Table::loadMetadataBinary()
{
    ifstream tbFile(m_tb, ios::binary);
   
    if (!tbFile) {
		throw std::runtime_error("无法打开元数据文件: " + m_tb);
    }

    // 读取每个 TableBlock，查找指定名称的表
    TableBlock tableBlock;
    while (tbFile.read(reinterpret_cast<char*>(&tableBlock), sizeof(TableBlock))) {
        if (string(tableBlock.name) == m_tableName) {
            // 找到目标表格，将其元数据复制到该表对象的 TableBlock 中
            m_recordCount = tableBlock.record_num;
            m_fieldCount = tableBlock.field_num;
            m_tdf = tableBlock.tdf;
            m_tic = tableBlock.tic;
            m_trd = tableBlock.trd;
            m_tid = tableBlock.tid;
            m_createTime = tableBlock.crtime;
            m_lastModifyTime = tableBlock.mtime;  // 复制数据
            m_abledUsers = tableBlock.abledUsers;

           
            tbFile.close();
            m_records.clear();
            std::ifstream trdFile(m_trd);
            if (trdFile.is_open()) {
                std::string line;
                while (std::getline(trdFile, line)) {
                    std::vector<std::string> record;
                    std::stringstream ss(line);
                    std::string value;
                    while (std::getline(ss, value, ',')) {
                        record.push_back(value);
                    }
                    m_records.push_back(record);
                }
                trdFile.close();
            }
            else {
                // 使用Output类处理错误信息
                if (Output::mode == 0) {
                    Output::printError_Cli("无法打开数据文件: " + m_trd);
                }
                else {
                    Output::printError(nullptr, QString::fromStdString("无法打开数据文件: " + m_trd));
                }
            }

          
            return;  // 找到后退出方法
        }
    }

    // 如果没有找到指定的表格
    if (Output::mode == 0) {
        Output::printError_Cli("未找到表格: " + m_tableName);
    }
    else {
        Output::printError(nullptr, QString::fromStdString("未找到表格: " + m_tableName));
    }
    tbFile.close();
}

//保存至.tb文件
void Table::saveMetadataBinary() {
    std::fstream tbFile(m_tb, std::ios::in | std::ios::out | std::ios::binary);

    if (!tbFile.is_open()) {
        // 如果文件不存在，则创建
        tbFile.open(m_tb, std::ios::out | std::ios::binary);
        tbFile.close();
        tbFile.open(m_tb, std::ios::in | std::ios::out | std::ios::binary);
    }

    if (!tbFile.is_open()) {
        throw std::runtime_error("无法打开或创建元数据文件: " + m_tb);
    }

    TableBlock tableBlock;
    bool tableFound = false;

    // 尝试更新已有记录
    while (tbFile.read(reinterpret_cast<char*>(&tableBlock), sizeof(TableBlock))) {
        if (std::string(tableBlock.name) == m_tableName) {
            tableFound = true;

            // 更新已有记录
            tbFile.seekp(-static_cast<int>(sizeof(TableBlock)), std::ios::cur);

            // 构造当前表信息
            memset(&tableBlock, 0, sizeof(TableBlock));
            strncpy_s(tableBlock.name, m_tableName.c_str(), sizeof(tableBlock.name) - 1);
            tableBlock.record_num = m_recordCount;
            tableBlock.field_num = m_fieldCount;
            tableBlock.crtime = m_createTime;
            tableBlock.mtime = m_lastModifyTime;

            strncpy_s(tableBlock.tdf, (m_tdf).c_str(), sizeof(tableBlock.tdf) - 1);
            strncpy_s(tableBlock.tic, (m_tic).c_str(), sizeof(tableBlock.tic) - 1);
            strncpy_s(tableBlock.trd, (m_trd).c_str(), sizeof(tableBlock.trd) - 1);
            strncpy_s(tableBlock.tid, (m_tid).c_str(), sizeof(tableBlock.tid) - 1);
            strcpy_s(tableBlock.abledUsers, sizeof(tableBlock.abledUsers), m_abledUsers.c_str());  // 写入 abledUsers

            tbFile.write(reinterpret_cast<char*>(&tableBlock), sizeof(TableBlock));
            break;
        }
    }

    // 如果未找到旧记录则追加新表记录
    if (!tableFound) {
        memset(&tableBlock, 0, sizeof(TableBlock));
        strncpy_s(tableBlock.name, m_tableName.c_str(), sizeof(tableBlock.name) - 1);
        tableBlock.record_num = m_recordCount;
        tableBlock.field_num = m_fieldCount;
        tableBlock.crtime = m_createTime;
        tableBlock.mtime = m_lastModifyTime;

        strncpy_s(tableBlock.tdf, (m_tableName + ".tdf").c_str(), sizeof(tableBlock.tdf) - 1);
        strncpy_s(tableBlock.tic, (m_tableName + ".tic").c_str(), sizeof(tableBlock.tic) - 1);
        strncpy_s(tableBlock.trd, (m_tableName + ".trd").c_str(), sizeof(tableBlock.trd) - 1);
        strncpy_s(tableBlock.tid, (m_tableName + ".tid").c_str(), sizeof(tableBlock.tid) - 1);

        tbFile.clear(); // 清除 EOF 状态以便写入
        tbFile.seekp(0, std::ios::end);
        tbFile.write(reinterpret_cast<char*>(&tableBlock), sizeof(TableBlock));
    }

    tbFile.close();
}

//从.tb文件中删除表的元数据
bool Table::deleteTableMetadata(){
    // 1. 打开元数据文件
    std::ifstream tbFileIn(m_tb, std::ios::binary);
    if (!tbFileIn.is_open()) {
        throw std::runtime_error("无法打开元数据文件: " + m_tb);
    }

    // 2. 创建临时文件
    std::string tempFile = m_tb + ".tmp";
    std::ofstream tbFileOut(tempFile, std::ios::binary);
    if (!tbFileOut.is_open()) {
        tbFileIn.close();
        throw std::runtime_error("无法创建临时文件: " + tempFile);
    }

    TableBlock block;
    bool found = false;

    // 3. 遍历并筛选写入
    while (tbFileIn.read(reinterpret_cast<char*>(&block), sizeof(TableBlock))) {
        if (std::string(block.name) == m_tableName) {
            found = true; // 找到要删除的表，跳过写入
            continue;
        }
        tbFileOut.write(reinterpret_cast<const char*>(&block), sizeof(TableBlock));
    }

    tbFileIn.close();
    tbFileOut.close();

    // 4. 检查是否找到并替换原文件
    if (!found) {
        std::remove(tempFile.c_str()); // 清理临时文件
        throw std::runtime_error("未找到表元数据: " + m_tableName);
    }

    if (std::remove(m_tb.c_str()) != 0) {
        throw std::runtime_error("无法删除旧元数据文件: " + m_tb);
    }

    if (std::rename(tempFile.c_str(), m_tb.c_str()) != 0) {
        throw std::runtime_error("无法重命名临时文件: " + tempFile);
    }

    return true;
}

vector<std::string> Table::getFieldNames() const {  
   vector<std::string> fieldNames;  
   for (const auto& field : m_fields) {  
       fieldNames.push_back(field.name);  
   }  
   return fieldNames;  
}
/*
// 更新列√
void Table::updateCol(const Column& oldCol, const Column& newCol) {
	// 更新指定列
	auto it = std::find_if(m_columns.begin(), m_columns.end(), [&](const Column& col) {
		return col.name == oldCol.name;
		});
	if (it != m_columns.end()) {
		*it = newCol; // 更新列
	}
    loadRecord();//更改表记录文件内容
	m_lastModifyTime = time(0); // 更新最后修改时间
}

*/

//表记录文件操作
//bool Table::loadRecord() {
//    ifstream trdFile(m_trd); // 打开记录文件
//
//    if (!trdFile.is_open()) {
//        throw std::runtime_error( "无法打开数据文件: " + m_trd );
//    }
//
//    // 读取文件的第一行（列名）
//    string header;
//    getline(trdFile, header);
//    vector<string> fileColumns;
//    stringstream ss(header);
//    string colName;
//
//    // 将文件中的列名提取到 fileColumns 向量中
//    while (getline(ss, colName, ',')) {
//        fileColumns.push_back(colName);
//    }
//
//    // 找出哪些列需要删除（文件中有，但 m_columns 中没有）
//    vector<string> columnsToAdd;
//    vector<int> columnsToDelete;
//
//    for (size_t i = 0; i < fileColumns.size(); ++i) {
//        bool found = false;
//        for (size_t j = 0; j < m_fields.size(); ++j) {
//            if (fileColumns[i] == m_fields[j].name) {
//                found = true;
//                break;
//            }
//        }
//        if (!found) {
//            columnsToDelete.push_back(i); // 记录需要删除的列的索引
//        }
//    }
//
//    // 查找哪些列需要增加（m_columns 中有，但文件中没有）
//    for (size_t i = 0; i < m_fields.size(); ++i) {
//        bool found = false;
//        for (size_t j = 0; j < fileColumns.size(); ++j) {
//            if (m_fields[i].name == fileColumns[j]) {
//                found = true;
//                break;
//            }
//        }
//        if (!found) {
//            columnsToAdd.push_back(m_fields[i].name); // 记录需要添加的列名
//        }
//    }
//
//    // 读取源数据（从第二行开始）
//    vector<vector<string>> records;
//    string line;
//    while (getline(trdFile, line)) {
//        vector<string> record;
//        stringstream recordStream(line);
//        string value;
//
//        while (getline(recordStream, value, ',')) {
//            record.push_back(value);
//        }
//        records.push_back(record);
//    }
//
//    // 处理文件中的每条记录，按需要删除和添加列
//    for (size_t i = 0; i < records.size(); ++i) {
//        // 删除多余的列
//        for (size_t delIdx : columnsToDelete) {
//            records[i].erase(records[i].begin() + delIdx);
//        }
//
//        // 增加缺失的列，根据默认值补充
//        for (const string& colNameToAdd : columnsToAdd) {
//            // 找到对应的列的默认值，并添加到当前记录
//            for (const auto& col : m_fields) {
//                if (col.name == colNameToAdd) {
//                    //records[i].push_back(col.defaultValue);
//                    break;
//                }
//            }
//        }
//    }
//    trdFile.close(); // 关闭文件
//    return true;
//}

//lzl疑似已写
/*
//保存表记录文件（不知道用来干嘛）
bool Table::saveRecord() const{


}

// 向表中添加记录（待改:数据未经过验证直接插入文档最后）
void Table::addRecord(const string& recordData) {
    ofstream trdFile(m_tableName + ".trd", ios::app); // 以追加模式打开记录文件

    if (!trdFile.is_open()) {
        cerr << "无法打开数据文件: " << m_tableName + ".trd" << endl;
        return;
    }

    // 写入新的记录
    trdFile << recordData << endl;

    trdFile.close();

    m_recordCount++; // 更新记录数
    m_lastModifyTime = time(0); // 更新最后修改时间
}

// 删除表中的记录（待改）
void Table::deleteRecord(int recordID) {
    // 假设记录文件按行存储，删除某条记录
    ifstream trdFile(m_tableName + ".trd");
    if (!trdFile.is_open()) {
        cerr << "无法打开数据文件: " << m_tableName + ".trd" << endl;
        return;
    }

    vector<string> records;
    string line;
    int currentID = 0;

    // 读取所有记录
    while (getline(trdFile, line)) {
        if (currentID != recordID) {
            records.push_back(line);
        }
        currentID++;
    }
    trdFile.close();

    // 重新写回所有记录，排除已删除的记录
    ofstream trdFileOut(m_tableName + ".trd");
    if (!trdFileOut.is_open()) {
        cerr << "无法打开数据文件进行写入: " << m_tableName + ".trd" << endl;
        return;
    }

    for (const string& record : records) {
        trdFileOut << record << endl;
    }
    trdFileOut.close();

    m_recordCount--; // 更新记录数
    m_lastModifyTime = time(0); // 更新最后修改时间
}

// 修改表中的记录（待改）
void Table::updateRecord(int recordID, const string& updatedData) {
    // 假设记录文件按行存储，更新指定的记录
    ifstream trdFile(m_tableName + ".trd");
    if (!trdFile.is_open()) {
        cerr << "无法打开数据文件: " << m_tableName + ".trd" << endl;
        return;
    }

    vector<string> records;
    string line;
    int currentID = 0;

    // 读取所有记录
    while (getline(trdFile, line)) {
        if (currentID == recordID) {
            records.push_back(updatedData); // 更新记录
        }
        else {
            records.push_back(line);
        }
        currentID++;
    }
    trdFile.close();

    // 重新写回所有记录
    ofstream trdFileOut(m_tableName + ".trd");
    if (!trdFileOut.is_open()) {
        cerr << "无法打开数据文件进行写入: " << m_tableName + ".trd" << endl;
        return;
    }

    for (const string& record : records) {
        trdFileOut << record << endl;
    }
    trdFileOut.close();

    m_lastModifyTime = time(0); // 更新最后修改时间
}
*/

//检验表是否存在√
bool Table::isTableExist() const {
    // 打开数据库描述文件进行二进制读取
    std::fstream tbFile(m_tb, std::ios::in | std::ios::binary);

    if (!tbFile.is_open()) {
        throw std::runtime_error( "无法打开数据库文件: "+ m_tb);
        return false;  // 无法打开文件，认为表不存在
    }

    TableBlock tableBlock;
    // 遍历文件中的每个 TableBlock，查找匹配的表名
    while (tbFile.read(reinterpret_cast<char*>(&tableBlock), sizeof(TableBlock))) {
        if (std::string(tableBlock.name) == m_tableName) {
            tbFile.close();
            return true;  // 找到匹配的表名，返回 true
        }
    }

    tbFile.close();
    return false;  // 没有找到表名，返回 false
}

//从磁盘中删除4个表定义文件
void Table::deleteFilesDisk()
{
	// 删除表的相关文件
	std::vector<std::string> filesToDelete = {
		 // 表的定义文件
		m_tdf,
        m_tic,// 表的完整性文件
		m_trd, // 表的数据文件
		m_tid  // 表的索引文件
	};
	for (const auto& file : filesToDelete) {
		if (std::remove(file.c_str()) == 0) {
			cout << "文件删除成功: " << file << endl;
		}
		else {
			throw std::runtime_error("文件删除失败: " + file);
		}
	}
}

size_t get_field_size(const FieldBlock& field) {
    switch (field.type) {
    case 1: // INT
        return sizeof(int);
    case 2: // DOUBLE
        return sizeof(double);
    case 3: // VARCHAR(n)
        return static_cast<size_t>(field.param);  // 最长 n 字节（用户定义）
    case 4: // BOOL
        return sizeof(char);
    case 5: // DATETIME
        return 16; // 定长字符串（如 "YYYY-MM-DD HH:MM"）
    default:
        throw std::runtime_error("未知字段类型: " + std::to_string(field.type));
    }
}


void Table::addAbledUser(const std::string& username) {
    if (!isUserAuthorized(username)) {
        if (!m_abledUsers.empty()) m_abledUsers += "|";
        m_abledUsers += username;
        saveMetadataBinary();  // 写回 .tb 文件
    }
}

bool Table::isUserAuthorized(const std::string& username) const {
    std::istringstream ss(m_abledUsers);
    std::string token;
    while (std::getline(ss, token, '|')) {
        if (token == username) return true;
    }
    return false;
}
