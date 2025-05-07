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
std::string ToGbk(const std::string& utf8)
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

std::string Parse::executeSQL(const std::string& sql)
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
        Output::setOstream(&output);

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





/*std::string Parse::executeSQL(const std::string& sql)
{
    std::ostringstream output;
    try {
        std::string cleanedSQL = trim(sql);
        if (cleanedSQL.empty())
            return "";
        std::smatch m;
        //创建
        if (std::regex_match(cleanedSQL, m, std::regex(R"(CREATE\s+DATABASE\s+(\w+);?)", std::regex::icase)))
        {
            try {
                dbManager::getInstance().create_user_db(m[1]);
                output << "数据库 '" << m[1] << "' 创建成功！";
            }
            catch (const std::exception& e) {
                output << "数据库创建失败: " << e.what();
            }
            return output.str();
        }
        //删除
        if (std::regex_match(cleanedSQL, m, std::regex(R"(DROP\s+DATABASE\s+(\w+);?)", std::regex::icase))) 
        {
            try {
                dbManager::getInstance().delete_user_db(m[1]);
                output << "数据库 '" << m[1] << "' 删除成功！";
            }
            catch (const std::exception& e) {
                output << "数据库删除失败: " << e.what();
            }
            return output.str();
        }
        //使用
        if (std::regex_match(cleanedSQL, m, std::regex(R"(USE\s+(\w+);?)", std::regex::icase))) {
            try {
                dbManager::getInstance().useDatabase(m[1]);
                output << "已成功切换到数据库 '" << m[1] << "'.";
            }
            catch (const std::exception& e) {
                output << "切换数据库失败: " << e.what();
            }
            return output.str();
        }
        //show
        if (std::regex_match(cleanedSQL, m, std::regex(R"(SHOW\s+DATABASES;?)", std::regex::icase))) {
            try {
                auto dbs = dbManager::getInstance().get_database_list_by_db();
                output << "数据库列表：\n";
                for (const auto& db : dbs) output << "  " << db << "\n";
            }
            catch (const std::exception& e) {
                output << "获取数据库列表失败: " << e.what();
            }
            return output.str();
        }
        if (std::regex_match(cleanedSQL, m, std::regex(R"(SELECT\s+\*\s+FROM\s+(\w+);?)", std::regex::icase))) {
            try {
                std::string tableName = m[1];
                Database* db = dbManager::getInstance().get_current_database();
                if (!db) {
                    output << "未选择数据库，请先使用 USE 语句切换数据库。";
                    return output.str();
                }
                Table* table = db->getTable(tableName);
                if (!table) {
                    output << ToGbk( "表 '") << tableName << ToGbk( "' 不存在！");
                    return output.str();
                }
                // 输出表头
                auto fields = table->getFieldNames();
                for (const auto& f : fields) output << ToGbk(f) << "\t";
                output << "\n";
                // 输出所有记录
                auto records = table->selectAll(); // 假设返回 vector<vector<string>>
                for (const auto& rec : records) {
                    for (const auto& val : rec) output << ToGbk(val) << "\t";
                    output << "\n";
                }
            }
            catch (const std::exception& e) {
                output << "SELECT 执行失败: " << e.what();
            }
            return output.str();
        }
      
        output << "不支持的SQL语句: " << cleanedSQL;
        return output.str();
    }
    catch (const std::exception& e) 
    {
        return std::string("SQL执行异常: ") + e.what();
    }
}
*/
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
        if (TransactionManager::instance().isActive())
        {
            Output::printError(outputEdit, QString("已有事务正在进行中。"));
            return;
        }
        TransactionManager::instance().begin();
		Output::printMessage(outputEdit, "事务开始");
        return;
    }
    if (std::regex_search(upperSQL, std::regex("^COMMIT;$"))) {
        if (!TransactionManager::instance().isActive())
        {
            Output::printError(outputEdit, QString("当前没有活动的事务，无法提交"));
            return;
        }
        TransactionManager::instance().commit();
		Output::printMessage(outputEdit, "事务结束。成功提交。");
        return;
    }
    if (std::regex_search(upperSQL, std::regex("^ROLLBACK;$"))) {
        if (!TransactionManager::instance().isActive())
        {
            Output::printError(outputEdit, QString("当前没有活动的事务，无法回滚。"));
            return;
        }
        int rollback_count = TransactionManager::instance().rollback();
        // 使用 Output 类输出成功回滚的记录数
        Output::printMessage(outputEdit, QString("事务结束。成功回滚了 %1 条记录。").arg(rollback_count));
        return;
    }
    // 关闭自动提交
    if (std::regex_search(upperSQL, std::regex(R"(^SET\s+AUTOCOMMIT\s*=\s*0\s*;$)", std::regex::icase))) {
        TransactionManager::instance().setAutoCommit(false);
        Output::printMessage(outputEdit, "自动提交已关闭");
        return;
    }
    // 开启自动提交
    if (std::regex_search(upperSQL, std::regex(R"(^SET\s+AUTOCOMMIT\s*=\s*1\s*;$)", std::regex::icase))) {
        if (TransactionManager::instance().isActive())
        {
            Output::printError(outputEdit, "正处在事务中，自动提交默认关闭");
            return;
        }
        TransactionManager::instance().setAutoCommit(true);  // 注意这里也应该是 true
        Output::printMessage(outputEdit, "自动提交已开启");
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
