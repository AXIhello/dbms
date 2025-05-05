#include "parse/parse.h"

void Parse::handleInsertInto(const std::smatch& m) {
    std::string table_name = m[1];
    std::string cols = m[2];  // 可能为空
    std::string vals = m[3];  // 可能是多个 (....), (....)

    // 去掉列名部分的括号（如果有）
    auto trimParens = [](std::string& s) {
        if (!s.empty() && s.front() == '(' && s.back() == ')') {
            s = s.substr(1, s.length() - 2);
        }
        };
    trimParens(cols);

    try {
        // 拆分多个 VALUES (...) 块
        std::vector<std::string> values_list;
        std::regex val_regex(R"(\(([^)]*)\))");
        auto vals_begin = std::sregex_iterator(vals.begin(), vals.end(), val_regex);
        auto vals_end = std::sregex_iterator();
        for (auto it = vals_begin; it != vals_end; ++it) {
            values_list.push_back((*it)[1].str());  // 提取括号内的内容
        }

        if (values_list.empty()) {
            throw std::runtime_error("未检测到有效的VALUES数据");
        }

        // 执行插入
        Record r;
        int count = 0;
        for (const auto& val_block : values_list) {
            r.insert_record(table_name, cols, val_block);
            ++count;
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

