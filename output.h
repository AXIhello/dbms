// output.h
#ifndef OUTPUT_H
#define OUTPUT_H

#include <QString>
#include <QWidget>
#include <QTextEdit>
#include <vector>
#include "Record.h"

class Output {
public:
    static void setOutputTarget(QTextEdit* target);
    static void info(const QString& msg);
    static void error(const QString& msg);
    static void showTable(const std::vector<Record>& records);
    void displayResult(const QString& result, QTextEdit* outputEdit);

private:
    static QTextEdit* outputTarget;
};

#endif // OUTPUT_H
