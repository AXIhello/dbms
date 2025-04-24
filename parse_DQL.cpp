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



/*void Parse::handleSelect(const std::smatch& m) {
    std::string columns = m[1]; // 获取列名部分（可能是 '*' 或 'id, name'）
    std::string table_name = m[2];
    std::string table_path = dbManager::getInstance().get_current_database()->getDBPath() + "/" + table_name;

    // 权限检查
    // if (!user::hasPermission("select|" + table_name)) {
      //  Output::printError(outputEdit, "无权限访问表 '" + QString::fromStdString(table_name) + "'。");
       // return;
    //}

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
*/

void Parse::handleSelect(const std::smatch& m) {
    std::string columns = m[1]; // 获取列名部分（可能是 '*' 或 'id, name'）
    std::string main_table_name = m[2]; // 获取主表名部分

    // 初始化 JoinInfo 对象
    std::shared_ptr<JoinInfo> join_info = std::make_shared<JoinInfo>();

    // 解析 JOIN 信息
    for (size_t i = 3; i + 2 < m.size(); i += 4) {
        if (m[i].matched) {
            std::string join_table = m[i].str();
            std::string left_column = m[i + 1].str();
            std::string right_column = m[i + 2].str();

            join_info->tables.push_back(join_table);
            join_info->join_conditions.emplace_back(left_column, right_column);
        }
    }

    std::string table_name_for_select;
    if (!join_info->tables.empty()) {
        // 如果有 JOIN 子句，第一个表名是主表名，后续的是 JOIN 的表名
        join_info->tables.insert(join_info->tables.begin(), main_table_name);
        // 为了兼容 Record::select 的逻辑，这里我们传递主表名，实际的表在 join_info 中
        table_name_for_select = main_table_name;
    }
    else {
        // 如果没有 JOIN 子句，可能是单表查询或隐式多表连接
        table_name_for_select = main_table_name;
        if (table_name_for_select.find(',') != std::string::npos) {
            std::stringstream ss(table_name_for_select);
            std::string token;
            join_info->tables.clear(); // 确保 join_info 的 tables 是最新的
            while (std::getline(ss, token, ',')) {
                token.erase(0, token.find_first_not_of(" \t"));
                token.erase(token.find_last_not_of(" \t") + 1);
                join_info->tables.push_back(token);
            }
        }
    }

    try {
        std::string condition = (m.size() > 4 && m[4].matched) ? m[4].str() : "";
        std::string group_by = (m.size() > 5 && m[5].matched) ? m[5].str() : "";
        std::string order_by = (m.size() > 6 && m[6].matched) ? m[6].str() : "";
        std::string having = (m.size() > 7 && m[7].matched) ? m[7].str() : "";

        // 正确传参：第二个参数是主表名，join 信息通过 join_info 传递
        auto records = Record::select(columns, table_name_for_select, condition, group_by, order_by, having, join_info.get());

        Output::printSelectResult(outputEdit, records);
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, "查询失败: " + QString::fromStdString(e.what()));
    }

}

