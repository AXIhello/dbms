#include "output.h"
#include <QDateTime>
#include <iostream>
int Output::mode = 1; // 默认为GUI模式
std::ostream* Output::outputStream = nullptr;

// 获取当前时间戳字符串
static QString currentTimestamp() {
    return "[" + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "] ";
}
//===========================================
// GUI 模式输出函数
//===========================================

void Output::printSelectResult(QTextEdit* outputEdit, const std::vector<Record>& results, double duration_ms) {
    if (mode == 0) printSelectResult_Cli(results,duration_ms);
    if (!outputEdit) return;
   
    const auto& columns = results[0].get_columns();

    QString html;
    html += "<style>"
        "table { border-collapse: collapse; width: 100%; font-family: Consolas, monospace; }"
        "th, td { border: 1px solid #888; padding: 6px 10px; text-align: left; }"
        "th { background-color: #f0f0f0; }"
        "tr:nth-child(even) { background-color: #fafafa; }"
        "</style>";

    html += "<table>";

    // 表头
    html += "<tr>";
    for (const auto& col : columns) {
        html += "<th>" + QString::fromStdString(col) + "</th>";
    }
    html += "</tr>";

    // 数据行
    for (const auto& record : results) {
        const auto& values = record.get_values();
        html += "<tr>";
        for (const auto& val : values) {
            html += "<td>" + QString::fromStdString(val) + "</td>";
        }
        html += "</tr>";
    }

    html += "</table>";

    outputEdit->append(currentTimestamp() + "查询结果：");
    outputEdit->append(html);
    outputEdit->append("查询耗时：" + QString::number(duration_ms) + " ms");
    outputEdit->append(""); // 添加空行
}

void Output::printMessage(QTextEdit* outputEdit, const QString& message) {
    if (mode == 0) printMessage_Cli(message.toStdString());
    if (!outputEdit) return;
    outputEdit->append(currentTimestamp() + message);
    outputEdit->append(""); // 添加空行
}

void Output::printError(QTextEdit* outputEdit, const QString& error) {
    if (mode == 0) printError_Cli(error.toStdString());
    if (!outputEdit) return;
    outputEdit->append(currentTimestamp() + "<span style='color:red;'>[错误] " + error + "</span>");
    outputEdit->append(""); // 添加空行
}

void Output::printInfo(QTextEdit* outputEdit, const QString& message) {
    if (mode == 0) printInfo_Cli(message.toStdString());
    if (!outputEdit) return;
    outputEdit->append(currentTimestamp() + "<span style='color:blue;'>[信息] " + message + "</span>");
    outputEdit->append(""); // 添加空行
}

void Output::printDatabaseList(QTextEdit* outputEdit, const std::vector<std::string>& dbs) {
    if (mode == 0) printDatabaseList_Cli(dbs);
    if (!outputEdit) return;

    if (dbs.empty()) {
        outputEdit->append(currentTimestamp() + "<b>无数据库可用。</b>");
        outputEdit->append(""); // 添加空行
        return;
    }

    outputEdit->append(currentTimestamp() + "<b>数据库列表：</b>");

    QString html = "<table border='1' cellspacing='0' cellpadding='4' style='width: 100%;'>";
    html += "<tr><th style='text-align: center;'>数据库名称</th></tr>";

    for (const auto& name : dbs) {
        html += "<tr><td style='text-align: center;'>" + QString::fromStdString(name) + "</td></tr>";
    }

    html += "</table>";
    outputEdit->append(html);
    outputEdit->append(""); // 添加空行
}

void Output::printTableList(QTextEdit* outputEdit, const std::vector<std::string>& tables) {
    if (mode == 0) printTableList_Cli(tables);
    if (!outputEdit) return;

    if (tables.empty()) {
        outputEdit->append(currentTimestamp() + "<b>当前数据库没有表。</b>");
        outputEdit->append(""); // 添加空行
        return;
    }

    outputEdit->append(currentTimestamp() + "<b>当前数据库中的表：</b>");

    QString html = "<table border='1' cellspacing='0' cellpadding='4' style='width: 100%;'>";
    html += "<tr><th style='text-align: center;'>表名</th></tr>";

    for (const auto& name : tables) {
        html += "<tr><td style='text-align: center;'>" + QString::fromStdString(name) + "</td></tr>";
    }

    html += "</table>";
    outputEdit->append(html);
    outputEdit->append(""); // 添加空行
}

