#include "manager/parse.h"

void Parse::handleCreateDatabase(const std::smatch& m) {
    try { dbManager::getInstance().create_user_db(m[1]); }
    catch (const std::exception& e) {
        Output::printError(outputEdit, "数据库创建失败: " + QString::fromStdString(e.what()));
        return;
    }
    Output::printMessage(outputEdit, "数据库 '" + QString::fromStdString(m[1]) + "' 创建成功！");
    mainWindow->refreshTree();
}


void Parse::handleDropDatabase(const std::smatch& m) {
    try { dbManager::getInstance().delete_user_db(m[1]); }
    catch (const std::exception& e) {
        Output::printError(outputEdit, "数据库删除失败: " + QString::fromStdString(e.what()));
        return;
    }
    Output::printMessage(outputEdit, "数据库 '" + QString::fromStdString(m[1]) + "' 删除成功！");
    mainWindow->refreshTree();

}

void Parse::handleDropTable(const std::smatch& match) {
    std::string tableName = match[1];
    // 获取当前数据库
    try {
        Database* db = dbManager::getInstance().get_current_database();
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
    mainWindow->refreshTree();
}


void Parse::handleCreateTable(const std::smatch& match) {
    std::string tableName = match[1];

    std::string rawDefinition = match[2];


    std::vector<FieldBlock> fields;
    std::vector<ConstraintBlock> constraints;
    int fieldIndex = 0;

    // 分割字段定义和约束（支持括号内逗号）
    std::vector<std::string> definitions;
    int depth = 0;
    std::string token;
    for (char ch : rawDefinition) {
        if (ch == ',' && depth == 0) {
            if (!token.empty()) {
                definitions.push_back(trim(token));
                token.clear();
            }
        }
        else {
            if (ch == '(') depth++;
            else if (ch == ')') depth--;
            token += ch;
        }
    }
    if (!token.empty()) {
        definitions.push_back(trim(token));
    }

    for (const std::string& def : definitions) {
        //std::string upperDef = toUpper(def);

        // === 表级约束 ===

// PRIMARY KEY 处理
if (def.find("PRIMARY KEY") == 0) {
    size_t start = def.find('(');
    size_t end = def.find(')');
    if (start != std::string::npos && end != std::string::npos && end > start) {
        std::string inside = def.substr(start + 1, end - start - 1);
        std::vector<std::string> keys = split(inside, ',');

        // 联合主键，多个字段
        ConstraintBlock cb{};
        cb.type = 1;  // PRIMARY KEY 类型
        std::string pkName = "PK_" + toUpper(tableName);  // 生成主键约束名称
        strncpy_s(cb.name, pkName.c_str(), sizeof(cb.name));  // 存储约束名称

        // 遍历每个字段，将它们加入到约束中
        std::string allFields;
        for (const std::string& key : keys) {
            std::string field = toUpper(trim(key));  // 转大写处理字段名
            if (!allFields.empty()) {
                allFields += ", ";  // 字段之间用逗号分隔
            }
            allFields += field;

            // 存储字段名到 m_constraints.fieldName
            strncpy_s(cb.field, allFields.c_str(), sizeof(cb.field));

            // 存储字段参数到 m_constraints.param
            // 这里将字段名作为联合主键的参数
            strncpy_s(cb.param, allFields.c_str(), sizeof(cb.param));
        }

        constraints.push_back(cb);  // 添加到约束列表
    }
    continue;
}

// CHECK 处理
if (def.find("CHECK") == 0) {
    size_t start = def.find('(');
    size_t end = def.rfind(')');
    if (start != std::string::npos && end != std::string::npos && end > start) {
        std::string expr = trim(def.substr(start + 1, end - start - 1));
        ConstraintBlock cb{};
        cb.type = 3;  // CHECK 类型
        cb.field[0] = '\0';  // 没有特定字段，CHECK 通常是表级约束
        strncpy_s(cb.param, expr.c_str(), sizeof(cb.param));  // 存储 CHECK 表达式

        std::hash<std::string> hasher;
        std::string checkName = "CHK_" + std::to_string(hasher(expr));  // 生成 CHECK 约束名称
        strncpy_s(cb.name, checkName.c_str(), sizeof(cb.name));  // 存储约束名称

        constraints.push_back(cb);  // 添加到约束列表
    }
    continue;
}

// FOREIGN KEY 处理
if (def.find("FOREIGN KEY") == 0) {
    std::regex tableLevelFKRegex(R"(FOREIGN\s+KEY\s*\(\s*(\w+)\s*\)\s+REFERENCES\s+(\w+)\s*\(\s*(\w+)\s*\))", std::regex::icase);
    std::smatch match;
    if (std::regex_search(def, match, tableLevelFKRegex)) {
        std::string localField = toUpper(match[1].str());  // 本地字段
        std::string refTable = toUpper(match[2].str());   // 参考表
        std::string refField = toUpper(match[3].str());   // 参考字段

        ConstraintBlock cb{};
        cb.type = 2;  // FOREIGN KEY 类型
        std::string fkName = "FK_" + localField + "_" + refTable;  // 生成外键约束名称
        strncpy_s(cb.name, fkName.c_str(), sizeof(cb.name));  // 存储约束名称
        strncpy_s(cb.field, localField.c_str(), sizeof(cb.field));  // 存储本地字段
        std::string paramStr = refTable + "(" + refField + ")";  // 存储外键的参考信息
        strncpy_s(cb.param, paramStr.c_str(), sizeof(cb.param));  // 存储外键约束的参数

        constraints.push_back(cb);  // 添加到约束列表
    }
    continue;
}

// UNIQUE 处理
if (def.find("UNIQUE") == 0) {
    size_t start = def.find('(');
    size_t end = def.find(')');
    if (start != std::string::npos && end != std::string::npos && end > start) {
        std::string inside = def.substr(start + 1, end - start - 1);
        std::vector<std::string> keys = split(inside, ',');
        for (const std::string& key : keys) {
            ConstraintBlock cb{};
            cb.type = 4;  // UNIQUE 类型
            std::string field = toUpper(trim(key));  // 转大写处理字段名
            strncpy_s(cb.field, field.c_str(), sizeof(cb.field));  // 存储字段名称

            std::string uniqueName = "UQ_" + field;  // 生成 UNIQUE 约束名称
            strncpy_s(cb.name, uniqueName.c_str(), sizeof(cb.name));  // 存储约束名称

            cb.param[0] = '\0';  // UNIQUE 通常不附带参数
            constraints.push_back(cb);  // 添加到约束列表
        }
    }
    continue;
}

        // === 字段定义 ===
        std::regex fieldRegex(R"(^\s*(\w+)\s+([a-zA-Z]+)(\s*\(\s*(\d+)\s*\))?)", std::regex::icase);
        std::smatch fieldMatch;
        if (!std::regex_search(def, fieldMatch, fieldRegex)) {
            Output::printError(outputEdit, "字段定义解析失败: " + QString::fromStdString(def));
            return;
        }

        std::string name = fieldMatch[1];
        std::string typeStr = toUpper(fieldMatch[2]);
        std::string paramStr = fieldMatch[4];

        FieldBlock field{};
        field.order = fieldIndex++;
        strncpy_s(field.name, name.c_str(), sizeof(field.name));
        field.name[sizeof(field.name) - 1] = '\0';
        field.mtime = std::time(nullptr);
        field.integrities = 0;

        // 类型映射
        if (typeStr == "INT") {
            field.type = 1; field.param = 4;
        }
        else if (typeStr == "BOOL") {
            field.type = 4; field.param = 1;
        }
        else if (typeStr == "DOUBLE") {
            field.type = 2; field.param = 2;
        }
        else if (typeStr == "VARCHAR") {
            field.type = 3;
            field.param = paramStr.empty() ? 255 : std::stoi(paramStr);
        }
        else if (typeStr == "DATETIME") {
            field.type = 5; field.param = 16;
        }
        else {
            Output::printError(outputEdit, "未知字段类型: " + QString::fromStdString(typeStr));
            return;
        }

        std::string rest = def.substr(fieldMatch[0].length());
        
        if (rest.find("NOT NULL") != std::string::npos) {
            ConstraintBlock cb{};
            cb.type = 5;
            strncpy_s(cb.field, name.c_str(), sizeof(cb.field));
            constraints.push_back(cb);
        }

        std::regex defaultRegex(R"(DEFAULT\s+([^\s,]+|CURRENT_TIMESTAMP))", std::regex::icase);
        std::smatch defaultMatch;
        if (std::regex_search(rest, defaultMatch, defaultRegex)) {
            ConstraintBlock cb{};
            cb.type = 6;
            strncpy_s(cb.field, name.c_str(), sizeof(cb.field));
            strncpy_s(cb.param, defaultMatch[1].str().c_str(), sizeof(cb.param));
            constraints.push_back(cb);
        }

        if (rest.find("AUTO_INCREMENT") != std::string::npos) {
            ConstraintBlock cb{};
            cb.type = 7;
            strncpy_s(cb.field, name.c_str(), sizeof(cb.field));
            constraints.push_back(cb);
        }

        if (rest.find("PRIMARY KEY") != std::string::npos) {
			field.integrities = 1;
            ConstraintBlock cb{};
            cb.type = 1;
            strncpy_s(cb.field, name.c_str(), sizeof(cb.field));
            std::string pkName = "PK_" + name;
            strncpy_s(cb.name, pkName.c_str(), sizeof(cb.name));
            constraints.push_back(cb);
        }

        std::regex fkSimpleRegex(R"(REFERENCES\s+(\w+)\s*\(\s*(\w+)\s*\))", std::regex::icase);
        std::smatch fkSimpleMatch;
        if (std::regex_search(rest, fkSimpleMatch, fkSimpleRegex)) {
			field.integrities = 1;
            ConstraintBlock cb{};
            cb.type = 2;
            std::string refTable = toUpper(fkSimpleMatch[1].str());
            std::string refField = toUpper(fkSimpleMatch[2].str());
            std::string localField = toUpper(name);
            std::string fkName = "FK_" + localField + "_" + refTable;
            strncpy_s(cb.name, fkName.c_str(), sizeof(cb.name));
            strncpy_s(cb.field, localField.c_str(), sizeof(cb.field));
            std::string paramStr = refTable + "(" + refField + ")";
            strncpy_s(cb.param, paramStr.c_str(), sizeof(cb.param));
            constraints.push_back(cb);
        }

        std::regex checkRegex(R"(CHECK\s*\(([^)]+)\))", std::regex::icase);
        std::smatch checkMatch;
        if (std::regex_search(rest, checkMatch, checkRegex)) {
			field.integrities = 1;
            ConstraintBlock cb{};
            cb.type = 3;
            std::string expr = checkMatch[1];
            std::string checkName = "CHK_" + toUpper(name);
            strncpy_s(cb.name, checkName.c_str(), sizeof(cb.name));
            strncpy_s(cb.field, name.c_str(), sizeof(cb.field));
            strncpy_s(cb.param, expr.c_str(), sizeof(cb.param));
            constraints.push_back(cb);
        }

        if (rest.find("UNIQUE") != std::string::npos) {
			field.integrities = 1;
            ConstraintBlock cb{};
            cb.type = 4;
            strncpy_s(cb.field, name.c_str(), sizeof(cb.field));
            std::string uniqueName = "UNQ_" + name;
            strncpy_s(cb.name, uniqueName.c_str(), sizeof(cb.name));
            constraints.push_back(cb);
        }
        fields.push_back(field);
    }

    try {
		dbManager::getInstance().get_current_database()->createTable(tableName, fields, constraints);
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, "表创建失败: " + QString::fromStdString(e.what()));
        return;
    }

    Output::printMessage(outputEdit, "表 " + QString::fromStdString(tableName) + " 创建成功");
    mainWindow->refreshTree();
}


