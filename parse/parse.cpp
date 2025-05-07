#include "parse.h"
#include <transaction/TransactionManager.h>
#include<regex>
#include<sstream>
#include<Windows.h>
//#include <main.cpp>
Parse::Parse() : outputEdit(nullptr), mainWindow(nullptr), db(nullptr) {
    registerPatterns();
}
Parse::Parse(QTextEdit* outputEdit, MainWindow* mainWindow) : outputEdit(outputEdit), mainWindow(mainWindow) {
    registerPatterns();
}
Parse::Parse(Database* database)
    : db(database) {  // 初始化 db 指针
}

/*std::string Parse::executeSQL(const std::string& sql)
{
    std::ostringstream output;

    try {
        std::string cleanedSQL = trim(sql);
        std::string upperSQL = toUpperPreserveQuoted(cleanedSQL);

        // 特判事务控制语句
        if (std::regex_search(upperSQL, std::regex("^START TRANSACTION;$"))) {
            TransactionManager::instance().begin();
            return "事务开始";
        }
        if (std::regex_search(upperSQL, std::regex("^COMMIT;$"))) {
            TransactionManager::instance().commit();
            return "事务已提交";
        }
        if (std::regex_search(upperSQL, std::regex("^ROLLBACK;$"))) {
            TransactionManager::instance().rollback();
            return "事务已回滚";
        }

        // 🩸设置 CLI 模式的输出流
        //Output::setOstream(&output);

         // 判断模式，设置输出流
        if (Output::mode == 0) {  // CLI模式
            Output::setOstream(&std::cout);
        }
        else if (Output::mode == 1) {  // GUI模式
            Output::setOstream(&output);
        }

        for (const auto& p : patterns) {
            std::smatch match;
            if (std::regex_match(upperSQL, match, p.pattern)) {
                p.action(match);  
                return output.str();  // 输出内容返回
            }
        }

        Output::printError("SQL 语法不支持: " + cleanedSQL);
        return output.str();

    }
    catch (const std::exception& e) {
        Output::printError(std::string("SQL 执行异常: ") + e.what());
        return output.str();
    }
    
}
*/


/*
std::string Parse::executeSQL(const std::string& sql)
{
    std::ostringstream output;

    try {
        std::string cleanedSQL = trim(sql);
        std::string upperSQL = toUpperPreserveQuoted(cleanedSQL);

        // 特判事务控制语句
        if (std::regex_search(upperSQL, std::regex("^START TRANSACTION;$"))) {
            TransactionManager::instance().begin();
            if (Output::mode == 0) {
                Output::printInfo("事务开始");
            }
            return "事务开始";
        }
        if (std::regex_search(upperSQL, std::regex("^COMMIT;$"))) {
            TransactionManager::instance().commit();
            if (Output::mode == 0) {
                Output::printInfo("事务已提交");
            }
            return "事务已提交";
        }
        if (std::regex_search(upperSQL, std::regex("^ROLLBACK;$"))) {
            TransactionManager::instance().rollback();
            if (Output::mode == 0) {
                Output::printInfo("事务已回滚");
            }
            return "事务已回滚";
        }

        // 根据不同模式设置输出流
        if (Output::mode == 0) {  // CLI模式
            // CLI模式下不需要在这里设置输出流，因为在RunCliMode中已经设置了
            // Output::setOstream已经在RunCliMode中设置为&std::cout
        }
        else if (Output::mode == 1) {  // GUI模式
            Output::setOstream(&output);
        }

        for (const auto& p : patterns) {
            std::smatch match;
            if (std::regex_match(upperSQL, match, p.pattern)) {
                p.action(match);
                return output.str();  // 返回GUI模式下的输出内容
            }
        }

        if (Output::mode == 0) {
            Output::printError("SQL 语法不支持: " + cleanedSQL);
        }
        else {
            Output::printError(nullptr, QString::fromStdString("SQL 语法不支持: " + cleanedSQL));
        }
        return output.str();
    }
    catch (const std::exception& e) {
        if (Output::mode == 0) {
            Output::printError(std::string("SQL 执行异常: ") + e.what());
        }
        else {
            Output::printError(nullptr, QString::fromStdString(std::string("SQL 执行异常: ") + e.what()));
        }
        return output.str();
    }
}
*/


