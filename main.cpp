#include <QtWidgets/QApplication>
#include"base/user.h"
#include"ui/login.h"
#include"manager/dbManager.h"
#include "ui/mainWindow.h"
#include"debug.h"
using namespace std;
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
	//debug::printDB(dbManager::basePath + "/ruanko.db");
	//debug::printTB(dbManager::basePath + "/data/1/1.tb");
	//debug::printTDF(dbManager::basePath + "/data/11/STUDENT.tdf");
    //debug::printTIC(dbManager::basePath + "/data/6/TEST.tic");

    MainWindow w;
    dbManager& db = dbManager::getInstance();
    user::createSysDBA();
	login loginWidget;
    loginWidget.show();
    QObject::connect(&loginWidget, &login::acceptedLogin, [&w]() {
        w.showMaximized();
        });
	////loginWidget.show();
    return a.exec();
}