void Output::printSelectResultEmpty(QTextEdit* outputEdit, const std::vector<std::string>& cols) {
    if (mode == 0) printSelectResultEmpty_Cli(cols);
    if (!outputEdit) return;

    QString html;
    html += "<style>"
        "table { border-collapse: collapse; width: 100%; font-family: Consolas, monospace; }"
        "th, td { border: 1px solid #888; padding: 6px 10px; text-align: left; }"
        "th { background-color: #f0f0f0; }"
        "tr:nth-child(even) { background-color: #fafafa; }"
        "</style>";

    html += "<table><tr>";

    for (const auto& col : cols) {
        html += "<th>" + QString::fromStdString(col) + "</th>";
    }

    html += "</tr></table>";

    outputEdit->append(currentTimestamp() + "当前表中无数据：");
    outputEdit->append(html);
    outputEdit->append(""); // 添加空行
}



//===========================================
// CLI 模式输出函数
//===========================================

void Output::setOstream(std::ostream* outStream) {
    outputStream = outStream;
}

void Output::printMessage_Cli(const std::string& message) {
    if (!outputStream) return;
    *outputStream << currentTimestamp().toStdString() + message << std::endl;
}

void Output::printError_Cli(const std::string& error) {
    if (!outputStream) return;
    *outputStream << currentTimestamp().toStdString() + "[ERROR] " + error << std::endl;
}

void Output::printInfo_Cli(const std::string& message) {
    if (!outputStream) return;
    *outputStream << currentTimestamp().toStdString() + "[INFO] " + message << std::endl;
}

void Output::printDatabaseList_Cli(const std::vector<std::string>& dbs) {
    if (!outputStream) return;

    if (dbs.empty()) {
        *outputStream << currentTimestamp().toStdString() + "[INFO] 无数据库可用。" << std::endl;
        return;
    }

    *outputStream << currentTimestamp().toStdString() + "[INFO] 数据库列表：" << std::endl;
    for (const auto& name : dbs) {
        *outputStream << "  " + name << std::endl;
    }
}

void Output::printTableList_Cli(const std::vector<std::string>& tables) {
    if (!outputStream) return;

    if (tables.empty()) {
        *outputStream << currentTimestamp().toStdString() + "[INFO] 当前数据库没有表。" << std::endl;
        return;
    }

    *outputStream << currentTimestamp().toStdString() + "[INFO] 当前数据库中的表：" << std::endl;
    for (const auto& name : tables) {
        *outputStream << "  " + name << std::endl;
    }
}

void Output::printSelectResultEmpty_Cli(const std::vector<std::string>& cols) {
    if (!outputStream) return;

    *outputStream << currentTimestamp().toStdString() + "[INFO] 当前表中无数据：" << std::endl;
    std::string line = "+";
    for (const auto& col : cols) {
        line += std::string(col.length() + 2, '-') + "+";
    }
    *outputStream << line << std::endl << "| ";
    for (const auto& col : cols) {
        *outputStream << col << " | ";
    }
    *outputStream << std::endl << line << std::endl;
}

// CLI Select 输出
void Output::printSelectResult_Cli(const std::vector<Record>& results, double duration_ms) {
    if (!outputStream || results.empty()) return;

    const auto& columns = results[0].get_columns();
    const int col_count = columns.size();

    std::vector<size_t> col_widths(col_count);
    for (size_t i = 0; i < col_count; ++i) {
        col_widths[i] = columns[i].size();
    }

    // 计算每列最大宽度
    for (const auto& record : results) {
        const auto& values = record.get_values();
        for (size_t i = 0; i < values.size(); ++i) {
            col_widths[i] = std::max(col_widths[i], values[i].size());
        }
    }

    // 打印头部
    *outputStream << currentTimestamp().toStdString() + "查询结果：" << std::endl;

    std::string line = "+";
    for (const auto& width : col_widths) {
        line += std::string(width + 2, '-') + "+";
    }
    *outputStream << line << std::endl << "| ";

    for (size_t i = 0; i < col_count; ++i) {
        *outputStream << std::left << std::setw(col_widths[i] + 1) << columns[i] << "| ";
    }
    *outputStream << std::endl << line << std::endl;

    // 打印数据行
    for (const auto& record : results) {
        const auto& values = record.get_values();
        *outputStream << "| ";
        for (size_t i = 0; i < values.size(); ++i) {
            *outputStream << std::left << std::setw(col_widths[i] + 1) << values[i] << "| ";
        }
        *outputStream << std::endl;
    }

    *outputStream << line << std::endl;
    *outputStream << "查询耗时：" << duration_ms << " ms" << std::endl << std::endl;
}