std::string Parse::executeSQL(const std::string& sql)
{
    std::ostringstream output;
    try {
        std::string cleanedSQL = trim(sql);
        std::string upperSQL = toUpperPreserveQuoted(cleanedSQL);

        // 特判事务控制语句
        if (std::regex_search(upperSQL, std::regex("^START TRANSACTION;$"))) {
            TransactionManager::instance().begin();
            if (Output::mode == 0) {
                Output::printInfo_Cli("事务开始");
            }
            else {
                Output::printInfo(nullptr, QString::fromStdString("事务开始"));
            }
            return "事务开始";
        }

        if (std::regex_search(upperSQL, std::regex("^COMMIT;$"))) {
            TransactionManager::instance().commit();
            if (Output::mode == 0) {
                Output::printInfo_Cli("事务已提交");
            }
            else {
                Output::printInfo(nullptr, QString::fromStdString("事务已提交"));
            }
            return "事务已提交";
        }

        if (std::regex_search(upperSQL, std::regex("^ROLLBACK;$"))) {
            TransactionManager::instance().rollback();
            if (Output::mode == 0) {
                Output::printInfo_Cli("事务已回滚");
            }
            else {
                Output::printInfo(nullptr, QString::fromStdString("事务已回滚"));
            }
            return "事务已回滚";
        }

        // GUI模式下设置输出流到ostringstream
        if (Output::mode == 1) {  // GUI模式
            Output::setOstream(&output);
        }

        // 执行SQL匹配和处理
        for (const auto& p : patterns) {
            std::smatch match;
            if (std::regex_match(upperSQL, match, p.pattern)) {
                p.action(match);
                return output.str();  // 返回GUI模式下的输出内容
            }
        }

        // SQL语法不支持的提示信息
        if (Output::mode == 0) {
            Output::printError_Cli("SQL 语法不支持: " + cleanedSQL);
        }
        else {
            Output::printError(nullptr, QString::fromStdString("SQL 语法不支持: " + cleanedSQL));
        }
        return output.str();
    }
    catch (const std::exception& e) {
        std::string errorMsg = std::string("SQL 执行异常: ") + e.what();
        if (Output::mode == 0) {
            Output::printError_Cli(errorMsg);
        }
        else {
            Output::printError(nullptr, QString::fromStdString(errorMsg));
        }
        return output.str();
    }
}

