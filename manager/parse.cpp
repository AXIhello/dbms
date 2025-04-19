#include "parse.h"
#include "dbManager.h"
#include "base/record/Record.h"
#include "ui/output.h"
#include "base/user.h"
#include "base/database.h"
#include"fieldBlock.h"
#include "constraintBlock.h"
#include <QRegularExpression>
#include <QStringList>
#include <iostream>
#include <sstream>
#include <regex>
#include <vector>
#include <algorithm>
//#include <main.cpp>

Parse::Parse(QTextEdit* outputEdit, MainWindow* mainWindow) : outputEdit(outputEdit), mainWindow(mainWindow) {
    registerPatterns();
}
Parse::Parse(Database* database)
    : db(database) {  // 初始化 db 指针

}



void Parse::registerPatterns() {
    /*对数据库级别的操作*/
    //DONE
    patterns.push_back({
        std::regex(R"(^CREATE\s+DATABASE\s+(\w+);$)", std::regex::icase),
        [this](const std::smatch& m) { handleCreateDatabase(m); }
        });

    //DONE
    patterns.push_back({
    std::regex(R"(^USE\s+(?:DATABASE\s+)?(\w+);$)", std::regex::icase),
    [this](const std::smatch& m) { handleUseDatabase(m); }
        });

    //DONE
    patterns.push_back({
        std::regex(R"(^DROP\s+DATABASE\s+(\w+);$)", std::regex::icase),
        [this](const std::smatch& m) { handleDropDatabase(m); }
        });

    //DONE
    patterns.push_back({
        std::regex(R"(^SHOW\s+DATABASES\s*;$)", std::regex::icase),
        [this](const std::smatch& m) { handleShowDatabases(m); }
        });

    //DONE
    patterns.push_back({
    std::regex(R"(^SELECT\s+DATABASE\s*\(\s*\)\s*;?$)", std::regex::icase),
    [this](const std::smatch& m) { handleSelectDatabase(); }
        });


    /*对表级别的操作*/
    //待修改：建表的同时规定表级完整性定义 check, 字段级完整性约束
    patterns.push_back({
    std::regex(R"(^CREATE\s+TABLE\s+(\w+)\s*\((.*)\)\s*;?$)", std::regex::icase),
    [this](const std::smatch& m) { handleCreateTable(m); }
        });

    //DONE
    patterns.push_back({
    std::regex(R"(DROP\s+TABLE\s+(\w+);)", std::regex::icase),
    [this](const std::smatch& m) { handleDropTable(m); }
        });

    //DONE
    patterns.push_back({
        std::regex(R"(^SHOW\s+TABLES\s*;$)", std::regex::icase),
        [this](const std::smatch& m) { handleShowTables(m); }
        });

    //大小写问题未完成（输入端应该大小写都可以）
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



    /*对数据的操作*/
    //DONE 
    patterns.push_back({
        std::regex(R"(^INSERT\s+INTO\s+(\w+)\s*\(([^)]+)\)\s*VALUES\s*\(([^)]+)\);$)", std::regex::icase),
        [this](const std::smatch& m) { handleInsertInto(m); }
        });

    //DONE (匹配 select * )
    patterns.push_back({
        std::regex(R"(^SELECT\s+\*\s+FROM\s+(\w+)(?:\s+WHERE\s+(.+))?;$)", std::regex::icase),
        [this](const std::smatch& m) { handleSelect(m); }
        });

    //
    patterns.push_back({
        std::regex(R"(^UPDATE\s+(\w+)\s+SET\s+(.+?)(?:\s+WHERE\s+(.+))?$)", std::regex::icase),
        [this](const std::smatch& m) { handleUpdate(m); }
        });

    //
    patterns.push_back({
        std::regex(R"(^DELETE\s+FROM\s+(\w+)\s*(?:WHERE\s+(.+))?\s*;$)", std::regex::icase),
        [this](const std::smatch& m) { handleDelete(m); }
        });

}

