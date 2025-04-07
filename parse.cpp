#include "parse.h"
#include "dbManager.h"
#include "Record.h"
#include "output.h"
#include "user.h"
#include "database.h"

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

void Parse::registerPatterns() {
    patterns.push_back({
        std::regex(R"(^CREATE\s+DATABASE\s+(\w+);$)", std::regex::icase),
        [this](const std::smatch& m) { handleCreateDatabase(m); }
        });

    patterns.push_back({
        std::regex(R"(^DROP\s+DATABASE\s+(\w+);$)", std::regex::icase),
        [this](const std::smatch& m) { handleDropDatabase(m); }
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

    patterns.push_back({
        std::regex(R"(^SHOW\s+DATABASES\s*;$)", std::regex::icase),
        [this](const std::smatch& m) { handleShowDatabases(m); }
        });

    patterns.push_back({
        std::regex(R"(^SHOW\s+TABLES\s*;$)", std::regex::icase),
        [this](const std::smatch& m) { handleShowTables(m); }
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
    dbManager().createUserDatabase(m[1]);
    Output::printMessage(outputEdit, "数据库 '" + QString::fromStdString(m[1]) + "' 创建成功！");
}

void Parse::handleDropDatabase(const std::smatch& m) {
    dbManager().dropDatabase(m[1]);
}

void Parse::handleInsertInto(const std::smatch& m) {
    Record r;
    r.insert_record(m[1], m[2], m[3]);
   
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

Parse::Parse(Database* database)
    : db(database) {  // 初始化 db 指针
    
}

void Parse::handleAddColumn(const std::smatch& match) {
    std::string tableName = match[1];  // 获取表名
    std::string columnName = match[2];  // 获取列名
    std::string columnType = match[3];  // 获取列类型

    Table* table = db->getTable(tableName);  // 获取表对象
    if (table) {
        // 创建新列
        Table::Column newColumn = { columnName, columnType, 0, "" };  // 默认 size 为 0，defaultValue 为空字符串
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
        Table::Column oldCol = { oldColumnName, "", 0, "" };  // 旧列只需要名字，其他信息保持默认
        Table::Column newCol = { newColumnName, newColumnType, 0, "" };  // 新列的名字和类型从命令中获取

        table->updateCol(oldCol, newCol);  // 调用 Table 的 updateCol 方法
    }
    else {
        std::cerr << "表 " << tableName << " 不存在！" << std::endl;
    }
}




void Parse::handleShowDatabases(const std::smatch& m) {
    auto dbs = dbManager().getDatabaseList();

    if (dbs.empty()) {
        std::cout << "没有任何数据库存在。" << std::endl;
    }
    else {
        for (const auto& name : dbs) {
            std::cout << "数据库: " << name << std::endl;
        }
    }
}
void Parse:: handleShowTables(const std::smatch& m) {

}
void Parse::handleShowColumns(const std::smatch& m) {

}
