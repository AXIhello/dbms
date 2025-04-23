#include "manager/parse.h"
#include <set>

void Parse::handleSelectDatabase() {
    try {
        std::string dbName = dbManager::getInstance().get_current_database()->getDBName();
        Output::printMessage(outputEdit, "当前数据库为： " + QString::fromStdString(dbName));
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, e.what());
    }
}

void Parse::handleShowDatabases(const std::smatch& m) {
    auto dbs = dbManager::getInstance().get_database_list_by_db();

    Output::printDatabaseList(outputEdit, dbs);
}


void Parse::handleShowTables(const std::smatch& m) {
    try {
        Database* db = dbManager::getInstance().get_current_database();
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
    std::string columns = m[1];
    std::string table_name = m[2];

    try {
        std::string condition, group_by, order_by, having;

        if (m.size() > 3 && m[3].matched) condition = m[3].str();
        if (m.size() > 4 && m[4].matched) group_by = m[4].str();
        if (m.size() > 5 && m[5].matched) order_by = m[5].str();
        if (m.size() > 6 && m[6].matched) having = m[6].str();

        std::unique_ptr<JoinInfo> join_info = nullptr;

        if (m.size() > 7 && m[7].matched) {
            join_info = std::make_unique<JoinInfo>();
            std::string join_clause = m[7].str();

            std::regex join_regex(R"(JOIN\s+(\w+)\s+ON\s+([a-zA-Z0-9_\.]+)\s*=\s*([a-zA-Z0-9_\.]+))");
            std::smatch join_match;

            // 插入主表名到 JoinInfo::tables
            join_info->tables.push_back(table_name);

            while (std::regex_search(join_clause, join_match, join_regex)) {
                std::string right_table = join_match[1].str();
                join_info->tables.push_back(right_table);
                join_info->join_conditions.emplace_back(join_match[2].str(), join_match[3].str());

                join_clause = join_match.suffix().str();
            }

            table_name = join_info->tables[0];
        }

        auto records = Record::select(columns,table_name,condition,group_by,order_by,having,join_info.get() );

        Output::printSelectResult(outputEdit, records);
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, "查询失败: " + QString::fromStdString(e.what()));
    }
}