void Parse::execute(const QString& sql_qt) {
    // 1. 清理 SQL 字符串
    QString cleanedSQL = cleanSQL(sql_qt);  // 使用 cleanSQL 来处理输入
    std::string sql = cleanedSQL.toStdString();  // 转为 std::string 类型

    // 2. 转换为大写
    std::string upperSQL = toUpper(sql);

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


void Parse::handleCreateDatabase(const std::smatch& m) {
    try { dbManager::getInstance().createUserDatabase(m[1]); }
    catch (const std::exception& e) {
        Output::printError(outputEdit, "数据库创建失败: " + QString::fromStdString(e.what()));
        return;
    }
    Output::printMessage(outputEdit, "数据库 '" + QString::fromStdString(m[1]) + "' 创建成功！");
    mainWindow->refreshDatabaseList();
}


void Parse::handleDropDatabase(const std::smatch& m) {
    try { dbManager::getInstance().dropDatabase(m[1]); }
    catch (const std::exception& e) {
        Output::printError(outputEdit, "数据库删除失败: " + QString::fromStdString(e.what()));
        return;
    }
    Output::printMessage(outputEdit, "数据库 '" + QString::fromStdString(m[1]) + "' 删除成功！");

}

void Parse::handleUseDatabase(const std::smatch& m) {
    std::string dbName = m[1];

    try {
        // 尝试切换数据库
        dbManager::getInstance().useDatabase(dbName);

        // 成功后输出信息
        Output::printMessage(outputEdit, "已成功切换到数据库 '" + QString::fromStdString(dbName) + "'.");

    }
    catch (const std::exception& e) {
        // 捕获异常并输出错误信息
        Output::printError(outputEdit, e.what());
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



std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::string current;
    int bracketCount = 0; // 用来追踪括号层数

    for (char ch : str) {
        if (ch == '(') {
            bracketCount++; // 遇到左括号，进入括号内
        }
        if (ch == ')') {
            bracketCount--; // 遇到右括号，退出括号内
        }

        if (ch == delimiter && bracketCount == 0) {
            // 只有当括号层数为0时，才分割
            result.push_back(current);
            current.clear();
        }
        else {
            current.push_back(ch);
        }
    }

    // 将最后一个部分添加到结果中
    if (!current.empty()) {
        result.push_back(current);
    }

    return result;
}



void Parse::handleCreateTable(const std::smatch& match) {
    std::string tableName = match[1];
    std::string rawDefinition = match[2];

    Database* db = nullptr;
    try {
        db = dbManager::getInstance().getCurrentDatabase();
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, QString::fromStdString(e.what()));
        return;
    }

    std::vector<FieldBlock> fields;
    std::vector<ConstraintBlock> constraints;
    int fieldIndex = 0;

    // 分割字段定义和约束（支持括号内逗号）
    std::vector<std::string> definitions;
    int depth = 0;
    std::string token;
    for (char ch : rawDefinition) {
        if (ch == ',' && depth == 0) {
            if (!token.empty()) {
                definitions.push_back(trim(token));
                token.clear();
            }
        }
        else {
            if (ch == '(') depth++;
            else if (ch == ')') depth--;
            token += ch;
        }
    }
    if (!token.empty()) {
        definitions.push_back(trim(token));
    }

    for (const std::string& def : definitions) {
        std::string upperDef = toUpper(def);

        // === 表级约束 ===
        if (upperDef.find("PRIMARY KEY") == 0) {
            size_t start = def.find('(');
            size_t end = def.find(')');
            if (start != std::string::npos && end != std::string::npos && end > start) {
                std::string inside = def.substr(start + 1, end - start - 1);
                std::vector<std::string> keys = split(inside, ',');
                for (const std::string& key : keys) {
                    ConstraintBlock cb{};
                    cb.type = 1;
                    std::string field = toUpper(trim(key));
                    strncpy_s(cb.field, field.c_str(), sizeof(cb.field));
                    std::string pkName = "PK_" + toUpper(tableName);
                    strncpy_s(cb.name, pkName.c_str(), sizeof(cb.name));
                    cb.param[0] = '\0';
                    constraints.push_back(cb);
                }
            }
            continue;
        }

        if (upperDef.find("CHECK") == 0) {
            size_t start = def.find('(');
            size_t end = def.rfind(')');
            if (start != std::string::npos && end != std::string::npos && end > start) {
                std::string expr = trim(def.substr(start + 1, end - start - 1));
                ConstraintBlock cb{};
                cb.type = 3;
                cb.field[0] = '\0';
                strncpy_s(cb.param, expr.c_str(), sizeof(cb.param));
                std::hash<std::string> hasher;
                std::string checkName = "CHK_" + std::to_string(hasher(expr));
                strncpy_s(cb.name, checkName.c_str(), sizeof(cb.name));
                constraints.push_back(cb);
            }
            continue;
        }

        if (upperDef.find("FOREIGN KEY") == 0) {
            std::regex tableLevelFKRegex(R"(FOREIGN\s+KEY\s*\(\s*(\w+)\s*\)\s+REFERENCES\s+(\w+)\s*\(\s*(\w+)\s*\))", std::regex::icase);
            std::smatch match;
            if (std::regex_search(def, match, tableLevelFKRegex)) {
                std::string localField = toUpper(match[1].str());
                std::string refTable = toUpper(match[2].str());
                std::string refField = toUpper(match[3].str());

                ConstraintBlock cb{};
                cb.type = 2;
                std::string fkName = "FK_" + localField + "_" + refTable;
                strncpy_s(cb.name, fkName.c_str(), sizeof(cb.name));
                strncpy_s(cb.field, localField.c_str(), sizeof(cb.field));
                std::string paramStr = refTable + "(" + refField + ")";
                strncpy_s(cb.param, paramStr.c_str(), sizeof(cb.param));
                constraints.push_back(cb);
            }
            continue;
        }

        if (upperDef.find("UNIQUE") == 0) {
            size_t start = def.find('(');
            size_t end = def.find(')');
            if (start != std::string::npos && end != std::string::npos && end > start) {
                std::string inside = def.substr(start + 1, end - start - 1);
                std::vector<std::string> keys = split(inside, ',');
                for (const std::string& key : keys) {
                    ConstraintBlock cb{};
                    cb.type = 4;
                    std::string field = toUpper(trim(key));
                    strncpy_s(cb.field, field.c_str(), sizeof(cb.field));
                    std::string uniqueName = "UQ_" + field;
                    strncpy_s(cb.name, uniqueName.c_str(), sizeof(cb.name));
                    cb.param[0] = '\0';
                    constraints.push_back(cb);
                }
            }
            continue;
        }

        // === 字段定义 ===
        std::regex fieldRegex(R"(^\s*(\w+)\s+([a-zA-Z]+)(\s*\(\s*(\d+)\s*\))?)", std::regex::icase);
        std::smatch fieldMatch;
        if (!std::regex_search(def, fieldMatch, fieldRegex)) {
            Output::printError(outputEdit, "字段定义解析失败: " + QString::fromStdString(def));
            return;
        }

        std::string name = fieldMatch[1];
        std::string typeStr = toUpper(fieldMatch[2]);
        std::string paramStr = fieldMatch[4];

        FieldBlock field{};
        field.order = fieldIndex++;
        strncpy_s(field.name, name.c_str(), sizeof(field.name));
        field.name[sizeof(field.name) - 1] = '\0';
        field.mtime = std::time(nullptr);
        field.integrities = 0;

        // 类型映射
        if (typeStr == "INT") {
            field.type = 1; field.param = 4;
        }
        else if (typeStr == "BOOL") {
            field.type = 4; field.param = 1;
        }
        else if (typeStr == "DOUBLE") {
            field.type = 2; field.param = 2;
        }
        else if (typeStr == "VARCHAR") {
            field.type = 3;
            field.param = paramStr.empty() ? 255 : std::stoi(paramStr);
        }
        else if (typeStr == "DATETIME") {
            field.type = 5; field.param = 16;
        }
        else {
            Output::printError(outputEdit, "未知字段类型: " + QString::fromStdString(typeStr));
            return;
        }

        std::string rest = toUpper(def.substr(fieldMatch[0].length()));

        if (rest.find("PRIMARY KEY") != std::string::npos) {
            ConstraintBlock cb{};
            cb.type = 1;
            strncpy_s(cb.field, name.c_str(), sizeof(cb.field));
            std::string pkName = "PK_" + name;
            strncpy_s(cb.name, pkName.c_str(), sizeof(cb.name));
            constraints.push_back(cb);
        }

        std::regex fkSimpleRegex(R"(REFERENCES\s+(\w+)\s*\(\s*(\w+)\s*\))", std::regex::icase);
        std::smatch fkSimpleMatch;
        if (std::regex_search(rest, fkSimpleMatch, fkSimpleRegex)) {
            ConstraintBlock cb{};
            cb.type = 2;
            std::string refTable = toUpper(fkSimpleMatch[1].str());
            std::string refField = toUpper(fkSimpleMatch[2].str());
            std::string localField = toUpper(name);
            std::string fkName = "FK_" + localField + "_" + refTable;
            strncpy_s(cb.name, fkName.c_str(), sizeof(cb.name));
            strncpy_s(cb.field, localField.c_str(), sizeof(cb.field));
            std::string paramStr = refTable + "(" + refField + ")";
            strncpy_s(cb.param, paramStr.c_str(), sizeof(cb.param));
            constraints.push_back(cb);
        }

        std::regex checkRegex(R"(CHECK\s*\(([^)]+)\))", std::regex::icase);
        std::smatch checkMatch;
        if (std::regex_search(rest, checkMatch, checkRegex)) {
            ConstraintBlock cb{};
            cb.type = 3;
            std::string expr = checkMatch[1];
            std::string checkName = "CHK_" + toUpper(name);
            strncpy_s(cb.name, checkName.c_str(), sizeof(cb.name));
            strncpy_s(cb.field, name.c_str(), sizeof(cb.field));
            strncpy_s(cb.param, expr.c_str(), sizeof(cb.param));
            constraints.push_back(cb);
        }

        if (rest.find("UNIQUE") != std::string::npos) {
            ConstraintBlock cb{};
            cb.type = 4;
            strncpy_s(cb.field, name.c_str(), sizeof(cb.field));
            std::string uniqueName = "UNQ_" + name;
            strncpy_s(cb.name, uniqueName.c_str(), sizeof(cb.name));
            constraints.push_back(cb);
        }

        if (rest.find("NOT NULL") != std::string::npos) {
            ConstraintBlock cb{};
            cb.type = 5;
            strncpy_s(cb.field, name.c_str(), sizeof(cb.field));
            constraints.push_back(cb);
        }

        std::regex defaultRegex(R"(DEFAULT\s+([^\s,]+|CURRENT_TIMESTAMP))", std::regex::icase);
        std::smatch defaultMatch;
        if (std::regex_search(rest, defaultMatch, defaultRegex)) {
            ConstraintBlock cb{};
            cb.type = 6;
            strncpy_s(cb.field, name.c_str(), sizeof(cb.field));
            strncpy_s(cb.param, defaultMatch[1].str().c_str(), sizeof(cb.param));
            constraints.push_back(cb);
        }

        if (rest.find("AUTO_INCREMENT") != std::string::npos) {
            ConstraintBlock cb{};
            cb.type = 7;
            strncpy_s(cb.field, name.c_str(), sizeof(cb.field));
            constraints.push_back(cb);
        }

        fields.push_back(field);
    }

    try {
        db->createTable(tableName, fields, constraints);
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, "表创建失败: " + QString::fromStdString(e.what()));
        return;
    }

    Output::printMessage(outputEdit, "表 " + QString::fromStdString(tableName) + " 创建成功");
}

