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
    static void printMessage_Cli(const std::string& message);
    static void printError_Cli(const std::string& error);
    static void printInfo_Cli(const std::string& message);
    static void printDatabaseList_Cli(const std::vector<std::string>& dbs);
    static void printTableList_Cli(const std::vector<std::string>& tables);
    static void printSelectResultEmpty_Cli(const std::vector<std::string>& cols);
    static void printSelectResult_Cli(const std::vector<Record>& results, double duration_ms);
    
    //当前模式
    static  int mode; 
    static std::ostream* outputStream;
 
};
   

#endif // OUTPUT_H
