#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_dbms.h"
#include <QMessageBox>


class dbms : public QMainWindow
{
    Q_OBJECT

public:
    dbms(QWidget *parent = nullptr);
    ~dbms();

private slots:
    void on_runButton_clicked();  // 运行 SQL 按钮
private:
    Ui::dbmsClass ui;
};