void Parse::handleDropTable(const std::smatch& match) {
    std::string tableName = match[1];
    // 获取当前数据库
    try {
        Database* db = dbManager::getInstance().getCurrentDatabase();
        if (!db->getTable(tableName))
            throw std::runtime_error("表 '" + tableName + "' 不存在。");
        db->dropTable(tableName);
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, "表删除失败: " + QString::fromStdString(e.what()));
        return;
    }

    // 输出删除成功信息
    QString message = "表 " + QString::fromStdString(tableName) + " 删除成功";
    Output::printMessage(outputEdit, message);
}

void Parse::handleShowTables(const std::smatch& m) {
    try {
        Database* db = dbManager::getInstance().getCurrentDatabase();
        /* if (!db) {
             Output::printError(outputEdit, "请先使用 USE 语句选择数据库");
             return;
         }*/
         // 获取表名列表
        std::vector<std::string> tableNames = db->getAllTableNames();

        /*  if (tableNames.empty()) {
              Output::printMessage(outputEdit, "当前数据库中没有表");
              return;
          }*/

          // 使用 Output 类的 printTableList 方法来美化输出
        Output::printTableList(outputEdit, tableNames);
    }
    catch (const std::exception& e) {
        // 捕获并输出异常信息
        Output::printError(outputEdit, "错误: " + QString::fromStdString(e.what()));
    }
    catch (...) {
        // 捕获未知异常
        Output::printError(outputEdit, "发生未知错误");
    }
}