void Parse::registerPatterns() {
  
    /*   DDL   */
    //√
    patterns.push_back({
        std::regex(R"(^CREATE\s+DATABASE\s+(\w+);$)", std::regex::icase),
        [this](const std::smatch& m) { handleCreateDatabase(m); }
        });

    //√
    patterns.push_back({
        std::regex(R"(^DROP\s+DATABASE\s+(\w+);$)", std::regex::icase),
        [this](const std::smatch& m) { handleDropDatabase(m); }
        });

    //√
    patterns.push_back({
    std::regex(R"(^CREATE\s+TABLE\s+(\w+)\s*\((.*)\)\s*;?$)", std::regex::icase),
    [this](const std::smatch& m) { handleCreateTable(m); }
        });

    //√
    patterns.push_back({
    std::regex(R"(DROP\s+TABLE\s+(\w+);)", std::regex::icase),
    [this](const std::smatch& m) { handleDropTable(m); }
        });

    
    //
    patterns.push_back({
     std::regex(R"(ALTER\s+TABLE\s+(\w+)\s+ADD\s+COLUMN\s+(\w+)\s+(\w+)(\(\d+\))?(?:\s+(.*?))?\s*;?\s*$)", std::regex::icase),
     [this](const std::smatch& m) { handleAddColumn(m); }
        });


    //会崩溃 xtree报错
    patterns.push_back({
        std::regex(R"(ALTER\s+TABLE\s+(\w+)\s+DROP\s+COLUMN\s+(\w+);)", std::regex::icase),
        [this](const std::smatch& m) { handleDropColumn(m); }
        });

    //解析有问题。modify修改类型，change修改字段名 
    patterns.push_back({
      std::regex(R"(ALTER\s+TABLE\s+(\w+)\s+MODIFY\s+(\w+)\s+(\w+)(\(\d+\))?(.*);?)", std::regex::icase),
      [this](const std::smatch& m) { handleModifyColumn(m); }
        });

    //仅支持primary key, unique和check的表级约束
    patterns.push_back({
    std::regex(R"(ALTER\s+TABLE\s+(\w+)\s+ADD\s+CONSTRAINT\s+(\w+)\s+(PRIMARY\s+KEY|UNIQUE|CHECK)\s*(.*);)", std::regex::icase),
    [this](const std::smatch& m) { handleAddConstraint(m); }
        });

    //对foreign key 特别设置的表级约束
	patterns.push_back({ 
    std::regex(R"(ALTER\s+TABLE\s+(\w+)\s+ADD\s+CONSTRAINT\s+(\w+)\s+FOREIGN\s+KEY\s*\((\w+)\)\s+REFERENCES\s+(\w+)\s*\((\w+)\);)", std::regex::icase),
	[this](const std::smatch& m) { handleAddForeignKey(m); }
		});
    
    //
    patterns.push_back({
    std::regex(R"(ALTER\s+TABLE\s+(\w+)\s+DROP\s+CONSTRAINT\s+(\w+);)", std::regex::icase),
    [this](const std::smatch& m) { handleDropConstraint(m); }
        });

    
    patterns.push_back({
    std::regex(R"(CREATE\s+INDEX\s+(\w+)\s+ON\s+(\w+)\s*\(\s*(\w+)(?:\s*,\s*(\w+))?\s*\);?)", std::regex::icase),
    [this](const std::smatch& m) { handleCreateIndex(m); }
        });

  
    patterns.push_back({
    std::regex(R"(DROP\s+INDEX\s+(\w+)\s+ON\s+(\w+);?)", std::regex::icase),
    [this](const std::smatch& m) { handleDropIndex(m); }
        });


    /*  DML  */
    //√
    patterns.push_back({
     std::regex(R"(^INSERT\s+INTO\s+(\w+)\s*(?:\(([^)]+)\))?\s*VALUES\s*((?:\([^)]*\)\s*,\s*)*\([^)]*\))\s*;$)", std::regex::icase),
     [this](const std::smatch& m) {
         handleInsertInto(m);
     }
        });



    //√
    patterns.push_back({
    std::regex(R"(^UPDATE\s+(\w+)\s+SET\s+(.+?)(?:\s+WHERE\s+(.+?))?\s*;$)", std::regex::icase),
    [this](const std::smatch& m) { handleUpdate(m); }
        });


    //√
    patterns.push_back({
        std::regex(R"(^DELETE\s+FROM\s+(\w+)\s*(?:WHERE\s+(.+))?\s*;$)", std::regex::icase),
        [this](const std::smatch& m) { handleDelete(m); }
        });


    /*  DQL  */
    //√
    patterns.push_back({
        std::regex(R"(^SHOW\s+DATABASES\s*;$)", std::regex::icase),
        [this](const std::smatch& m) { handleShowDatabases(m); }
        });

    //√
    patterns.push_back({
    std::regex(R"(^SELECT\s+DATABASE\s*\(\s*\)\s*;?$)", std::regex::icase),
    [this](const std::smatch& m) { handleSelectDatabase(); }
        });

    //√
    patterns.push_back({
        std::regex(R"(^SHOW\s+TABLES\s*;$)", std::regex::icase),
        [this](const std::smatch& m) { handleShowTables(m); }
        });

    //√ 
    patterns.push_back({
    std::regex(R"(^SELECT\s+(\*|[\w\s\(\)\*,\.]+)\s+FROM\s+([\w\s,]+)((?:\s+JOIN\s+\w+\s+ON\s+[\w\.]+\s*=\s*[\w\.]+)+)?(?:\s+WHERE\s+(.+?))?(?:\s+GROUP\s+BY\s+(.+?))?(?:\s+ORDER\s+BY\s+(.+?))?(?:\s+HAVING\s+(.+?))?\s*;$)", std::regex::icase),
    [this](const std::smatch& m) { handleSelect(m); }
        });


    /*  DCL  */
    //√
    patterns.push_back({
    std::regex(R"(^USE\s+(?:DATABASE\s+)?(\w+);$)", std::regex::icase),
    [this](const std::smatch& m) { handleUseDatabase(m); }
        });

    /* 用户权限管理相关DCL */

    // CREATE USER xxx IDENTIFIED BY xxx;
    patterns.push_back({
        std::regex(R"(^CREATE\s+USER\s+(\w+)\s+IDENTIFIED\s+BY\s+(\w+);$)", std::regex::icase),
        [this](const std::smatch& m) { handleCreateUser(m); }
        });

    // GRANT conn|resource TO xxx;
    patterns.push_back({
        std::regex(R"(^GRANT\s+(conn|resource)\s+TO\s+(\w+);$)", std::regex::icase),
        [this](const std::smatch& m) { handleGrantPermission(m); }
        });

    // REVOKE conn|resource FROM xxx;
    patterns.push_back({
        std::regex(R"(^REVOKE\s+(conn|resource)\s+FROM\s+(\w+);$)", std::regex::icase),
        [this](const std::smatch& m) { handleRevokePermission(m); }
        });

    //SHOW USERS;
    patterns.push_back({
    std::regex(R"(^SHOW\s+USERS;$)", std::regex::icase),
    [this](const std::smatch& m) { handleShowUsers(m); }
        });
}

