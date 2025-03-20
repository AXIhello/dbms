#include "dbms.h"
#include <QtWidgets/QApplication>
#include"user.h"
using namespace std;
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    dbms w;
    w.show();
    user::createSysDBA();
    return a.exec();
}
