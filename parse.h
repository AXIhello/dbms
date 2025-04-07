#ifndef PARSE_H
#define PARSE_H
#include <QTextEdit>
#include <QString>
#include <regex>
#include <functional>
#include <vector>
#include"Record.h"
#include "table.h"
#include "database.h"
#include <string>
class Parse {
public:
    Parse(QTextEdit* outputEdit);  
    void execute(const QString& sql);
   
    Parse(Database* database);
    std::vector<std::string> splitString(const std::string& str, char delimiter);
    std::string joinStrings(const std::vector<std::string>& strings, char delimiter);
    std::string trim(const std::string& str);

private:
	QTextEdit* outputEdit;  // 输出编辑器指针
    struct SqlPattern {
        std::regex pattern;
        std::function<void(const std::smatch&)> action;
    };

    std::vector<SqlPattern> patterns;
   
    void registerPatterns();

    void handleCreateDatabase(const std::smatch& m);
    void handleDropDatabase(const std::smatch& m);
    void handleInsertInto(const std::smatch& m);
    void handleSelect(const std::smatch& m);

    void handleAddColumn(const std::smatch& m);
    void handleDeleteColumn(const std::smatch& m);
    void handleUpdateColumn(const std::smatch& m);
   
    void handleShowDatabases(const std::smatch& m);
    void handleShowTables(const std::smatch& m);
    void handleShowColumns(const std::smatch& m);

    Database* db;
};

#endif // PARSE_H
