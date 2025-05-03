#include "parse/parse.h"
#include <set>
// 小写无关字符串比较，返回 true 则相同
bool iequals(const std::string& a, const std::string& b) {
    return std::equal(a.begin(), a.end(), b.begin(), b.end(),
        [](char a, char b) {
            return tolower(a) == tolower(b);
        });
}

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

#include <chrono>  // 加头文件

void Parse::handleSelect(const std::smatch& m) {
    try {
        // 开始计时
        auto start_time = std::chrono::high_resolution_clock::now();

        std::string columns = m[1];
        std::string table_part = m[2];
        std::string join_part;
        if (m.size() > 3 && m[3].matched) {
            join_part = m[3].str();
        }
        std::string condition, group_by, order_by, having;
        if (m.size() > 4 && m[4].matched) condition = m[4].str();
        if (m.size() > 5 && m[5].matched) group_by = m[5].str();
        if (m.size() > 6 && m[6].matched) order_by = m[6].str();
        if (m.size() > 7 && m[7].matched) having = m[7].str();

        // 解析 FROM 后面的所有表（初始表）
        std::vector<std::string> tables;
        {
            std::stringstream ss(table_part);
            std::string table;
            while (std::getline(ss, table, ',')) {
                table.erase(0, table.find_first_not_of(" \t"));
                table.erase(table.find_last_not_of(" \t") + 1);
                tables.push_back(table);
            }
        }

        JoinInfo join_info;
        join_info.tables = tables; // 初始加入 from 后所有表

        bool use_join_info = false;

        // 解析 JOIN ... ON ... 部分
        if (!join_part.empty()) {
            use_join_info = true;
            std::regex join_regex(R"(\s+JOIN\s+(\w+)\s+ON\s+([\w\.]+)\s*=\s*([\w\.]+))", std::regex::icase);
            std::sregex_iterator it(join_part.begin(), join_part.end(), join_regex), end;

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

                std::string left_table = left_field.substr(0, dot_pos1);
                std::string left_col = left_field.substr(dot_pos1 + 1);
                std::string right_tab = right_field.substr(0, dot_pos2);
                std::string right_col = right_field.substr(dot_pos2 + 1);

                // 检查如果 right_table 没有在 tables 中，则补充进去
                bool exists_r = false;
                for (const auto& t : join_info.tables) {
                    if (iequals(t, right_table)) {
                        exists_r = true;
                        break;
                    }
                }
                if (!exists_r) join_info.tables.push_back(right_table);

                // 加入一组 JoinPair
                JoinPair jp;
                jp.left_table = left_table;
                jp.right_table = right_tab;
                jp.conditions.push_back({ left_col, right_col });

                join_info.joins.push_back(jp);
            }
        }

        if (join_info.tables.size() > 1) {
            use_join_info = true;
        }

        std::vector<Record> records;
        if (use_join_info) {
            records = Record::select(columns, join_info.tables[0], condition, group_by, order_by, having, &join_info);
        }
        else {
            records = Record::select(columns, join_info.tables[0], condition, group_by, order_by, having, nullptr);
        }
        // 结束计时
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

        if (!records.empty()) {
            Output::printSelectResult(outputEdit, records,duration);
        }
        else {
            Table* table = dbManager::getInstance().get_current_database()->getTable(join_info.tables[0]);
            Output::printSelectResultEmpty(outputEdit, table->getFieldNames());
        }

    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, "查询失败: " + QString::fromStdString(e.what()));
    }
}
