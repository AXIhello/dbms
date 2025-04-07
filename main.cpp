#include <QtWidgets/QApplication>
#include"base/user.h"
#include"ui/login.h"
#include"manager/dbManager.h"
#include "ui/mainWindow.h"
using namespace std;
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
  
    MainWindow w;
    dbManager& db = dbManager::getInstance();
    
    user::createSysDBA();
	login loginWidget;
    loginWidget.show();
    QObject::connect(&loginWidget, &login::acceptedLogin, [&w]() {
        w.showMaximized();
        });
	//loginWidget.show();
    return a.exec();
}
