#include "parse.h"
#include "dbManager.h"
#include "Record.h"
#include "output.h"
#include"user.h"

#include <QRegularExpression>
#include <QStringList>
#include <sstream>

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
    r.insert_into("当前数据库路径或名称");
   
}

void Parse::handleSelect(const std::smatch& m) {
    std::string table_name = m[1];
    //权限检查
    if (!user::hasPermission("select|" + table_name))
    {
        return;
    }

    try {
        std::string columns = m[2].str();
        if (columns.empty()) {
            columns = "*";  // 默认选择所有列
        }

        auto records = Record::select(columns, table_name);//获取数据
        
        //处理WHERE字句
        std::string where_condition = m[3].str();  //
        if (!where_condition.empty()) {
            // 解析 WHERE 子句
            std::vector<Record> filtered_records;
            for (const auto& record : records) {
                if (matchesCondition(record, where_condition)) {
                    filtered_records.push_back(record);
                }
            }
            records = filtered_records;  // 替换为筛选后的记录
        }

        // 将 records 传递给 Output 进行处理
        Output::printSelectResult(outputEdit, records);

    }
    catch (const std::exception& e) {
        std::cerr << "查询失败: " << e.what() << std::endl;
    }
}
	
	

void Parse::handleAlterTable(const std::smatch& m) {
    Record r;
    // 假设 m[1] 是表名，m[2] 是 ALTER 命令的具体内容
   // r.alterTable(m[1], m[2]);
    
}

bool Parse::matchesCondition(const Record& record, const std::string& condition) {

    return false;  // 默认不匹配
}
