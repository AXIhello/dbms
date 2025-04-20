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


void Parse::handleAddColumn(const std::smatch& m) {
    std::string tableName = m[1].str();
    std::string fieldName = m[2].str();
    std::string typeStr = m[3].str();
    std::string paramStr = m[4].str();

    FieldBlock field = {};
    strncpy_s(field.name, fieldName.c_str(), sizeof(field.name) - 1);

    if (typeStr == "INT") {
        field.type = 1; field.param = 4;
    }
    else if (typeStr == "DOUBLE") {
        field.type = 2; field.param = 4;
    }
    else if (typeStr == "VARCHAR") {
        field.type = 3;
    }
    else if (typeStr == "CHAR") {
        field.type = 4;
    }
    else if (typeStr == "DATETIME") {
        field.type = 5;
    }
    else {
        Output::printError(outputEdit, QString("不支持的字段类型: %1").arg(QString::fromStdString(typeStr)));
        return;
    }

    if (!paramStr.empty()) {
        field.param = std::stoi(paramStr.substr(1, paramStr.length() - 2));
    }
    else if (field.type == 3 || field.type == 4) {
        field.param = 32;
    }

    try {
        Table* table = dbManager::getInstance().getCurrentDatabase()->getTable(tableName);
        table->addField(field);
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, QString("添加字段失败: %1").arg(QString::fromStdString(e.what())));
    }


    Output::printMessage(outputEdit, QString("字段 %1 成功添加到表 %2")
        .arg(QString::fromStdString(fieldName), QString::fromStdString(tableName)));
}


//dropField还未实现
void Parse::handleDropColumn(const std::smatch& match) {
    std::string tableName = match[1];  // 获取表名
    std::string columnName = match[2];  // 获取列名

    try {
        Table* table = db->getTable(tableName);  // 获取表对象
        if (table) {
            table->dropField(columnName);  // 调用 Table 的 dropField 方法

            Output::printMessage(outputEdit, QString::fromStdString("ALTER TABLE " + tableName + " DROP COLUMN 执行成功。"));
        }
        else {
            throw std::runtime_error("表 " + tableName + " 不存在！");
        }
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, QString::fromStdString(e.what()));
    }
}

//待修改
void Parse::handleModifyColumn(const std::smatch& match) {
    std::string tableName = match[1];  // 获取表名
    std::string oldColumnName = match[2];  // 获取旧列名
    std::string newColumnName = match[3];  // 获取新列名
    std::string newColumnType = match[4];  // 获取新列类型
    std::string paramStr = match[5];

    try {
        Table* table = db->getTable(tableName);  // 获取表对象
        if (table) {
            FieldBlock newField;
            strcpy_s(newField.name, sizeof(newField.name), newColumnName.c_str());
            newField.type = getTypeFromString(newColumnType);
            newField.param = paramStr.empty() ? 0 : std::stoi(paramStr);
            //newField.order = 0;
            newField.mtime = std::time(nullptr);
            newField.integrities = 0;

            table->updateField(oldColumnName, newField);

            Output::printMessage(outputEdit, QString::fromStdString("ALTER TABLE " + tableName + " RENAME COLUMN 执行成功。"));
        }
        else {
            throw std::runtime_error("表 " + tableName + " 不存在！");
        }
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, QString::fromStdString(e.what()));
    }
}
