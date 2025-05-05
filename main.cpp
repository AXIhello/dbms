#include <QtWidgets/QApplication>
#include"base/user.h"
#include"ui/login.h"
#include"manager/dbManager.h"
#include "ui/mainWindow.h"
#include"debug.h"
using namespace std;

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
    QApplication a(argc, argv);
    user::createSysDBA();
    showLogin();
    return a.exec();
}