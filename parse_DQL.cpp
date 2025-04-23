#include "manager/parse.h"

void Parse::handleSelectDatabase() {
    try {
        std::string dbName = dbManager::getInstance().getCurrentDatabase()->getDBName();
        Output::printMessage(outputEdit, "当前数据库为： " + QString::fromStdString(dbName));
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, e.what());
    }
}

void Parse::handleShowDatabases(const std::smatch& m) {
    auto dbs = dbManager::getInstance().getDatabaseList();

    Output::printDatabaseList(outputEdit, dbs);
}


void Parse::handleShowTables(const std::smatch& m) {
    try {
        Database* db = dbManager::getInstance().getCurrentDatabase();
        std::vector<std::string> tableNames = db->getAllTableNames();

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

void Parse::handleSelect(const std::smatch& m) {
    std::string columns = m[1]; // 获取列名部分（可能是 '*' 或 'id, name'）
    std::string table_name = m[2];
    std::string table_path = dbManager::getInstance().getCurrentDatabase()->getDBPath() + "/" + table_name;

    // 权限检查
    /* if (!user::hasPermission("select|" + table_name)) {
        Output::printError(outputEdit, "无权限访问表 '" + QString::fromStdString(table_name) + "'。");
        return;
    }*/

    try {
        std::string condition;
        if (m.size() > 3 && m[3].matched) {
            condition = m[3].str();
        }

        std::string group_by;
        if (m.size() > 4 && m[4].matched) {
            group_by = m[4].str();
        }

        std::string order_by;
        if (m.size() > 5 && m[5].matched) {
            order_by = m[5].str();
        }

        std::string having;
        if (m.size() > 6 && m[6].matched) {
            having = m[6].str();
        }

        // 调用封装了 where 逻辑的 Record::select
        auto records = Record::select(columns, table_path, condition, group_by, order_by, having);

        // 显示查询结果
        Output::printSelectResult(outputEdit, records);
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, "查询失败: " + QString::fromStdString(e.what()));
    }
}

