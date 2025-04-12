#include "parse.h"
#include "dbManager.h"
#include "base/Record.h"
#include "ui/output.h"
#include "base/user.h"
#include "base/database.h"

#include <QRegularExpression>
#include <QStringList>
#include <iostream>
#include <sstream>
#include <regex>
#include <vector>
#include <algorithm>

Parse::Parse(QTextEdit * outputEdit) {
    this->outputEdit = outputEdit;
    registerPatterns();
}
Parse::Parse(Database* database)
    : db(database) {  // 初始化 db 指针

}

void Parse::registerPatterns() {
    patterns.push_back({
        std::regex(R"(^CREATE\s+DATABASE\s+(\w+);$)", std::regex::icase),
        [this](const std::smatch& m) { handleCreateDatabase(m); }
        });

    patterns.push_back({
    std::regex(R"(^USE\s+(?:DATABASE\s+)?(\w+);$)", std::regex::icase),
    [this](const std::smatch& m) { handleUseDatabase(m); }
        });

    patterns.push_back({
        std::regex(R"(^DROP\s+DATABASE\s+(\w+);$)", std::regex::icase),
        [this](const std::smatch& m) { handleDropDatabase(m); }
        });

    patterns.push_back({
        std::regex(R"(^SHOW\s+DATABASES\s*;$)", std::regex::icase),
        [this](const std::smatch& m) { handleShowDatabases(m); }
        });

    patterns.push_back({
    std::regex(R"(^SELECT\s+DATABASE\s*\(\s*\)\s*;?$)", std::regex::icase),
    [this](const std::smatch& m) { handleSelectDatabase(); }
        });

    //待修改：建表的同时规定表结构
    patterns.push_back({
    std::regex(R"(^CREATE\s+TABLE\s+(\w+)\s*\((.*)\)\s*;?$)", std::regex::icase),
    [this](const std::smatch& m) { handleCreateTable(m); }
        });

    patterns.push_back({
    std::regex(R"(DROP\s+TABLE\s+(\w+);)", std::regex::icase),
    [this](const std::smatch& m) { handleDropTable(m); }
        });


    patterns.push_back({
        std::regex(R"(^SHOW\s+TABLES\s*;$)", std::regex::icase),
        [this](const std::smatch& m) { handleShowTables(m); }
        });

    patterns.push_back({
        std::regex(R"(^INSERT\s+INTO\s+(\w+)\s*\(([^)]+)\)\s*VALUES\s*\(([^)]+)\);$)", std::regex::icase),
        [this](const std::smatch& m) { handleInsertInto(m); }
        });

    patterns.push_back({
        std::regex(R"(^SELECT\s+\*\s+FROM\s+(\w+)(?:\s+WHERE\s+(.+))?;$)", std::regex::icase),
        [this](const std::smatch& m) { handleSelect(m); }
        });

    patterns.push_back({
        std::regex(R"(ALTER\s+TABLE\s+(\w+)\s+ADD\s+COLUMN\s+(\w+)\s+(\w+)(\(\d+\))?;)", std::regex::icase),
        [this](const std::smatch& m) { handleAddColumn(m); }
        });

    patterns.push_back({
        std::regex(R"(ALTER\s+TABLE\s+(\w+)\s+DROP\s+COLUMN\s+(\w+);)", std::regex::icase),
        [this](const std::smatch& m) { handleDeleteColumn(m); }
        });

    patterns.push_back({
        std::regex(R"(ALTER\s+TABLE\s+(\w+)\s+UPDATE\s+COLUMN\s+(\w+)\s+SET\s+(\w+)(\(\d+\))?;)", std::regex::icase),
        [this](const std::smatch& m) { handleUpdateColumn(m); }
        });

}

void Parse::execute(const QString& sql_qt) {
    std::string sql = sql_qt.toStdString();
    for (const auto& p : patterns) {
        std::smatch match;
        if (std::regex_match(sql, match, p.pattern)) {
            p.action(match);
            return;
        }
    }
}
    // 如果没有匹配，输出错误
  

void Parse::handleCreateDatabase(const std::smatch& m) {
    dbManager::getInstance().createUserDatabase(m[1]);
    Output::printMessage(outputEdit, "数据库 '" + QString::fromStdString(m[1]) + "' 创建成功！");
}

void Parse::handleDropDatabase(const std::smatch& m) {
    dbManager::getInstance().dropDatabase(m[1]);
    Output::printMessage(outputEdit, "数据库 '" + QString::fromStdString(m[1]) + "' 删除成功！");
}

