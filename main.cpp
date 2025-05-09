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

static void RunCliMode()
{
    // 设置控制台编码为UTF-8，确保中文显示正常
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // 明确设置CLI模式
    Output::mode = 0;  // 设置为CLI模式
    Output::setOstream(&std::cout);  // 设置输出流为标准输出

    std::cout << "当前数据库根目录(basePath): " << dbManager::basePath << std::endl;
    Parse parser;
    std::string sql;
    std::cout << "欢迎使用 DBMS 命令行模式。输入 exit 或 quit 退出。\n" << std::endl;

    while (true) {
        std::cout << "dbms> ";
        if (!std::getline(std::cin, sql)) break;

        // 去除前后空白
        sql.erase(0, sql.find_first_not_of(" \t\r\n"));
        if (sql.empty()) continue;
        sql.erase(sql.find_last_not_of(" \t\r\n") + 1);

        // 检查退出命令
        if (sql == "exit" || sql == "quit") break;

        // 执行SQL
        parser.executeSQL(sql);
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
    debug::printTIC("D:\\dbms\\DBMS_ROOT\\data\\6\\TEST8.tic");
    // 检查启动参数，判断是否为CLI模式
    if (argc > 1 && strcmp(argv[1], "--cli") == 0) {
        Output::mode = 0;  // 设置为CLI模式
        RunCliMode();
        return 0;  // 退出
    }
    // GUI模式
    Output::mode = 1;  // 设置为GUI模式

    QApplication a(argc, argv);
    user::createSysDBA();
    showLogin();
    return a.exec();
  
}