void Parse::execute(const QString& sql_qt) {
    // 1. 清理 SQL 字符串
    qDebug() << "原始SQL:" << sql_qt;
    QString cleanedSQL = cleanSQL(sql_qt);  // 使用 cleanSQL 来处理输入
    std::string sql = cleanedSQL.toStdString();  // 转为 std::string 类型

    // 2. 转换为大写（除引号内的内容不变）
    std::string upperSQL = toUpperPreserveQuoted(sql);
    qDebug() << "清理后SQL:" << QString::fromStdString(upperSQL);
    qDebug() << "清理后SQL长度：" << upperSQL.size();  // 打印长度确认是否有异常字符

    // 直接判断事务；TODO：判断之后仍会进入正则匹配，此时尚未注册
    if (std::regex_search(upperSQL, std::regex("^START TRANSACTION;$"))) {
        TransactionManager::instance().begin();
        return;
    }
    if (std::regex_search(upperSQL, std::regex("^COMMIT;$"))) {
        TransactionManager::instance().commit();
        return;
    }
    if (std::regex_search(upperSQL, std::regex("^ROLLBACK;$"))) {
        TransactionManager::instance().rollback();
        return;
    }

    // 3. 遍历所有正则模式并匹配
    for (const auto& p : patterns) {
        std::smatch match;

        // 4. 正则匹配
        if (std::regex_match(upperSQL, match, p.pattern)) {
            p.action(match);  // 执行匹配后的动作
            return;
        }
    }

    // 如果没有找到匹配的模式，可以选择抛出异常或返回错误
    Output::printError(outputEdit, "SQL 语句格式错误或不支持的 SQL 类型");
}
