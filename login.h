#pragma once

#include <QWidget>
#include "ui_login.h"

class login : public QWidget
{
	Q_OBJECT

signals:
	void acceptedLogin();

public:
	login(QWidget *parent = nullptr);
	~login();

private:
	Ui::loginClass ui;

	bool authenticate(const QString& username, const QString& password); //验证


private slots:
	void on_loginButton_clicked();
};
