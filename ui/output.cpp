#include "output.h"

void Output::printSelectResult(QTextEdit* outputEdit, const std::vector<Record>& results) {
    if (!outputEdit) return;
    outputEdit->clear();

    if (results.empty()) {
        outputEdit->append("<p style='color:gray;'>无查询结果。</p>");
        return;
    }

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

    outputEdit->append(html);
}


void Output::printMessage(QTextEdit* outputEdit, const QString& message) {
    if (!outputEdit) return;
    outputEdit->clear();
    outputEdit->append(message);
}

void Output::printError(QTextEdit* outputEdit, const QString& error) {
    if (!outputEdit) return;
    outputEdit->clear();
    outputEdit->append("[错误] " + error);
}

void Output::printInfo(QTextEdit* outputEdit, const QString& message) {
    if (outputEdit) {
        outputEdit->clear();
        outputEdit->append("[信息] :" + message);
    }
}

void Output::printDatabaseList(QTextEdit* outputEdit, const std::vector<std::string>& dbs) {
    if (!outputEdit) return;
    outputEdit->clear();

    // 如果没有数据库
    if (dbs.empty()) {
        outputEdit->append("<b>无数据库可用。</b>");
        return;
    }

    // 添加标题
    outputEdit->append("<b>数据库列表</b>");

    // 使用HTML表格展示数据库列表
    QString html = "<table border='1' cellspacing='0' cellpadding='4' style='width: 100%;'>";
    html += "<tr><th style='text-align: center;'>数据库名称</th></tr>";  // 表头居中

    // 表体，数据库名称居中
    for (const auto& name : dbs) {
        html += "<tr><td style='text-align: center;'>" + QString::fromStdString(name) + "</td></tr>";  // 数据库名称居中
    }

    html += "</table>";
    outputEdit->append(html);
}

void Output::printTableList(QTextEdit* outputEdit, const std::vector<std::string>& tables) {
    if (!outputEdit) return;
    outputEdit->clear();

    // 如果没有表
    if (tables.empty()) {
        outputEdit->append("<b>当前数据库没有表。</b>");
        return;
    }

    // 添加标题
    outputEdit->append("<b>当前数据库中的表：</b>");

    // 使用HTML表格展示表名列表
    QString html = "<table border='1' cellspacing='0' cellpadding='4' style='width: 100%;'>";
    html += "<tr><th style='text-align: center;'>表名</th></tr>";  // 表头居中

    // 表体，表名居中
    for (const auto& name : tables) {
        html += "<tr><td style='text-align: center;'>" + QString::fromStdString(name) + "</td></tr>";
    }

    html += "</table>";
    outputEdit->append(html);
}


