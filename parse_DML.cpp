#include "manager/parse.h"

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

