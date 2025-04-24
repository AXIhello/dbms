#include "parse.h"

//#include <main.cpp>

Parse::Parse(QTextEdit* outputEdit, MainWindow* mainWindow) : outputEdit(outputEdit), mainWindow(mainWindow) {
    registerPatterns();
}
Parse::Parse(Database* database)
    : db(database) {  // 初始化 db 指针

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
    [this](const std::smatch& m) { handleCreateIndex(m); }
        });


    /*  DML  */
    //√
    patterns.push_back({
     std::regex(R"(^INSERT\s+INTO\s+(\w+)\s*(?:\(([^)]+)\))?\s*VALUES\s*\(([^)]+)\);$)", std::regex::icase),
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

    //√ (匹配 select * )
   // patterns.push_back({
     //std::regex(R"(^SELECT\s+(\*|[\w\s\(\)\*,]+)\s+FROM\s+(\w+)(?:\s+WHERE\s+(.+?))?(?:\s+GROUP\s+BY\s+(.+?))?(?:\s+ORDER\s+BY\s+(.+?))?(?:\s+HAVING\s+(.+?))?\s*;$)", std::regex::icase),
    //[this](const std::smatch& m) { handleSelect(m); }
      //});

    patterns.push_back({
    std::regex(R"(^SELECT\s+(\*|[\w\s\(\)\*,]+)\s+FROM\s+([\w\s,]+)(?:\s+WHERE\s+(.+?))?(?:\s+JOIN\s+(\w+)\s+ON\s+([\w\.]+)\s*=\s*([\w\.]+))?(?:\s+GROUP\s+BY\s+(.+?))?(?:\s+ORDER\s+BY\s+(.+?))?(?:\s+HAVING\s+(.+?))?\s*;$)", std::regex::icase),
    [this](const std::smatch& m) { handleSelect(m); }
        });


    /*  DCL  */
    //√
    patterns.push_back({
    std::regex(R"(^USE\s+(?:DATABASE\s+)?(\w+);$)", std::regex::icase),
    [this](const std::smatch& m) { handleUseDatabase(m); }
        });
}

void Parse::execute(const QString& sql_qt) {
    // 1. 清理 SQL 字符串
    QString cleanedSQL = cleanSQL(sql_qt);  // 使用 cleanSQL 来处理输入
    std::string sql = cleanedSQL.toStdString();  // 转为 std::string 类型

	// 2. 转换为大写（除引号内的内容不变）
    std::string upperSQL = toUpperPreserveQuoted(sql);

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