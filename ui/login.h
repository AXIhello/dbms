#pragma once

#include <QDialog>
#include "ui_login.h"


class login : public QDialog
{
	Q_OBJECT

signals:
	void acceptedLogin();

public:
	login(QDialog *parent = nullptr);
	~login();

private:
	Ui::Dialog ui;

	bool authenticate(const QString& username, const QString& password); //验证


private slots:
	void on_loginBtn_clicked();
};