void Parse::handleInsertInto(const std::smatch& m) {
    std::string table_name = m[1];
    std::string cols = m[2];
    std::string vals = m[3];

    if (cols.front() == '(' && cols.back() == ')') {
        cols = cols.substr(1, cols.length() - 2);
    }
    if (vals.front() == '(' && vals.back() == ')') {
        vals = vals.substr(1, vals.length() - 2);
    }

    try {
        std::string table_path = dbManager::getInstance().getCurrentDatabase()->getDBPath() + "/" + table_name;
        Record r;
        r.insert_record(table_path, cols, vals);
        Output::printMessage(outputEdit, QString::fromStdString("INSERT INTO 执行成功。"));
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, QString::fromStdString(e.what()));
    }
}



void Parse::handleSelect(const std::smatch& m) {
    std::string table_name = m[1];
    std::string table_path = dbManager::getInstance().getCurrentDatabase()->getDBPath() + "/" + table_name;

    // 权限检查
   /* if (!user::hasPermission("select|" + table_name)) {
        Output::printError(outputEdit, "无权限访问表 '" + QString::fromStdString(table_name) + "'。");
        return;
    }*/

    try {
        std::string columns = "*"; // 目前仅支持 SELECT *
        std::string condition;

        // 如果有 WHERE 子句则获取
        if (m.size() > 2 && m[2].matched) {
            condition = m[2].str();
        }

        // 直接调用封装了 where 逻辑的 Record::select
        auto records = Record::select(columns, table_path, condition);

        // 显示查询结果
        Output::printSelectResult(outputEdit, records);
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, "查询失败: " + QString::fromStdString(e.what()));
    }

}


