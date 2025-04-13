#include "output.h"

void Output::printSelectResult(QTextEdit* outputEdit, const std::vector<Record>& results) {
    if (!outputEdit) return;
    outputEdit->clear();

    if (results.empty()) {
        outputEdit->append("无查询结果。");
        return;
    }

    QString html = "<table border='1' cellspacing='0' cellpadding='4'>";

    // 表头
    html += "<tr>";
    const auto& columns = results[0].get_columns();
    for (const auto& col : columns) {
        html += "<th>" + QString::fromStdString(col) + "</th>";
    }
    html += "</tr>";

    // 表体
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

void Output::printDatabaseList(QTextEdit* outputEdit, const std::vector<std::string>& dbs) {
    if (!outputEdit) return;
    outputEdit->clear();

    if (dbs.empty()) {
        outputEdit->append("无数据库可用。");
        return;
    }

    outputEdit->append("数据库列表：");
    outputEdit->append("------------------");

    for (const auto& name : dbs) {
        outputEdit->append(QString::fromStdString(name));
    }

    outputEdit->append("------------------");
}
