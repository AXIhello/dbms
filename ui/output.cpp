#include "output.h"
#include <QDateTime>
#include <iostream>

// 获取当前时间戳字符串
static QString currentTimestamp() {
    return "[" + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "] ";
}

void Output::printSelectResult(QTextEdit* outputEdit, const std::vector<Record>& results, double duration_ms) {
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
    if (!outputEdit) return;
    outputEdit->append(currentTimestamp() + message);
    outputEdit->append(""); // 添加空行

}

void Output::printError(QTextEdit* outputEdit, const QString& error) {
    if (!outputEdit) return;
    outputEdit->append(currentTimestamp() + "<span style='color:red;'>[错误] " + error + "</span>");
    outputEdit->append(""); // 添加空行

}

void Output::printInfo(QTextEdit* outputEdit, const QString& message) {
    if (!outputEdit) return;
    outputEdit->append(currentTimestamp() + "<span style='color:blue;'>[信息] " + message + "</span>");
    outputEdit->append(""); // 添加空行

}

void Output::printDatabaseList(QTextEdit* outputEdit, const std::vector<std::string>& dbs) {
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