//TODO : 新增了以后，如果.trd中原本有数据，则将新增的字段的值置空
void Parse::handleAddColumn(const std::smatch& m) {
    std::string tableName = m[1].str();
    std::string fieldName = m[2].str();
    std::string typeStr = m[3].str();
    std::string paramStr = m[4].str();

    FieldBlock field = {};
    strncpy_s(field.name, fieldName.c_str(), sizeof(field.name) - 1);

    if (typeStr == "INT") {
        field.type = 1; field.param = 4;
    }
    else if (typeStr == "FLOAT") {
        field.type = 2; field.param = 4;
    }
    else if (typeStr == "VARCHAR") {
        field.type = 3;
    }
    else if (typeStr == "CHAR") {
        field.type = 4;
    }
    else if (typeStr == "DATETIME") {
        field.type = 5;
    }
    else {
        Output::printError(outputEdit, QString("不支持的字段类型: %1").arg(QString::fromStdString(typeStr)));
        return;
    }

    if (!paramStr.empty()) {
        field.param = std::stoi(paramStr.substr(1, paramStr.length() - 2));
    }
    else if (field.type == 3 || field.type == 4) {
        field.param = 32;
    }

    try {
        Table* table = dbManager::getInstance().getCurrentDatabase()->getTable(tableName);
        table->addField(field);
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, QString("添加字段失败: %1").arg(QString::fromStdString(e.what())));
    }


    Output::printMessage(outputEdit, QString("字段 %1 成功添加到表 %2")
        .arg(QString::fromStdString(fieldName), QString::fromStdString(tableName)));
}


//dropField还未实现
void Parse::handleDropColumn(const std::smatch& match) {
    std::string tableName = match[1];  // 获取表名
    std::string columnName = match[2];  // 获取列名

    try {
        Table* table = db->getTable(tableName);  // 获取表对象
        if (table) {
            table->dropField(columnName);  // 调用 Table 的 dropField 方法

            Output::printMessage(outputEdit, QString::fromStdString("ALTER TABLE " + tableName + " DROP COLUMN 执行成功。"));
        }
        else {
            throw std::runtime_error("表 " + tableName + " 不存在！");
        }
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, QString::fromStdString(e.what()));
    }
}

