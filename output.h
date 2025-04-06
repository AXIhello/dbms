#ifndef OUTPUT_H
#define OUTPUT_H

#include <QTextEdit>
#include <vector>
#include "record.h"  // 假设 Record 类定义在这个头文件里

class Output {
public:
    // 打印 SELECT 查询结果
    static void printSelectResult(QTextEdit* outputEdit, const std::vector<Record>& results);

    // 其他通用输出方法（可选）
    static void printMessage(QTextEdit* outputEdit, const QString& message);
    static void printError(QTextEdit* outputEdit, const QString& error);
};

#endif // OUTPUT_H
