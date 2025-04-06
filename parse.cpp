#include "parse.h"
#include "dbManager.h"
#include "Record.h"
#include "output.h"
#include"user.h"

#include <QRegularExpression>
#include <QStringList>
#include <sstream>
#include <regex>

Parse::Parse(QTextEdit * outputEdit) {
    this->outputEdit = outputEdit;
    registerPatterns();
}

void Parse::registerPatterns() {
    patterns.push_back({
        std::regex(R"(^CREATE\s+DATABASE\s+(\w+);?$)", std::regex::icase),
        [this](const std::smatch& m) { handleCreateDatabase(m); }
        });

    patterns.push_back({
        std::regex(R"(^DROP\s+DATABASE\s+(\w+);?$)", std::regex::icase),
        [this](const std::smatch& m) { handleDropDatabase(m); }
        });

    patterns.push_back({
        std::regex(R"(^INSERT\s+INTO\s+(\w+)\s*\(([^)]+)\)\s*VALUES\s*\(([^)]+)\);?$)", std::regex::icase),
        [this](const std::smatch& m) { handleInsertInto(m); }
        });

    patterns.push_back({
        std::regex(R"(^SELECT\s+\*\s+FROM\s+(\w+)(?:\s+WHERE\s+(.+))?;?$)", std::regex::icase),
        [this](const std::smatch& m) { handleSelect(m); }
        });

    patterns.push_back({
    std::regex(R"(^ALTER\s+TABLE\s+(\w+)\s+ADD\s+(\w+)\s+(\w+);?$)", std::regex::icase),
    [this](const std::smatch& m) { handleAlterTable(m); }
        });

    patterns.push_back({
        std::regex(R"(^SHOW\s+DATABASES\s*;?$)", std::regex::icase),
        [this](const std::smatch& m) { handleShowDatabases(m); }
        });

    patterns.push_back({
        std::regex(R"(^SHOW\s+TABLES\s*;?$)", std::regex::icase),
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
    auto records = Record::select("*", m[1]);
    
}

void Parse::handleAlterTable(const std::smatch& m) {
    Record r;
    // 假设 m[1] 是表名，m[2] 是 ALTER 命令的具体内容
   // r.alterTable(m[1], m[2]);
    
}

bool Parse::matchesCondition(const Record& record, const std::string& condition) {

    return false;  // 默认不匹配
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