void Parse::handleAddColumn(const std::smatch& m) {
    std::string tableName = m[1].str();
    std::string fieldName = m[2].str();
    std::string typeStr = m[3].str();
    std::string paramStr = m[4].str();
    std::string optionalConstraint = m[5].str();
    paramStr = std::regex_replace(paramStr, std::regex(R"([\(\)])"), "");

    std::vector<ConstraintBlock> constraints; // 可能对应多个约束
    FieldBlock field = {};
    strncpy_s(field.name, fieldName.c_str(), sizeof(field.name) - 1);
    // order 在整个传给 Table 后再计算
    if (typeStr == "INT") {
        field.type = 1; field.param = 4;
    }
    else if (typeStr == "BOOL") {
        field.type = 4; field.param = 1;
    }
    else if (typeStr == "DOUBLE") {
        field.type = 2; field.param = 2;
    }
    else if (typeStr == "VARCHAR") {
        field.type = 3;
        field.param = paramStr.empty() ? 255 : std::stoi(paramStr);
    }
    else if (typeStr == "DATETIME") {
        field.type = 5; field.param = 16;
    }
    else {
        Output::printError(outputEdit, "未知字段类型: " + QString::fromStdString(typeStr));
        return;
    }

    // 处理约束部分
    std::string rest = optionalConstraint;

    // 处理 PRIMARY KEY 约束
    if (rest.find("PRIMARY KEY") != std::string::npos) {
        ConstraintBlock cb{};
        cb.type = 1;
        strncpy_s(cb.field, fieldName.c_str(), sizeof(cb.field));
        std::string pkName = "PK_" + fieldName;
        strncpy_s(cb.name, pkName.c_str(), sizeof(cb.name));
        constraints.push_back(cb);
    }

    // 处理 FOREIGN KEY 约束
    std::regex fkSimpleRegex(R"(REFERENCES\s+(\w+)\s*\(\s*(\w+)\s*\))", std::regex::icase);
    std::smatch fkSimpleMatch;
    if (std::regex_search(rest, fkSimpleMatch, fkSimpleRegex)) {
        ConstraintBlock cb{};
        cb.type = 2;
        std::string refTable = toUpper(fkSimpleMatch[1].str());
        std::string refField = toUpper(fkSimpleMatch[2].str());
        std::string localField = toUpper(fieldName);
        std::string fkName = "FK_" + localField + "_" + refTable;
        strncpy_s(cb.name, fkName.c_str(), sizeof(cb.name));
        strncpy_s(cb.field, localField.c_str(), sizeof(cb.field));
        std::string paramStr = refTable + "(" + refField + ")";
        strncpy_s(cb.param, paramStr.c_str(), sizeof(cb.param));
        constraints.push_back(cb);
    }

    // 处理 CHECK 约束
    std::regex checkRegex(R"(CHECK\s*\(([^)]+)\))", std::regex::icase);
    std::smatch checkMatch;
    if (std::regex_search(rest, checkMatch, checkRegex)) {
        ConstraintBlock cb{};
        cb.type = 3;
        std::string expr = checkMatch[1];
        std::string checkName = "CHK_" + toUpper(fieldName);
        strncpy_s(cb.name, checkName.c_str(), sizeof(cb.name));
        strncpy_s(cb.field, fieldName.c_str(), sizeof(cb.field));
        strncpy_s(cb.param, expr.c_str(), sizeof(cb.param));
        constraints.push_back(cb);
    }

    // 处理 UNIQUE 约束
    if (rest.find("UNIQUE") != std::string::npos) {
        ConstraintBlock cb{};
        cb.type = 4;
        strncpy_s(cb.field, fieldName.c_str(), sizeof(cb.field));
        std::string uniqueName = "UNQ_" + fieldName;
        strncpy_s(cb.name, uniqueName.c_str(), sizeof(cb.name));
        constraints.push_back(cb);
    }

    // 处理 NOT NULL 约束
    if (rest.find("NOT NULL") != std::string::npos) {
        ConstraintBlock cb{};
        cb.type = 5;
        strncpy_s(cb.field, fieldName.c_str(), sizeof(cb.field));
        constraints.push_back(cb);
    }

    // 处理 DEFAULT 约束
    std::regex defaultRegex(R"(DEFAULT\s+([^\s,]+|CURRENT_TIMESTAMP))", std::regex::icase);
    std::smatch defaultMatch;
    if (std::regex_search(rest, defaultMatch, defaultRegex)) {
        ConstraintBlock cb{};
        cb.type = 6;
        strncpy_s(cb.field, fieldName.c_str(), sizeof(cb.field));
        strncpy_s(cb.param, defaultMatch[1].str().c_str(), sizeof(cb.param));
        constraints.push_back(cb);
    }

    // 处理 AUTO_INCREMENT 约束
    if (rest.find("AUTO_INCREMENT") != std::string::npos) {
        ConstraintBlock cb{};
        cb.type = 7;
        strncpy_s(cb.field, fieldName.c_str(), sizeof(cb.field));
        constraints.push_back(cb);
    }

    // 获取表并添加字段
    try {
        Table* table = dbManager::getInstance().get_current_database()->getTable(tableName);
        
		std::vector<FieldBlock> fieldsCopy = table->getFields();
        table->addField(field);

        for (const auto& constraint : constraints) {
            table->addConstraint(constraint);
        }

        table->updateRecord(fieldsCopy);
        table;
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, QString("添加字段失败: %1").arg(QString::fromStdString(e.what())));
        return;
    }

    Output::printMessage(outputEdit, QString("字段 %1 成功添加到表 %2")
        .arg(QString::fromStdString(fieldName), QString::fromStdString(tableName)));
}


