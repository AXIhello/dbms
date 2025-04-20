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
	m_tdf = dbManager::basePath + "/data/" + m_db_name + "/" + m_tableName + ".tdf";
    m_tic = dbManager::basePath + "/data/" + m_db_name + "/" + m_tableName + ".tic";
    m_tid = dbManager::basePath + "/data/" + m_db_name + "/" + m_tableName + ".tid";
    m_trd = dbManager::basePath + "/data/" + m_db_name + "/" + m_tableName + ".trd";

	if (isTableExist()) {
		load(); // 加载表的元数据
	}
	else {
	    initializeNew(); // 初始化四个文件
		save();  // 保存表的元数据
	}
}


// 析构函数（一般为空）
Table::~Table() {

    // 删除表的相关文件  

	//deleteTableMetadata(); // 删除表的元数据文件

    //vector<string> filesToDelete = {
    //    m_tic, // 表的完整性文件  
    //    m_trd, // 表的数据文件  
    //    m_tdf, // 表的定义文件  
    //    m_tid  // 表的索引文件  
    //};

    //for (const auto& file : filesToDelete) {
    //    if (std::remove(file.c_str()) == 0) {
    //        cout << "文件删除成功: " << file << endl;
    //    }
    //    else {
    //       throw std::runtime_error("文件删除失败: ");
    //    }
    //}
}

void Table::load()
{
    loadMetadataBinary();
    loadDefineBinary();
    //loadRecordBinary();
    loadIntegrityBinary(); // 未实现
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

void Table::loadMetadataBinary() {
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
            return;  // 找到后退出方法
        }
    }

    // 如果没有找到指定的表格
	cerr << "未找到表格: " << m_tableName << endl;
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


//表定义文件操作
// 加载表的定义（待验证）

void Table::loadDefineBinary() {
    std::ifstream in(m_tdf, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("无法打开定义文件: " + m_tdf);
    }

    m_fields.clear();

    while (in.peek() != EOF) {
        FieldBlock field;
        in.read(reinterpret_cast<char*>(&field), sizeof(FieldBlock));
        if (in.gcount() < sizeof(FieldBlock)) break;

        m_fields.push_back(field);
    }

    in.close();
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
        field.integrities = 0; //无用的参数。 全部设置 0

        // 将当前字段块写入文件
        out.write(reinterpret_cast<const char*>(&field), sizeof(FieldBlock));
    }

    out.close();
}

void Table::addConstraint(const ConstraintBlock& constraint) {
    // 将约束添加到表的约束列表中
    m_constraints.push_back(constraint);

    // 更新时间戳
   // m_lastModifyTime = std::time(nullptr);
    // 将所有约束保存到 .tic 文件中
    saveIntegrityBinary();

    // 保存定义文件和元数据文件
    /*saveDefineBinary();
    saveMetadataBinary();*/
}


//字段操作
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
    loadRecord(); //
}

//待实现。不管有没有值，都直接删
void Table::dropField(const std::string fieldName) {

}

void Table::updateField(const std::string fieldName, const FieldBlock& updatedField) {

}

