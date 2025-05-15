#include "parse/parse.h"

void Parse::handleInsertInto(const std::smatch& m) {
    std::string table_name = m[1];
    std::string dbName = dbManager::getCurrentDBName();
    if (!(user::hasPermission("CONNECT", dbName) && user::hasPermission("RESOURCE", dbName))) {
        Output::printError(outputEdit, QString::fromStdString("权限不足，无法向表 " + table_name+ "插入数据。"));
        return;
    }
    
    std::string cols = m[2];  // 可为空
    std::string vals = m[3];  // 可能是多个 (....), (....)

    // 去掉列名部分的括号（如果有）
    auto trimParens = [](std::string& s) {
        if (!s.empty() && s.front() == '(' && s.back() == ')') {
            s = s.substr(1, s.length() - 2);
        }
        };
    trimParens(cols);

    try {
        std::vector<std::string> valueBlocks;
        std::string current;
        int depth = 0;
        for (size_t i = 0; i < vals.size(); ++i) {
            char c = vals[i];
            if (c == '(') {
                if (depth == 0) current.clear();
                depth++;
            }

            if (depth > 0) current += c;

            if (c == ')') {
                depth--;
                if (depth == 0) {
                    valueBlocks.push_back(current);
                }
            }
        }

        if (valueBlocks.empty()) {
            throw std::runtime_error("未检测到有效的 VALUES 数据");
        }

        int count = 0;
        for (const std::string& val_block : valueBlocks) {
            std::string inner = val_block.substr(1, val_block.size() - 2); // 去掉 ()
            Record r;
            r.insert_record(table_name, cols, inner);
            ++count;

            if (count % 1000 == 0) {
                qDebug() << "已插入" << count << "条...";
            }
        }

        Output::printMessage(outputEdit, QString::fromStdString(
            "INSERT INTO 执行成功：已插入 " + std::to_string(count) + " 条记录。"
        ));
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, QString::fromStdString(e.what()));
    }
}




void Parse::handleUpdate(const std::smatch& m) {
    std::string tableName = m[1];
    std::string dbName = dbManager::getCurrentDBName();
    if (!(user::hasPermission("CONNECT", dbName) && user::hasPermission("RESOURCE", dbName))) {
        Output::printError(outputEdit, QString::fromStdString("权限不足，无法更新表 " + tableName + "的数据。"));
        return;
    }
    std::string setClause = m[2];  // SET 子句
    std::string condition = m.size() > 3 ? m[3].str() : "";

        

    // 创建 Record 对象来执行更新操作
    Record record;
    try {
        
        int num=record.update(tableName, setClause, condition);

        Output::printMessage(outputEdit, QString::fromStdString("UPDATE 执行成功：已更新"+std::to_string(num)+"条记录。"));
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, QString::fromStdString(e.what()));
    }
}



void Parse::handleDelete(const std::smatch& m) {
    std::string table_name = m[1];
    std::string dbName = dbManager::getCurrentDBName();
    if (!(user::hasPermission("CONNECT", dbName) && user::hasPermission("RESOURCE", dbName))) {
        Output::printError(outputEdit, QString::fromStdString("权限不足，无法删除表 " + table_name + "中的数据。"));
        return;
    }
    std::string condition = m[2];   // 删除条件

    if (condition.front() == '(' && condition.back() == ')') {
        condition = condition.substr(1, condition.length() - 2);
    }

    try {

        Record r;
        int num=r.delete_(table_name, condition);

        Output::printMessage(outputEdit, QString::fromStdString("DELETE FROM 执行成功：已删除"+std::to_string(num)+"条记录。"));
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, QString::fromStdString(e.what()));
    }
}