void Parse::handleUseDatabase(const std::smatch& m) {
    std::string dbName = m[1];

    // 调用 dbManager 的 useDatabase 方法来切换数据库
    dbManager::getInstance().useDatabase(dbName);

    // 判断是否切换成功
    if (dbManager::getInstance().getCurrentDatabase() == nullptr) {
        // 如果没有切换成功，输出错误信息
        Output::printError(outputEdit, "无法切换到数据库 '" + QString::fromStdString(dbName) + "'。");
    } else {
        // 如果切换成功，输出成功信息
        Output::printMessage(outputEdit, "已成功切换到数据库 '" + QString::fromStdString(dbName) + "'.");
    }
}

void Parse::handleShowDatabases(const std::smatch& m) {
    auto dbs = dbManager::getInstance().getDatabaseList();

    Output::printDatabaseList(outputEdit, dbs);
}

void Parse::handleSelectDatabase() {
    try {
        std::string dbName = dbManager::getInstance().getCurrentDatabase()->getDBName();
        Output::printMessage(outputEdit, "当前数据库为： " + QString::fromStdString(dbName));
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, e.what());
    }
}



void Parse::handleCreateTable(const std::smatch& match) {
    std::string tableName = match[1];
    std::string rawDefinition = match[2]; // 字段部分，如 id INT, name CHAR(20)

    // 获取当前数据库
    Database* db = dbManager::getInstance().getCurrentDatabase();
    if (!db) {
        // 如果没有选择数据库，给用户提示
        Output::printError(outputEdit, "请先使用 USE 语句选择数据库");
        return;
    }

    // 解析字段定义
    std::vector<Table::FieldBlock> columns;
    std::regex colPattern(R"((\w+)\s+(\w+)(?:\((\d+)\))?)");  // 支持 name CHAR(10)
    auto begin = std::sregex_iterator(rawDefinition.begin(), rawDefinition.end(), colPattern);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        std::smatch m = *it;
        Table::FieldBlock col;
        col.name = m[1];
        col.type = m[2];
        col.size = m[3].matched ? std::stoi(m[3]) : 4; // 默认大小 4
        col.defaultValue = ""; // 可后续扩展 default
        columns.push_back(col);
    }

    // 异常捕获部分
    try {
        // 传递 tableName 和 columns 参数到 createTable 方法
        db->createTable(tableName, columns);
    }
    catch (const std::exception& e) {
        // 将 std::string 转换为 QString
        Output::printError(outputEdit, "表创建失败: " + QString::fromStdString(e.what()));
        return;
    }

    // 成功提示
    QString message = "表 " + QString::fromStdString(tableName) + " 创建成功";
    Output::printMessage(outputEdit, message);
}



void Parse::handleDropTable(const std::smatch& match) {
    std::string tableName = match[1];

    // 获取当前数据库
    Database* db = dbManager::getInstance().getCurrentDatabase();
    if (!db) {
        Output::printError(outputEdit, "请先使用 USE 语句选择数据库");
        return;
    }

    // 检查表是否存在
    if (!db->getTable(tableName)) {
        QString error = "表 " + QString::fromStdString(tableName) + " 不存在";
        Output::printError(outputEdit, error);
        return;
    }

    // 删除表
    db->dropTable(tableName);

    // 输出删除成功信息
    QString message = "表 " + QString::fromStdString(tableName) + " 删除成功";
    Output::printMessage(outputEdit, message);
}

void Parse::handleShowTables(const std::smatch& m) {
    // 获取当前数据库
    Database* db = dbManager::getInstance().getCurrentDatabase();
    if (!db) {
        Output::printError(outputEdit, "请先使用 USE 语句选择数据库");
        return;
    }

    // 获取表名列表
    std::vector<std::string> tableNames = db->getAllTableNames();

    if (tableNames.empty()) {
        Output::printMessage(outputEdit, "当前数据库中没有表");
        return;
    }

    // 构造输出内容
    QString message = "当前数据库中的表：\n";
    for (const auto& name : tableNames) {
        message += QString::fromStdString(name) + "\n";
    }

    Output::printMessage(outputEdit, message);
}