//dropField还未实现
void Parse::handleDropColumn(const std::smatch& match) {
    std::string tableName = match[1];  // 获取表名
    std::string columnName = match[2];  // 获取列名
	Database* db = dbManager::getInstance().get_current_database();
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
        Table* table = dbManager::getInstance().get_current_database()->getTable(tableName);  // 获取表对象
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

void Parse::handleAddConstraint(const std::smatch& m) {
    std::string tableName = m[1];  // 表名
    std::string constraintName = m[2];  // 约束名
    std::string constraintType = m[3];  // 约束类型：PRIMARY KEY, UNIQUE, CHECK,
    std::string constraintBody = m[4];  // 约束内容：字段名、表达式等

    if (!constraintBody.empty() && constraintBody.front() == '(' && constraintBody.back() == ')') {
        constraintBody = constraintBody.substr(1, constraintBody.length() - 2);
    } //去除括号


    // 调用 Table::addConstraint 处理约束
    try {
        Table* table = dbManager::getInstance().get_current_database()->getTable(tableName);

        // 处理 PRIMARY KEY, UNIQUE, CHECK表级约束
        table->addConstraint(constraintName, constraintType, constraintBody);

        Output::printMessage(outputEdit, QString::fromStdString("ALTER TABLE 添加约束成功"));
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, QString::fromStdString(e.what()));
    }
}

