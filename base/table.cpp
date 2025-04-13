#include "table.h"
#include <iostream>
#include <ctime>
#include <cstring>
#include <iomanip>
#include"manager/dbManager.h"

// 构造函数，初始化表的相关信息√
Table::Table(const std::string& dbName, const std::string& tableName)
    : m_db_name(dbName), m_tableName(tableName), m_fieldCount(0), m_recordCount(0) {
    m_createTime = time(0);
    m_lastModifyTime = m_createTime;
	m_tb = dbManager::basePath + "/data/" + m_db_name + "/" + m_db_name + ".tb";

	if (isTableExist()) {
		load(); // 加载表的元数据
	}
	else {
	    initializeNew(); // 初始化四个文件
		save();  // 保存表的元数据
	}
}


// 析构函数（待改）
Table::~Table() {

    // 删除表的相关文件  

	deleteTableMetadata(); // 删除表的元数据文件

    vector<string> filesToDelete = {
        m_tableName + ".tic", // 表的完整性文件  
        m_tableName + ".trd", // 表的数据文件  
        m_tableName + ".tdf", // 表的定义文件  
        m_tableName + ".tid"  // 表的索引文件  
    };

    for (const auto& file : filesToDelete) {
        if (std::remove(file.c_str()) == 0) {
            cout << "文件删除成功: " << file << endl;
        }
        else {
            cerr << "文件删除失败: " << file << endl;
        }
    }
}

void Table::load()
{
    loadMetadataBinary();
    loadDefineBinary();
    //loadRecordBinary();
    // loadIntegrality(); // 未实现
    // loadIndex();       // 未实现
}

void Table::save() {
	//saveMetadataBinary();
	//saveDefineBinary();
	//saveRecordBinary();
	// saveIntegrality(); // 未实现
	// saveIndex();       // 未实现
}



