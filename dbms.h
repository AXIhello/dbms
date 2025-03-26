#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_dbms.h"


class dbms : public QMainWindow
{
    Q_OBJECT

public:
    dbms(QWidget *parent = nullptr);
    ~dbms();

private:
    Ui::dbmsClass ui;
};
