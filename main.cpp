#include "dbms.h"
#include <QtWidgets/QApplication>
#include"user.h"
#include"login.h"
using namespace std;
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    dbms w;
    user::createSysDBA();
	login loginWidget;
    QObject::connect(&loginWidget, &login::acceptedLogin, [&w]() {
        w.show();
        });
	loginWidget.show();
    return a.exec();
}
