#include "parse/parse.h"


QString Parse::cleanSQL(const QString& sql) {
    QString cleaned = sql.trimmed();
    // 统一换行符为空格
    cleaned.replace(QRegularExpression("[\\r\\n]+"), " ");

    // 多个空格或 Tab 变单空格
    cleaned.replace(QRegularExpression("[ \\t]+"), " ");

    ////处理逗号
    //cleaned.replace(QRegularExpression("\\s*,\\s*"), ", ");

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

std::string Parse::toUpperPreserveQuoted(const std::string& input) {
    std::string result;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;

    for (size_t i = 0; i < input.length(); ++i) {
        char ch = input[i];

        if (ch == '\'' && !inDoubleQuote) {
            inSingleQuote = !inSingleQuote;
            result += ch;
        }
        else if (ch == '"' && !inSingleQuote) {
            inDoubleQuote = !inDoubleQuote;
            result += ch;
        }
        else if (inSingleQuote || inDoubleQuote) {
            result += ch; // preserve quoted content
        }
        else {
            result += std::toupper(static_cast<unsigned char>(ch));
        }
    }

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

int Parse::getTypeFromString(const std::string& typeStr) {
    if (typeStr == "INT") return 1;
    else if (typeStr == "DOUBLE") return 2;
    else if (typeStr == "VARCHAR") return 3;
    else if (typeStr == "BOOL") return 4;
    else if (typeStr == "DATETIME") return 5;
    else return -1; // 未知类型
}