void Table::initializeNew() {
    std::string fullPath = dbManager::basePath + "/data/" + m_db_name + "/" + m_tableName;
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

void Table::saveDefineBinary() {
    std::ofstream out(m_tdf, std::ios::binary);
    if (!out.is_open()) {
        throw std::runtime_error("无法保存定义文件: " + m_tableName + " .tdf ");
    }

    for (int i = 0; i < m_fields.size(); ++i) {
        FieldBlock& field = m_fields[i];

        // 填充必要的字段信息
        field.order = i; // 设置字段的顺序
        field.mtime = std::time(nullptr); // 设置最后修改时间为当前时间
        field.integrities = 0; // TODO: 如果有完整性约束，需要在此处理

        // 将当前字段块写入文件
        out.write(reinterpret_cast<const char*>(&field), sizeof(FieldBlock));
    }

    out.close();
}



void Table::loadMetadataBinary() {
    ifstream tbFile(m_tb, ios::binary);
    if (!tbFile) {
        cerr << "Failed to open database file." << endl;
        return;
    }

    // 读取每个 TableBlock，查找指定名称的表
    TableBlock tableBlock;
    while (tbFile.read(reinterpret_cast<char*>(&tableBlock), sizeof(TableBlock))) {
        if (string(tableBlock.name) == m_tableName) {
            // 找到目标表格，将其元数据复制到该表对象的 TableBlock 中
            m_recordCount = tableBlock.record_num;
            m_fieldCount = tableBlock.field_num;
            m_tb = tableBlock.name;  // 假设表描述文件路径使用表名
            m_tdf = tableBlock.tdf;
            m_tic = tableBlock.tic;
            m_trd = tableBlock.trd;
            m_tid = tableBlock.tid;
            m_createTime = tableBlock.crtime;
            m_lastModifyTime = tableBlock.mtime;  // 复制数据
            return;  // 找到后退出方法
        }
    }

    // 如果没有找到指定的表格
    cerr << "表" << m_tableName << "未找到" << endl;
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

            strncpy_s(tableBlock.tdf, (m_tableName + ".tdf").c_str(), sizeof(tableBlock.tdf) - 1);
            strncpy_s(tableBlock.tic, (m_tableName + ".tic").c_str(), sizeof(tableBlock.tic) - 1);
            strncpy_s(tableBlock.trd, (m_tableName + ".trd").c_str(), sizeof(tableBlock.trd) - 1);
            strncpy_s(tableBlock.tid, (m_tableName + ".tid").c_str(), sizeof(tableBlock.tid) - 1);

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


//元数据操作

bool Table::deleteTableMetadata() const {
    // 打开文件进行读取
    string filename = m_db_name + ".tb";
    ifstream tbFileIn(filename);

    if (!tbFileIn.is_open()) {
        cerr << "无法打开元数据文件: " << filename << endl;
        return false;
    }

    // 将文件内容读取到字符串流
    stringstream buffer;
    buffer << tbFileIn.rdbuf();
    tbFileIn.close();

    // 将文件内容存储到字符串中进行分析
    string fileContent = buffer.str();

    // 查找表的元数据部分
    size_t startPos = fileContent.find("TableName: " + m_tableName);
    if (startPos == string::npos) {
        cerr << "未找到表的元数据: " << m_tableName << endl;
        return false;
    }

    size_t endPos = fileContent.find("TableName: ", startPos + 1); // 查找下一个表的开始
    if (endPos == string::npos) {
        endPos = fileContent.length(); // 如果没有下一个表，则到文件末尾
    }

    // 删除表的元数据部分
    fileContent.erase(startPos, endPos - startPos);

    // 将修改后的内容写回文件
    ofstream tbFileOut(filename, ios::trunc); // 使用截断模式重新写入文件
    if (!tbFileOut.is_open()) {
        cerr << "无法写入元数据文件: " << filename << endl;
        return false;
    }

    tbFileOut << fileContent;
    tbFileOut.close();

    cout << "表的元数据已成功删除: " << m_tableName << endl;
    return true;
}

//表定义文件操作
// 加载表的定义（待验证）

void Table::loadDefineBinary() {
    std::string filename = dbManager::basePath+"/data/" +m_db_name + "/" + m_tableName + ".tdf";
    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("无法打开定义文件: " + filename);
    }

    m_fields.clear();

    while (in.peek() != EOF) {
        FieldBlock field;
        in.read(reinterpret_cast<char*>(&field), sizeof(FieldBlock));
        if (in.gcount() < sizeof(FieldBlock)) break;

        FieldBlock col;
        //col.name = std::string(field.name);
        //col.size = field.param;

        //// 映射类型整数为字符串
        //switch (field.type) {
        //    case 0: col.type = "INT"; break;
        //    case 1: col.type = "FLOAT"; break;
        //    case 2: col.type = "CHAR"; break;
        //    case 3: col.type = "VARCHAR"; break;
        //    default: col.type = "UNKNOWN"; break;
        //}

        // 暂时忽略 integrities 和 mtime
        //col.defaultValue = "";  // 可以后期扩展读取默认值字段

        m_fields.push_back(col);
    }

    in.close();
}


void Table::addField(const FieldBlock& field) {
    FieldBlock newField = field;

    // 设置字段顺序为当前字段数
    newField.order = m_fields.size();

    // 设置最后修改时间
    newField.mtime = std::time(nullptr);

    // 添加到字段列表
    m_fields.push_back(newField);

    // 更新字段数
    m_fieldCount = m_fields.size();

    // 更新时间戳
    m_lastModifyTime = std::time(nullptr);

    // 保存到定义文件和元数据文件
    saveDefineBinary();
    saveMetadataBinary();
}


/*
// 添加列√
void Table::addCol(const FieldBlock& col) {
	m_columns.push_back(col); // 添加新列
    loadRecord();//更改表记录文件内容
	m_lastModifyTime = time(0); // 更新最后修改时间
}

// 删除列√
void Table::deleteCol(const string& colName) {
	// 删除指定列
	auto it = std::remove_if(m_columns.begin(), m_columns.end(), [&](const Column& col) {
		return col.name == colName;
		});
	m_columns.erase(it, m_columns.end());
    loadRecord();//更改表记录文件内容
	m_lastModifyTime = time(0); // 更新最后修改时间
}

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
//加载表记录文件(待验证)
bool Table::loadRecord() {
    ifstream trdFile(m_tableName + ".trd"); // 打开记录文件

    if (!trdFile.is_open()) {
        cerr << "无法打开数据文件: " << m_tableName + ".trd" << endl;
        return false;
    }

    // 读取文件的第一行（列名）
    string header;
    getline(trdFile, header);
    vector<string> fileColumns;
    stringstream ss(header);
    string colName;

    // 将文件中的列名提取到 fileColumns 向量中
    while (getline(ss, colName, ',')) {
        fileColumns.push_back(colName);
    }

    // 找出哪些列需要删除（文件中有，但 m_columns 中没有）
    vector<string> columnsToAdd;
    vector<int> columnsToDelete;

    for (size_t i = 0; i < fileColumns.size(); ++i) {
        bool found = false;
        for (size_t j = 0; j < m_fields.size(); ++j) {
            if (fileColumns[i] == m_fields[j].name) {
                found = true;
                break;
            }
        }
        if (!found) {
            columnsToDelete.push_back(i); // 记录需要删除的列的索引
        }
    }

    // 查找哪些列需要增加（m_columns 中有，但文件中没有）
    for (size_t i = 0; i < m_fields.size(); ++i) {
        bool found = false;
        for (size_t j = 0; j < fileColumns.size(); ++j) {
            if (m_fields[i].name == fileColumns[j]) {
                found = true;
                break;
            }
        }
        if (!found) {
            columnsToAdd.push_back(m_fields[i].name); // 记录需要添加的列名
        }
    }

    // 读取源数据（从第二行开始）
    vector<vector<string>> records;
    string line;
    while (getline(trdFile, line)) {
        vector<string> record;
        stringstream recordStream(line);
        string value;

        while (getline(recordStream, value, ',')) {
            record.push_back(value);
        }
        records.push_back(record);
    }

    // 处理文件中的每条记录，按需要删除和添加列
    for (size_t i = 0; i < records.size(); ++i) {
        // 删除多余的列
        for (size_t delIdx : columnsToDelete) {
            records[i].erase(records[i].begin() + delIdx);
        }

        // 增加缺失的列，根据默认值补充
        for (const string& colNameToAdd : columnsToAdd) {
            // 找到对应的列的默认值，并添加到当前记录
            for (const auto& col : m_fields) {
                if (col.name == colNameToAdd) {
                    //records[i].push_back(col.defaultValue);
                    break;
                }
            }
        }
    }
    trdFile.close(); // 关闭文件
    return true;
}

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
        std::cerr << "无法打开数据库文件: " << m_tb << std::endl;
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

