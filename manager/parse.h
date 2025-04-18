#ifndef PARSE_H
#define PARSE_H
#include <QTextEdit>
#include <QString>
#include <regex>
#include <functional>
#include <vector>
#include"base/record/Record.h"
#include "base/table.h"
#include "base/database.h"
#include <string>
#include "ui/mainWindow.h"

class Parse {
public:
    Parse(QTextEdit* outputEdit, MainWindow* mainWindow = nullptr);
    void execute(const QString& sql);
   
    Parse(Database* database);
    
private:
	QTextEdit* outputEdit;  // 输出编辑器指针
    MainWindow* mainWindow;
    struct SqlPattern {
        std::regex pattern;
        std::function<void(const std::smatch&)> action;
    };

    std::vector<SqlPattern> patterns;
    QString cleanSQL(const QString& sql);//清理sql结构，去除多余空格/制表符等
    
    std::string trim(const std::string& s); //工具用
    std::vector<std::string> splitDefinition(const std::string& input);
    std::string toUpper(const std::string& str);
    std::string toLower(const std::string& input);


    
    int getTypeFromString(const std::string& columnType);//类型转换函数


    void registerPatterns();

    void handleCreateDatabase(const std::smatch& m);
    void handleDropDatabase(const std::smatch& m);
    void handleUseDatabase(const std::smatch& m);
    void handleInsertInto(const std::smatch& m);
    void handleSelect(const std::smatch& m);
	void handleCreateTable(const std::smatch& m);

    void handleAddColumn(const std::smatch& m);
    void handleDropColumn(const std::smatch& m);
    void handleModifyColumn(const std::smatch& m);
   
    void handleShowDatabases(const std::smatch& m);
    void handleShowTables(const std::smatch& m);
    void handleShowColumns(const std::smatch& m);

    void handleSelectDatabase();

    void handleDropTable(const std::smatch& m);

    void handleUpdate(const std::smatch& m);
    void handleDelete(const std::smatch& m);

    Database* db;

};

#endif // PARSE_H