void Table::renameField(const std::string oldName, const std::string newName) {
	// 查找字段
	auto it = std::find_if(m_fields.begin(), m_fields.end(), [&](const FieldBlock& field) {
		return field.name == oldName;
		});
	if (it != m_fields.end()) {
        strncpy_s(it->name, newName.c_str(), sizeof(it->name) - 1); // 更新字段名
		saveDefineBinary(); // 保存到定义文件
		m_lastModifyTime = std::time(nullptr); // 更新时间戳
	}
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



//表完整性文件操作
void Table::loadIntegrityBinary() {
    // 打开 .tic 文件进行二进制读取
    std::ifstream in(m_tic, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("无法打开完整性文件: " + m_tic);
    }

    // 检查文件是否为空，若为空则直接返回
    in.seekg(0, std::ios::end);
    if (in.tellg() == 0) {
        in.close();
        return;  // 如果文件为空，直接返回
    }

    // 清空已有的约束数据
    m_constraints.clear();

    // 读取每个 ConstraintBlock 到 m_constraints 中
    ConstraintBlock constraint;
    while (in.read(reinterpret_cast<char*>(&constraint), sizeof(ConstraintBlock))) {
        m_constraints.push_back(constraint);
    }

    // 关闭文件
    in.close();
}


void Table::saveIntegrityBinary() {
    std::ofstream out(m_tic, std::ios::binary);
    if (!out.is_open()) {
        throw std::runtime_error("无法保存完整性文件: " + m_tableName + " .tic ");
    }

    // 遍历字段和对应的约束
    for (int i = 0; i < m_constraints.size(); ++i) {
        // 直接保存有效的约束
        out.write(reinterpret_cast<const char*>(&m_constraints[i]), sizeof(ConstraintBlock));
    }

    out.close();
}



//表记录文件操作
bool Table::loadRecord() {
    ifstream trdFile(m_trd); // 打开记录文件

    if (!trdFile.is_open()) {
        throw std::runtime_error( "无法打开数据文件: " + m_trd );
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

//TODO:待改；对应
void Table::updateRecord(std::vector<FieldBlock>& fields) {
    // 检查表是否存在
    if (!isTableExist()) {
        throw std::runtime_error("表 '" + m_tableName + "' 不存在。");
    }

    // 打开.trd文件
    std::fstream file(m_trd, std::ios::in | std::ios::out | std::ios::binary);
    if (!file) {
        throw std::runtime_error("无法打开表文件 '" + m_tableName + ".trd' 进行更新。");
    }

    // 读取现有记录
    std::vector<std::vector<std::string>> all_records;
    while (file.peek() != EOF) {
        std::vector<std::string> record_values;
        for (size_t i = 0; i < m_fields.size(); ++i) {
            FieldBlock& field = m_fields[i];
            bool is_null = false;
            file.read(reinterpret_cast<char*>(&is_null), sizeof(bool));

            if (is_null) {
                record_values.push_back("NULL");
                continue;
            }

            std::string value;
            switch (field.type) {
            case 1: { // INTEGER
                int int_val;
                file.read(reinterpret_cast<char*>(&int_val), sizeof(int));
                value = std::to_string(int_val);
                break;
            }
            case 2: { // DOUBLE
                double double_val;
                file.read(reinterpret_cast<char*>(&double_val), sizeof(double));
                value = std::to_string(double_val);
                break;
            }
            case 3: // VARCHAR
            case 4: { // CHAR
                char* buffer = new char[field.param];
                file.read(buffer, field.param);
                value = std::string(buffer, field.param);
                delete[] buffer;
                break;
            }
            case 5: { // DATETIME
                std::time_t timestamp;
                file.read(reinterpret_cast<char*>(&timestamp), sizeof(std::time_t));
                value = std::to_string(timestamp); // 或者转化为日期字符串
                break;
            }
            default:
                throw std::runtime_error("未知字段类型");
            }
            record_values.push_back(value);
        }
        all_records.push_back(record_values);
    }

    // 现在更新记录，找到需要更新的字段
    for (auto& record : all_records) {
        // 遍历更新的字段
        for (size_t i = 0; i < fields.size(); ++i) {
            FieldBlock& updated_field = fields[i];

            // 查找 m_fields 中相应字段的索引
            auto it = std::find_if(m_fields.begin(), m_fields.end(), [&](const FieldBlock& f) {
                return f.name == updated_field.name;
                });

            // 如果找到了该字段并且它不是删除的字段
            if (it != m_fields.end()) {
                size_t field_index = std::distance(m_fields.begin(), it);
            }
            else {
                // 如果字段不存在，说明是新增字段
                // 需要检查约束信息，例如DEFAULT约束或AUTO_INCREMENT约束
                std::string new_value = "NULL"; // 默认为 NULL

                // 查找是否有 DEFAULT 或 AUTO_INCREMENT 约束
                for (const auto& constraint : m_constraints) {
                    if (constraint.field == updated_field.name) {
                        switch (constraint.type) {
                        case 6: { // DEFAULT 约束
                            new_value = constraint.param; // 使用 DEFAULT 值
                            break;
                        }
                        case 7: { // AUTO_INCREMENT 约束
                            static int auto_increment_value = 1; // 自增值，从1开始
                            new_value = std::to_string(auto_increment_value++);
                            break;
                        }
                        default:
                            break;
                        }
                    }
                }

                // 将默认值或自增值填入记录
                record.push_back(new_value);
                m_fields.push_back(updated_field); // 更新表的字段信息
            }
        }
        // 删除字段逻辑：如果字段不在更新的字段列表中，就移除它
        for (size_t i = 0; i < m_fields.size(); ++i) {
            const FieldBlock& field = m_fields[i];
            if (std::find_if(fields.begin(), fields.end(), [&](const FieldBlock& f) {
                return f.name == field.name;
                }) == fields.end()) {
                // 如果该字段不在更新列表中，认为是删除的字段，移除该字段的记录
                record.erase(record.begin() + i);
                m_fields.erase(m_fields.begin() + i);
                --i; // 确保在删除后继续检查当前索引
            }
        }
    }

    // 清空原文件并重新写入所有更新后的记录
    file.close();
    file.open(m_trd, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!file) {
        throw std::runtime_error("无法重新打开表文件 '" + m_tableName + ".trd' 进行写入。");
    }

    // 重新写入更新后的记录
    for (const auto& record : all_records) {
        for (size_t i = 0; i < m_fields.size(); ++i) {
            const FieldBlock& field = m_fields[i];
            const std::string& value = record[i];

            // 写入NULL标志
            bool is_null = (value == "NULL");
            file.write(reinterpret_cast<const char*>(&is_null), sizeof(bool));

            if (!is_null) {
                switch (field.type) {
                case 1: { // INTEGER
                    int int_val = std::stoi(value);
                    file.write(reinterpret_cast<const char*>(&int_val), sizeof(int));
                    break;
                }
                case 2: { // DOUBLE
                    double double_val = std::stod(value);
                    file.write(reinterpret_cast<const char*>(&double_val), sizeof(double));
                    break;
                }
                case 3: {// VARCHAR
                    char* buffer = new char[field.param];
                    std::memset(buffer, 0, field.param);

                    if (strncpy_s(buffer, field.param, value.c_str(), field.param - 1) != 0) {
                        throw std::runtime_error("字符串复制失败。");
                    }

                    file.write(buffer, field.param);
                    delete[] buffer;
                    break;
                }
				case 4: { // BOOL
					bool bool_val = (value == "true" || value == "1");
					file.write(reinterpret_cast<const char*>(&bool_val), sizeof(bool));
					break;
                }
                case 5: { // DATETIME
                    std::time_t now = std::time(nullptr); // 简化为当前时间
                    file.write(reinterpret_cast<const char*>(&now), sizeof(std::time_t));
                    break;
                }
                default:
                    throw std::runtime_error("未知字段类型");
                }
            }
        }
    }

    file.close();
    std::cout << "记录更新成功。" << std::endl;
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