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


static std::string Utf8ToGbk(const std::string& utf8)
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

static void RunCliMode()
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

void showLogin();

static void showMainWindow() {
    MainWindow* w = new MainWindow;
    w->showMaximized();

    // 连接“切换用户”信号
    QObject::connect(w, &MainWindow::requestSwitchUser, [=]() {
        w->close();  // 会触发 deleteLater
        showLogin(); // 重新显示登录界面
        });
}

void showLogin() {
    login* loginWidget = new login;
    loginWidget->show();

    QObject::connect(loginWidget, &login::acceptedLogin, [=]() {
        loginWidget->close();
        showMainWindow();
        });
}
int main(int argc, char *argv[])
{   
    if (argc > 1 && std::string(argv[1]) == "--cli") {
        RunCliMode();
        return 0;
    }
    QApplication a(argc, argv);
    user::createSysDBA();
    showLogin();
    return a.exec();
  
}