void Parse::handleInsertInto(const std::smatch& m) {
    // 提取表名
    std::string tableName = m[1];

    // 提取字段部分并拆分成一个字段名列表
    std::string columnsStr = m[2];
    std::vector<std::string> columns = splitString(columnsStr, ',');

    // 提取值部分并拆分成一个值列表
    std::string valuesStr = m[3];
    std::vector<std::string> values = splitString(valuesStr, ',');

    // 确保字段和值的数量匹配
    if (columns.size() != values.size()) {
        std::cerr << "字段数量与值的数量不匹配!" << std::endl;
        return;
    }

    Record r;

    // 将 columns 和 values 转换为逗号分隔的字符串格式
    std::string columnsStrFormatted = joinStrings(columns, ',');
    std::string valuesStrFormatted = joinStrings(values, ',');

    try {
        // 调用 Record 类的 insert_record 方法，传递表名、字段和值
        r.insert_record(tableName, columnsStrFormatted, valuesStrFormatted);
    }
    catch (const std::exception& e) {
        std::cerr << "插入操作失败: " << e.what() << std::endl;
    }
}

// 用于拆分字符串的函数
std::vector<std::string> Parse::splitString(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::string token;
    std::istringstream tokenStream(str);

    while (std::getline(tokenStream, token, delimiter)) {
        token = trim(token);  // 调用 trim 去掉前后空白
        result.push_back(token);
    }
    return result;
}

// 用于连接字符串的函数
std::string Parse::joinStrings(const std::vector<std::string>& strings, char delimiter) {
    std::ostringstream oss;
    for (size_t i = 0; i < strings.size(); ++i) {
        oss << strings[i];
        if (i != strings.size() - 1) {
            oss << delimiter; // 在每个元素后面添加分隔符
        }
    }
    return oss.str();
}

// 去掉前后空白的函数
std::string Parse::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    size_t last = str.find_last_not_of(" \t");
    return (first == std::string::npos || last == std::string::npos) ? "" : str.substr(first, last - first + 1);
}



void Parse::handleSelect(const std::smatch& m) {
    std::string table_name = m[1];

    // 权限检查
    if (!user::hasPermission("select|" + table_name)) {
        Output::printError(outputEdit, "无权限访问表 '" + QString::fromStdString(table_name) + "'。");
        return;
    }

    try {
        std::string columns = "*"; // 目前仅支持 SELECT *
        std::string condition;

        // 如果有 WHERE 子句则获取
        if (m.size() > 2 && m[2].matched) {
            condition = m[2].str();
        }

        // 直接调用封装了 where 逻辑的 Record::select
        auto records = Record::select(columns, table_name, condition);

        // 显示查询结果
        Output::printSelectResult(outputEdit, records);
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, "查询失败: " + QString::fromStdString(e.what()));
    }

}



void Parse::handleAddColumn(const std::smatch& match) {
    std::string tableName = match[1];  // 获取表名
    std::string columnName = match[2];  // 获取列名
    std::string columnType = match[3];  // 获取列类型

    Table* table = db->getTable(tableName);  // 获取表对象
    if (table) {
        // 创建新列
        Table::FieldBlock newColumn = { columnName, columnType, 0, "" };  // 默认 size 为 0，defaultValue 为空字符串
        table->addCol(newColumn);  // 调用 Table 的 addCol 方法
    }
    else {
        std::cerr << "表 " << tableName << " 不存在！" << std::endl;
    }
}

void Parse::handleDeleteColumn(const std::smatch& match) {
    std::string tableName = match[1];  // 获取表名
    std::string columnName = match[2];  // 获取列名

    Table* table = db->getTable(tableName);  // 获取表对象
    if (table) {
        table->deleteCol(columnName);  // 调用 Table 的 deleteCol 方法
    }
    else {
        std::cerr << "表 " << tableName << " 不存在！" << std::endl;
    }
}

void Parse::handleUpdateColumn(const std::smatch& match) {
    std::string tableName = match[1];  // 获取表名
    std::string oldColumnName = match[2];  // 获取旧列名
    std::string newColumnName = match[3];  // 获取新列名
    std::string newColumnType = match[4];  // 获取新列类型

    Table* table = db->getTable(tableName);  // 获取表对象
    if (table) {
        // 创建旧列和新列
        Table::FieldBlock oldCol = { oldColumnName, "", 0, "" };  // 旧列只需要名字，其他信息保持默认
        Table::FieldBlock newCol = { newColumnName, newColumnType, 0, "" };  // 新列的名字和类型从命令中获取

        table->updateCol(oldCol, newCol);  // 调用 Table 的 updateCol 方法
    }
    else {
        std::cerr << "表 " << tableName << " 不存在！" << std::endl;
    }
}


