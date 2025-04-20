#include "manager/parse.h"


QString Parse::cleanSQL(const QString& sql) {
    QString cleaned = sql.trimmed();
    // 统一换行符为空格
    cleaned.replace(QRegularExpression("[\\r\\n]+"), " ");

    // 多个空格或 Tab 变单空格
    cleaned.replace(QRegularExpression("[ \\t]+"), " ");
    return cleaned.trimmed();
}


std::string Parse::toUpper(const std::string& str) {
    std::string upperStr = str;
    std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), ::toupper);
    return upperStr;
}

std::string Parse::toLower(const std::string& input) {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string Parse::trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    auto end = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::string current;
    int bracketCount = 0; // 用来追踪括号层数

    for (char ch : str) {
        if (ch == '(') {
            bracketCount++; // 遇到左括号，进入括号内
        }
        if (ch == ')') {
            bracketCount--; // 遇到右括号，退出括号内
        }

        if (ch == delimiter && bracketCount == 0) {
            // 只有当括号层数为0时，才分割
            result.push_back(current);
            current.clear();
        }
        else {
            current.push_back(ch);
        }
    }

    // 将最后一个部分添加到结果中
    if (!current.empty()) {
        result.push_back(current);
    }

    return result;
}

std::vector<std::string> Parse::splitDefinition(const std::string& input) {
    std::vector<std::string> tokens;
    std::string current;
    int parenCount = 0;

    for (char c : input) {
        if (c == ',' && parenCount == 0) {
            tokens.push_back(current);
            current.clear();
        }
        else {
            current += c;
            if (c == '(') parenCount++;
            if (c == ')') parenCount--;
        }
    }
    if (!current.empty()) {
        tokens.push_back(current); // 最后一部分
    }

    return tokens;
}

std::vector<std::string> Parse::split(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::string current;
    int bracketCount = 0; // 用来追踪括号层数

    for (char ch : str) {
        if (ch == '(') {
            bracketCount++; // 遇到左括号，进入括号内
        }
        if (ch == ')') {
            bracketCount--; // 遇到右括号，退出括号内
        }

        if (ch == delimiter && bracketCount == 0) {
            // 只有当括号层数为0时，才分割
            result.push_back(current);
            current.clear();
        }
        else {
            current.push_back(ch);
        }
    }

    // 将最后一个部分添加到结果中
    if (!current.empty()) {
        result.push_back(current);
    }

    return result;
}

void Parse::parseFieldAndConstraints(const std::string& def, std::vector<FieldBlock>& fields, std::vector<ConstraintBlock>& constraints, int& fieldIndex) {
    std::string upperDef = toUpper(def);  // 转换为大写

    // === 字段定义解析 ===
    std::regex fieldRegex(R"(^\s*(\w+)\s+([a-zA-Z]+)(\s*\(\s*(\d+)\s*\))?)", std::regex::icase);
    std::smatch fieldMatch;
    if (!std::regex_search(upperDef, fieldMatch, fieldRegex)) {
		throw std::runtime_error("字段定义解析失败: " + def);
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
		throw runtime_error("不支持的字段类型: " + typeStr);
    }

    // === 完整性约束解析 ===
    std::string rest = toUpper(def.substr(fieldMatch[0].length()));

    // 检查 PRIMARY KEY 约束
    if (rest.find("PRIMARY KEY") != std::string::npos) {
        ConstraintBlock cb{};
        cb.type = 1; // PRIMARY KEY 类型
        strncpy_s(cb.field, name.c_str(), sizeof(cb.field));
        std::string pkName = "PK_" + name;
        strncpy_s(cb.name, pkName.c_str(), sizeof(cb.name));
        constraints.push_back(cb);
    }

    // 检查 FOREIGN KEY 约束
    std::regex fkSimpleRegex(R"(REFERENCES\s+(\w+)\s*\(\s*(\w+)\s*\))", std::regex::icase);
    std::smatch fkSimpleMatch;
    if (std::regex_search(rest, fkSimpleMatch, fkSimpleRegex)) {
        ConstraintBlock cb{};
        cb.type = 2; // FOREIGN KEY 类型
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

    // 检查 CHECK 约束
    std::regex checkRegex(R"(CHECK\s*\(([^)]+)\))", std::regex::icase);
    std::smatch checkMatch;
    if (std::regex_search(rest, checkMatch, checkRegex)) {
        ConstraintBlock cb{};
        cb.type = 3; // CHECK 类型
        std::string expr = checkMatch[1];
        std::string checkName = "CHK_" + toUpper(name);
        strncpy_s(cb.name, checkName.c_str(), sizeof(cb.name));
        strncpy_s(cb.field, name.c_str(), sizeof(cb.field));
        strncpy_s(cb.param, expr.c_str(), sizeof(cb.param));
        constraints.push_back(cb);
    }

    // 检查 UNIQUE 约束
    if (rest.find("UNIQUE") != std::string::npos) {
        ConstraintBlock cb{};
        cb.type = 4; // UNIQUE 类型
        strncpy_s(cb.field, name.c_str(), sizeof(cb.field));
        std::string uniqueName = "UQ_" + name;
        strncpy_s(cb.name, uniqueName.c_str(), sizeof(cb.name));
        constraints.push_back(cb);
    }

    // 检查 NOT NULL 约束
    if (rest.find("NOT NULL") != std::string::npos) {
        ConstraintBlock cb{};
        cb.type = 5; // NOT NULL 类型
        strncpy_s(cb.field, name.c_str(), sizeof(cb.field));
        constraints.push_back(cb);
    }

    // 检查 DEFAULT 约束
    std::regex defaultRegex(R"(DEFAULT\s+([^\s,]+|CURRENT_TIMESTAMP))", std::regex::icase);
    std::smatch defaultMatch;
    if (std::regex_search(rest, defaultMatch, defaultRegex)) {
        ConstraintBlock cb{};
        cb.type = 6; // DEFAULT 类型
        strncpy_s(cb.field, name.c_str(), sizeof(cb.field));
        strncpy_s(cb.param, defaultMatch[1].str().c_str(), sizeof(cb.param));
        constraints.push_back(cb);
    }

    // 检查 AUTO_INCREMENT 约束
    if (rest.find("AUTO_INCREMENT") != std::string::npos) {
        ConstraintBlock cb{};
        cb.type = 7; // AUTO_INCREMENT 类型
        strncpy_s(cb.field, name.c_str(), sizeof(cb.field));
        constraints.push_back(cb);
    }

    //不支持的约束
    if (rest.find("CHECK") != std::string::npos && rest.find("CHECK") == std::string::npos) {
        throw std::runtime_error("不支持的约束类型或语法错误");
    }

    fields.push_back(field); // 添加字段到字段列表
}
