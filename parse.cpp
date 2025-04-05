#include "parse.h"
#include "dbManager.h"
#include "Record.h"

Parse::Parse() {
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
   
}

void Parse::handleDropDatabase(const std::smatch& m) {
    dbManager().dropDatabase(m[1]);
}

void Parse::handleInsertInto(const std::smatch& m) {
    Record r;
    r.insert_record(m[1], m[2], m[3]);
    r.insert_into("当前数据库路径或名称");
   
}

void Parse::handleSelect(const std::smatch& m) {
    auto records = Record::select("*", m[1]);
    
}
