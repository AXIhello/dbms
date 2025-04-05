#include <QtWidgets/QApplication>
#include"user.h"
#include"login.h"
#include"dbManager.h"
#include "mainWindow.h"
using namespace std;
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
  
    MainWindow w;
    dbManager db;
    db.createUserDatabase("test");
    user::createSysDBA();
	login loginWidget;
    loginWidget.show();
    QObject::connect(&loginWidget, &login::acceptedLogin, [&w]() {
        w.showMaximized();
        });
	//loginWidget.show();
    return a.exec();
}
