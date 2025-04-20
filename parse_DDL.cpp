#include "manager/parse.h"

void Parse::handleCreateDatabase(const std::smatch& m) {
    try { dbManager::getInstance().createUserDatabase(m[1]); }
    catch (const std::exception& e) {
        Output::printError(outputEdit, "数据库创建失败: " + QString::fromStdString(e.what()));
        return;
    }
    Output::printMessage(outputEdit, "数据库 '" + QString::fromStdString(m[1]) + "' 创建成功！");
    mainWindow->refreshDatabaseList();
}


void Parse::handleDropDatabase(const std::smatch& m) {
    try { dbManager::getInstance().dropDatabase(m[1]); }
    catch (const std::exception& e) {
        Output::printError(outputEdit, "数据库删除失败: " + QString::fromStdString(e.what()));
        return;
    }
    Output::printMessage(outputEdit, "数据库 '" + QString::fromStdString(m[1]) + "' 删除成功！");

}

void Parse::handleDropTable(const std::smatch& match) {
    std::string tableName = match[1];
    // 获取当前数据库
    try {
        Database* db = dbManager::getInstance().getCurrentDatabase();
        if (!db->getTable(tableName))
            throw std::runtime_error("表 '" + tableName + "' 不存在。");
        db->dropTable(tableName);
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, "表删除失败: " + QString::fromStdString(e.what()));
        return;
    }

    // 输出删除成功信息
    QString message = "表 " + QString::fromStdString(tableName) + " 删除成功";
    Output::printMessage(outputEdit, message);
}

void Parse::handleCreateTable(const std::smatch& match) {
    std::string tableName = match[1];
    std::string rawDefinition = match[2];

    Database* db = nullptr;
    try {
        // 获取当前数据库
        db = dbManager::getInstance().getCurrentDatabase();
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, QString::fromStdString("获取当前数据库失败: " + std::string(e.what())));
        return;
    }

    std::vector<FieldBlock> fields;
    std::vector<ConstraintBlock> constraints;
    int fieldIndex = 0;

    try {
        // 解析字段和约束
        std::vector<std::string> definitions = split(rawDefinition, ','); 
        for (const std::string& def : definitions) {
            parseFieldAndConstraints(def, fields, constraints, fieldIndex);
        }
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, QString::fromStdString("字段解析失败: " + std::string(e.what())));
        return;
    }

    try {
        // 创建表
        db->createTable(tableName, fields, constraints);
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, "表创建失败: " + QString::fromStdString(e.what()));
        return;
    }

    Output::printMessage(outputEdit, "表 " + QString::fromStdString(tableName) + " 创建成功");
}