void Parse::handleAddForeignKey(const std::smatch& m) {
    std::string tableName = m[1];          // 表名
    std::string constraintName = m[2];     // 约束名
    std::string foreignKeyField = m[3];    // 外键字段
    std::string referenceTable = m[4];     // 引用表
    std::string referenceField = m[5];     // 引用字段
    try {
        Table* table = dbManager::getInstance().get_current_database()->getTable(tableName);

        // 处理 ForeignKey
        table->addForeignKey(constraintName, foreignKeyField,referenceTable,referenceField);

        Output::printMessage(outputEdit, QString::fromStdString("ALTER TABLE 添加约束成功"));
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, QString::fromStdString(e.what()));
    }
}

void Parse::handleDropConstraint(const std::smatch& m) {
	std::string tableName = m[1];  // 表名
	std::string constraintName = m[2];  // 约束名
    try {
		Table* table = dbManager::getInstance().get_current_database()->getTable(tableName);
		table->dropConstraint(constraintName);  // 调用 Table 的 dropConstraint 方法
        Output::printMessage(outputEdit, QString::fromStdString("ALTER TABLE 删除约束成功"));
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, QString::fromStdString(e.what()));
    }
}



void Parse::handleCreateIndex(const std::smatch& m) {
    std::string indexName = m[1];     // 索引名
    std::string tableName = m[2];     // 表名
    std::string column1 = m[3];       // 第一个字段
    std::string column2 = m[4];       // 第二个字段（可选）

    try {
        Table* table = dbManager::getInstance().get_current_database()->getTable(tableName);

        IndexBlock index;
        strcpy_s(index.name, sizeof(index.name), indexName.c_str());

        index.field_num = (!m[4].str().empty() && m[4].matched) ? 2 : 1;

        strncpy_s(index.field[0], sizeof(index.field[0]), column1.c_str(), 2);
        index.field[0][1] = '\0'; // 手动加结尾，防止脏数据

        if (index.field_num == 2) {
            strncpy_s(index.field[1], sizeof(index.field[1]), column2.c_str(), 2);
            index.field[1][1] = '\0';
        }


        std::string recordPath = "data/" + tableName + ".trd";
        std::string indexPath = "data/" + tableName + "_" + indexName + ".idx";

        strcpy_s(index.record_file, sizeof(index.record_file), recordPath.c_str());
        strcpy_s(index.index_file, sizeof(index.index_file), indexPath.c_str());

        index.unique = false;
        index.asc = true;

        table->createIndex(index);

        Output::printMessage(outputEdit, QString::fromStdString("CREATE INDEX 创建索引成功"));
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, QString::fromStdString(e.what()));
    }
}


void Parse::handleDropIndex(const std::smatch& m) {
    try {
        // 获取表名和索引名
        std::string indexName = m[1].str();  // 索引名
        std::string tableName = m[2].str();  // 表名

        // 获取当前数据库中的表
        Table* table = dbManager::getInstance().get_current_database()->getTable(tableName);

        if (!table) throw std::runtime_error("未选择表。");

        // 确保索引名符合长度要求，截取前两个字符避免溢出
        if (indexName.size() > 2) {
            indexName = indexName.substr(0, 2);  // 保证索引名不超过2字符
        }

        // 删除索引
        table->dropIndex(indexName);

        Output::printMessage(outputEdit, "索引 '" + QString::fromStdString(indexName) + "' 删除成功！");
    }
    catch (const std::exception& e) {
        Output::printError(outputEdit, "索引删除失败: " + QString::fromStdString(e.what()));
    }
}