//待修改
void Parse::handleModifyColumn(const std::smatch& match) {
    std::string tableName = match[1];  // 获取表名
    std::string oldColumnName = match[2];  // 获取旧列名
    std::string newColumnName = match[3];  // 获取新列名
    std::string newColumnType = match[4];  // 获取新列类型
    std::string paramStr = match[5];

    try {
        Table* table = db->getTable(tableName);  // 获取表对象
        if (table) {
            FieldBlock newField;
            strcpy_s(newField.name, sizeof(newField.name), newColumnName.c_str());
            newField.type = getTypeFromString(newColumnType);
            newField.param = paramStr.empty() ? 0 : std::stoi(paramStr);
            //newField.order = 0;
            newField.mtime = std::time(nullptr);
            newField.integrities = 0;

            table->updateField(oldColumnName, newField);

            Output::printMessage(outputEdit, QString::fromStdString("ALTER TABLE " + tableName + " RENAME COLUMN 执行成功。"));
        }
        else {
            throw std::runtime_error("表 " + tableName + " 不存在！");
        }
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, QString::fromStdString(e.what()));
    }
}


void Parse::handleUpdate(const std::smatch& m) {
    std::string tableName = m[1];
    std::string setClause = m[2];  // SET 子句
    std::string condition = m.size() > 3 ? m[3].str() : "";

    std::cout << "UPDATE 表名: " << tableName << std::endl;
    std::cout << "SET 子句: " << setClause << std::endl;
    std::cout << "WHERE 条件: " << condition << std::endl;

    // 创建 Record 对象来执行更新操作
    Record record;
    try {
        record.update(tableName, setClause, condition);

        // 输出
        Output::printMessage(outputEdit, QString::fromStdString("UPDATE 执行成功。"));
    }
    catch (const std::exception& e) {
        // 错误处理
        Output::printError(outputEdit, QString::fromStdString(e.what()));
    }
}

void Parse::handleDelete(const std::smatch& m) {
    std::string table_name = m[1];
    std::string condition = m[2];   // 删除条件

    if (condition.front() == '(' && condition.back() == ')') {
        condition = condition.substr(1, condition.length() - 2);
    }

    try {
        std::string table_path = dbManager::getInstance().getCurrentDatabase()->getDBPath() + "/" + table_name;

        Record r;
        r.delete_(table_name, condition);

        // 输出
        Output::printMessage(outputEdit, QString::fromStdString("DELETE FROM 执行成功。"));
    }
    catch (const std::exception& e) {
        // 错误处理
        Output::printError(outputEdit, QString::fromStdString(e.what()));
    }
}


int Parse::getTypeFromString(const std::string& columnType) {
    if (columnType == "INT") return 1;
    if (columnType == "FLOAT") return 2;
    if (columnType == "VARCHAR") return 3;
    if (columnType == "CHAR") return 4;
    if (columnType == "DATETIME") return 5;

    return -1;  // 如果是无效类型
}

std::vector<std::string> Parse::splitDefinition(const std::string& input) {
    std::vector<std::string> tokens;
    std::string current;
    int parenCount = 0;

    for (char c : input) {
        if (c == ',' && parenCount == 0) {
            tokens.push_back(current);
            current.clear();
        }
        else {
            current += c;
            if (c == '(') parenCount++;
            if (c == ')') parenCount--;
        }
    }
    if (!current.empty()) {
        tokens.push_back(current); // 最后一部分
    }

    return tokens;
}


std::string Parse::trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    auto end = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}


QString Parse::cleanSQL(const QString& sql) {
    QString cleaned = sql.trimmed();

    // 统一换行符为空格
    cleaned.replace(QRegularExpression("[\\r\\n]+"), " ");

    // 多个空格或 Tab 变单空格
    cleaned.replace(QRegularExpression("[ \\t]+"), " ");

    //// 替换括号为带空格，便于分词（不去掉括号）
    //cleaned.replace("(", " ( ");
    //cleaned.replace(")", " ) ");

    // 最终 trim，不去掉分号或括号
    return cleaned.trimmed();
}


std::string Parse::toUpper(const std::string& str) {
    std::string upperStr = str;
    std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), ::toupper);
    return upperStr;
}

std::string Parse::toLower(const std::string& input) {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return result;
}