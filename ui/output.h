#ifndef OUTPUT_H
#define OUTPUT_H

#include <QTextEdit>
#include <vector>
#include "base/record/Record.h"  // 假设 Record 类定义在这个头文件里

class Output {
public:
    // 打印 SELECT 查询结果
	static void printSelectResultEmpty(QTextEdit* outputEdit,const std::vector<std::string> &cols);
    static void printSelectResult(QTextEdit* outputEdit, const std::vector<Record>& results, double duration_ms);
    static void printDatabaseList(QTextEdit* outputEdit, const std::vector<std::string>& dbs);
    static void printTableList(QTextEdit* outputEdit, const std::vector<std::string>& tables);

    // 其他通用输出方法（可选）
    static void printMessage(QTextEdit* outputEdit, const QString& message);
    static void printError(QTextEdit* outputEdit, const QString& error);
    static void printInfo(QTextEdit* outputEdit, const QString& message);

    // 设置 CLI 输出流
    static void setOstream(std::ostream* outStream);

    // CLI 输出函数
    static void printMessage(const std::string& message);
    static void printError(const std::string& error);
    static void printInfo(const std::string& message);
    static void printDatabaseList(const std::vector<std::string>& dbs);
    static void printTableList(const std::vector<std::string>& tables);
    static void printSelectResultEmpty(const std::vector<std::string>& cols);

private:
    static std::ostream* outputStream;  // CLI输出流指针（默认为nullptr）
};
   

#endif // OUTPUT_H
