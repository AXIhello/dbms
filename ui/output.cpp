#include "output.h"

void Output::printSelectResult(QTextEdit* outputEdit, const std::vector<Record>& results) {
    if (!outputEdit) return;

    if (results.empty()) {
        outputEdit->append("无查询结果。");
        return;
    }

    // 打印表头
    const auto& columns = results[0].get_columns();
    QString header;
    for (const auto& col : columns) {
        header += QString::fromStdString(col) + "\t";
    }
    outputEdit->append(header);
    outputEdit->append(QString(columns.size() * 8, '-'));

    // 打印每条记录
    for (const auto& record : results) {
        const auto& values = record.get_values();
        QString row;
        for (const auto& val : values) {
            row += QString::fromStdString(val) + "\t";
        }
        outputEdit->append(row);
    }
}

void Output::printMessage(QTextEdit* outputEdit, const QString& message) {
    if (!outputEdit) return;
    outputEdit->append(message);
}

void Output::printError(QTextEdit* outputEdit, const QString& error) {
    if (!outputEdit) return;
    outputEdit->append("[错误] " + error);
}

void Output::printInfo(QTextEdit* outputEdit, const QString& message) {
    if (outputEdit) {
        outputEdit->append("[信息] :" + message);
    }
}

void Output::printDatabaseList(QTextEdit* outputEdit, const std::vector<std::string>& dbs) {
    if (!outputEdit) return;

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
