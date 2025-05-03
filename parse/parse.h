#ifndef PARSE_H
#define PARSE_H
#include <QTextEdit>
#include <QString>
#include <functional>
#include <vector>
#include"base/record/Record.h"
#include "base/table/table.h"
#include "base/database.h"
#include <string>
#include "ui/output.h"
#include "ui/mainWindow.h"

#include "manager/dbManager.h"
#include "base/record/Record.h"
#include "base/user.h"
#include "base/database.h"
#include "base/block/fieldBlock.h"
#include "base/block/constraintBlock.h"
#include <QRegularExpression>
#include <QStringList>
#include <iostream>
#include <sstream>
#include <regex>
#include <vector>
#include <algorithm>

class Parse {
public:
    Parse();
    Parse(QTextEdit* outputEdit, MainWindow* mainWindow = nullptr);
    Parse(Database* database); 
    std::string executeSQL(const std::string& sql);
    void execute(const QString& sql);
    

    //util
    static std::string trim(const std::string& s);
    
private:
	QTextEdit* outputEdit;  // 输出编辑器指针

    MainWindow* mainWindow;
    struct SqlPattern {
        std::regex pattern;
        std::function<void(const std::smatch&)> action;
    };

    Database* db;
    std::vector<SqlPattern> patterns;
    void registerPatterns();

    //utility
    QString cleanSQL(const QString& sql);//清理sql结构，去除多余空格/制表符等
    
    
    std::vector<std::string> splitDefinition(const std::string& input);
    std::string toUpper(const std::string& str);
    std::string toLower(const std::string& input);
    std::string toUpperPreserveQuoted(const std::string& input);
    //void parseFieldAndConstraints(const std::string& def, std::vector<FieldBlock>& fields, std::vector<ConstraintBlock>& constraints, int& fieldIndex);
    std::vector<std::string> split(const std::string& str, char delimiter);
    int getTypeFromString(const std::string& columnType);//类型转换函数


    //DQL(查询，显示）
    void handleSelect(const std::smatch& m);
    void handleShowDatabases(const std::smatch& m);
    void handleShowTables(const std::smatch& m);
    void handleSelectDatabase();
    void handleShowColumns(const std::smatch& m);

    void handleCreateIndex(const std::smatch& m);
    void handleDropIndex(const std::smatch& m);

    //DDL
    void handleCreateDatabase(const std::smatch& m);
    void handleDropDatabase(const std::smatch& m);
   
	void handleCreateTable(const std::smatch& m);
    void handleDropTable(const std::smatch& m);
    
    void handleAddColumn(const std::smatch& m);
    void handleDropColumn(const std::smatch& m);
    void handleModifyColumn(const std::smatch& m);

    void handleAddConstraint(const std::smatch& m);


    //专门处理foreign key
	void handleAddForeignKey(const std::smatch& m);

    void handleDropConstraint(const std::smatch& m);

    //DML
    void handleInsertInto(const std::smatch& m);
    void handleUpdate(const std::smatch& m);
    void handleDelete(const std::smatch& m);

    //DCL
    void handleUseDatabase(const std::smatch& m);

    void handleCreateUser(const std::smatch& m);
    void handleGrantPermission(const std::smatch& m);
    void handleRevokePermission(const std::smatch& m);
    void handleShowUsers(const std::smatch& m);
     

};

#endif // PARSE_H
