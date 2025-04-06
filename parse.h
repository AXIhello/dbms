#ifndef PARSE_H
#define PARSE_H

#include <QString>
#include <regex>
#include <functional>
#include <vector>
#include <string>
class Parse {
public:
    Parse();  // 
    void execute(const QString& sql);
   

private:
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
   
    void handleAlterTable(const std::smatch& m);

    void handleShowDatabases(const std::smatch& m);
    void handleShowTables(const std::smatch& m);
    void handleShowColumns(const std::smatch& m);

};

#endif // PARSE_H
