#include "manager/parse.h"

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

    std::string columns = m[1]; // 获取列名（可能是 '*' 或 'id, name'）
    std::string from_clause = m[2]; // 可能是单表、多表，或带 JOIN 的部分

    //std::string table_name = m[2];
    //std::string table_path = dbManager::getInstance().get_current_database()->getDBPath() + "/" + table_name;
    
    // 权限检查
    /* if (!user::hasPermission("select|" + table_name)) {
        Output::printError(outputEdit, "无权限访问表 '" + QString::fromStdString(table_name) + "'。");
        return;
    }*/

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

    try {
        std::unique_ptr<JoinInfo> join_info = nullptr;

        // 处理 JOIN 和多表情况
        if (from_clause.find("JOIN") != std::string::npos) {
            // 显式 JOIN
            join_info = std::make_unique<JoinInfo>();
            std::regex join_regex(R"((\w+)\s+JOIN\s+(\w+)\s+ON\s+(.+))", std::regex::icase);
            std::smatch join_match;
            std::string temp_from_clause = from_clause;

            while (std::regex_search(temp_from_clause, join_match, join_regex)) {
                std::string t1 = trim(join_match[1]);
                std::string t2 = trim(join_match[2]);
                std::string on_clause = join_match[3];

                join_info->tables.push_back(t1);
                join_info->tables.push_back(t2);


                // 解析 ON 条件
                std::regex cond_regex(R"((\w+\.\w+)\s*=\s*(\w+\.\w+))");
                std::smatch cond_match;
                std::string temp_on_clause = on_clause;

                while (std::regex_search(temp_on_clause, cond_match, cond_regex)) {
                    join_info->join_conditions.emplace_back(cond_match[1], cond_match[2]);
                    temp_on_clause = cond_match.suffix();
                }

                temp_from_clause = join_match.suffix();
            }
        }
        else if (from_clause.find(',') != std::string::npos) {
            // 隐式 JOIN
            join_info = std::make_unique<JoinInfo>();
            std::regex implicit_join_regex(R"((\w+)\s*,\s*(\w+))");
            std::smatch implicit_join_match;
            std::string temp_from_clause = from_clause;

            while (std::regex_search(temp_from_clause, implicit_join_match, implicit_join_regex)) {
                join_info->tables.push_back(trim(implicit_join_match[1]));
                join_info->tables.push_back(trim(implicit_join_match[2]));
                temp_from_clause = implicit_join_match.suffix();
            }

            if (!condition.empty()) {
                std::regex cond_regex(R"((\w+\.\w+)\s*=\s*(\w+\.\w+))");
                std::smatch match;
                std::string temp_condition = condition;

                while (std::regex_search(temp_condition, match, cond_regex)) {
                    join_info->join_conditions.emplace_back(match[1], match[2]);
                    temp_condition = match.suffix();
                }

                // 清理残余的 AND/OR
                condition = std::regex_replace(condition, std::regex(R"((AND|OR)\s*)", std::regex::icase), "");
                condition = trim(condition);
            }
        }
        
       // 调用 Record::select 方法
        auto records = Record::select(columns, from_clause, condition, group_by, order_by, having, join_info.get());

        // 显示查询结果
        Output::printSelectResult(outputEdit, records);
 
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, "查询失败: " + QString::fromStdString(e.what()));
    }
}

