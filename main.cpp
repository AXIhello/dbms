#include "dbms.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    dbms w;
    w.show();
    return a.exec();
}
