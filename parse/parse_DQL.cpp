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
        std::vector<std::string> tableNames = dbManager::getInstance().get_current_database()->getAllTableNames();

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
    try {
        std::string columns = m[1];
        std::string table_part = m[2];
        std::string join_part;
        if (m.size() > 3 && m[3].matched) {
            join_part = m[3].str();
        }
        std::string condition;
        if (m.size() > 4 && m[4].matched) {
            condition = m[4].str();
        }
        std::string group_by;
        if (m.size() > 5 && m[5].matched) {
            group_by = m[5].str();
        }
        std::string order_by;
        if (m.size() > 6 && m[6].matched) {
            order_by = m[6].str();
        }
        std::string having;
        if (m.size() > 7 && m[7].matched) {
            having = m[7].str();
        }

        // 解析 table_part （多个表）
        std::vector<std::string> tables;
        {
            std::stringstream ss(table_part);
            std::string table;
            while (std::getline(ss, table, ',')) {
                table.erase(0, table.find_first_not_of(" \t"));
                table.erase(table.find_last_not_of(" \t") + 1);
                tables.push_back(dbManager::getInstance().get_current_database()->getDBPath() + "/" + table);
            }
        }

        JoinInfo join_info;
        bool use_join_info = false;

        // 解析 join_part （JOIN 连接）
        if (!join_part.empty()) {
            use_join_info = true;
            std::regex join_regex(R"(\s+JOIN\s+(\w+)\s+ON\s+([\w\.]+)\s*=\s*([\w\.]+))", std::regex::icase);
            std::sregex_iterator it(join_part.begin(), join_part.end(), join_regex);
            std::sregex_iterator end;

            for (; it != end; ++it) {
                std::string right_table = (*it)[1];
                std::string left_field = (*it)[2];
                std::string right_field = (*it)[3];

                size_t dot_pos1 = left_field.find('.');
                size_t dot_pos2 = right_field.find('.');

                if (dot_pos1 == std::string::npos || dot_pos2 == std::string::npos) {
                    Output::printError(outputEdit, "JOIN ON 子句字段格式错误，必须是表名.字段名");
                    return;
                }

                std::string left_table = dbManager::getInstance().get_current_database()->getDBPath() + "/" +left_field.substr(0, dot_pos1);
                std::string left_col = left_field.substr(dot_pos1 + 1);
                std::string right_tab = dbManager::getInstance().get_current_database()->getDBPath() + "/" + right_field.substr(0, dot_pos2);
                std::string right_col = right_field.substr(dot_pos2 + 1);

                JoinPair jp;
                jp.left_table = left_table;  // 左表
                jp.right_table =  right_tab; // 右表
                jp.conditions.push_back({ left_col, right_col });
                join_info.joins.push_back(jp);

                // 查找是否已有这个 join pair
                auto it_pair = std::find_if(join_info.joins.begin(), join_info.joins.end(), [&](const JoinPair& jp) {
                    return jp.left_table == left_table && jp.right_table == right_tab;
                    });

                if (it_pair != join_info.joins.end()) {
                    //it_pair->conditions.push_back({ left_col, right_col });
                }
                else {
                    JoinPair jp;
                    jp.left_table =   left_table;
                    jp.right_table =  right_tab;
                    jp.conditions.push_back({ left_col, right_col });
                    join_info.joins.push_back(jp);
                }
            }
        }

        if (tables.size() > 1) {
            use_join_info = true;
        }

        join_info.tables = tables;

        QString first_table_name = QString::fromStdString(tables[0]);
        std::string dbPath = dbManager::getInstance().get_current_database()->getDBPath();
        std::string table_path = tables[0];

        // 调用 select，分情况
        std::vector<Record> records;
        if (use_join_info) {
            records = Record::select(columns, table_path, condition, group_by, order_by, having, &join_info);
        }
        else {
            records = Record::select(columns, table_path, condition, group_by, order_by, having, nullptr);
        }

        if (!records.empty()) {
            Output::printSelectResult(outputEdit, records);
        }
        else {
            Table* table = dbManager::getInstance().get_current_database()->getTable(tables[0]);
            Output::printSelectResultEmpty(outputEdit, table->getFieldNames());
        }
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, "查询失败: " + QString::fromStdString(e.what()));
    }
}





//void Parse::handleSelect(const std::smatch& m) {
//    std::string columns = m[1]; // 获取列名部分（可能是 '*' 或 'id, name'）
//    std::string main_table_name = m[2]; // 获取主表名部分
//
//    // 初始化 JoinInfo 对象
//    std::shared_ptr<JoinInfo> join_info = std::make_shared<JoinInfo>();
//
//    // 解析 JOIN 信息
//    for (size_t i = 3; i + 2 < m.size(); i += 4) {
//        if (m[i].matched) {
//            std::string join_table = m[i].str();
//            std::string left_column = m[i + 1].str();
//            std::string right_column = m[i + 2].str();
//
//            join_info->tables.push_back(join_table);
//            join_info->join_conditions.emplace_back(left_column, right_column);
//        }
//    }
//
//    std::string table_name_for_select;
//    if (!join_info->tables.empty()) {
//        // 如果有 JOIN 子句，第一个表名是主表名，后续的是 JOIN 的表名
//        join_info->tables.insert(join_info->tables.begin(), main_table_name);
//        // 为了兼容 Record::select 的逻辑，这里我们传递主表名，实际的表在 join_info 中
//        table_name_for_select = main_table_name;
//    }
//    else {
//        // 如果没有 JOIN 子句，可能是单表查询或隐式多表连接
//        table_name_for_select = main_table_name;
//        if (table_name_for_select.find(',') != std::string::npos) {
//            std::stringstream ss(table_name_for_select);
//            std::string token;
//            join_info->tables.clear(); // 确保 join_info 的 tables 是最新的
//            while (std::getline(ss, token, ',')) {
//                token.erase(0, token.find_first_not_of(" \t"));
//                token.erase(token.find_last_not_of(" \t") + 1);
//                join_info->tables.push_back(token);
//            }
//        }
//    }
//
//    try {
//        std::string condition = (m.size() > 4 && m[4].matched) ? m[4].str() : "";
//        std::string group_by = (m.size() > 5 && m[5].matched) ? m[5].str() : "";
//        std::string order_by = (m.size() > 6 && m[6].matched) ? m[6].str() : "";
//        std::string having = (m.size() > 7 && m[7].matched) ? m[7].str() : "";
//
//        // 正确传参：第二个参数是主表名，join 信息通过 join_info 传递
//        auto records = Record::select(columns, table_name_for_select, condition, group_by, order_by, having, join_info.get());
//
//        Output::printSelectResult(outputEdit, records);
//    }
//    catch (const std::exception& e) {
//        Output::printError(outputEdit, "查询失败: " + QString::fromStdString(e.what()));
//    }
//
//}
//
