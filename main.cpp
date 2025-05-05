#include <QtWidgets/QApplication>
#include"base/user.h"
#include"ui/login.h"
#include"manager/dbManager.h"
#include "ui/mainWindow.h"
#include"debug.h"
#include "parse/parse.h" 
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <iostream>
using namespace std;


std::string Utf8ToGbk(const std::string& utf8)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
    if (len == 0) return "";

    std::wstring wstr(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], len);

    len = WideCharToMultiByte(936, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    if (len == 0) return "";

    std::string gbk(len, 0);
    WideCharToMultiByte(936, 0, wstr.c_str(), -1, &gbk[0], len, NULL, NULL);

    if (!gbk.empty() && gbk.back() == '\0') gbk.pop_back();
    return gbk;
}

void RunCliMode()
{
    SetConsoleOutputCP(936);
    SetConsoleCP(936);

    dbManager::basePath ="E:\\DBMS\\DBMS_ROOT";
    std::cout << Utf8ToGbk("当前数据库根目录(basePath): ") << dbManager::basePath << std::endl;
    Parse parser;
    std::string sql;
    std::cout << Utf8ToGbk("欢迎使用 DBMS 命令行模式。输入 exit 或 quit 退出。\n");
    while (true) {
        std::cout << "dbms> ";
        if (!std::getline(std::cin, sql)) break;

        sql.erase(0, sql.find_first_not_of(" \t\r\n"));
        sql.erase(sql.find_last_not_of(" \t\r\n") + 1);

        if (sql.empty()) continue;
        if (sql == "exit" || sql == "quit") break;

        std::string result = parser.executeSQL(sql); // 假设返回UTF-8
        std::cout << Utf8ToGbk(result) << std::endl;
    }
}
int main(int argc, char *argv[])
{   
    if (argc > 1 && std::string(argv[1]) == "--cli") {
        RunCliMode();
        return 0;
    }
   
    
    QApplication a(argc, argv);
    //debug::printDB(dbManager::basePath + "/ruanko.db");
    //debug::printTB(dbManager::basePath + "/data/1/1.tb");
    //debug::printTDF(dbManager::basePath + "/data/7/TEST4.tdf");
    //debug::printTIC(dbManager::basePath + "/data/7/TEST4.tic");
    MainWindow w;
    dbManager& db = dbManager::getInstance();/*
    user::createSysDBA();
    login loginWidget;
    loginWidget.show();
    QObject::connect(&loginWidget, &login::acceptedLogin, [&w]() {*/
    w.showMaximized();
    //    });
    ////loginWidget.show();
    return a.exec();
    
  
}