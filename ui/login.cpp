#include "login.h"
#include <QMessageBox>
#include "base/user.h"

login::login(QDialog *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
}

login::~login()
{}

void login::on_loginBtn_clicked()
{
	
	QString username = ui.usrLineEdit->text();
	QString password = ui.pwdLineEdit->text();
	if (authenticate(username, password))
	{
		emit acceptedLogin();
		this->close();
	}
	else
	{
		QMessageBox::warning(this, "错误", "用户名或密码错误");
	}
	
}

bool login::authenticate(const QString& username, const QString& password)
{
	if (username.toStdString() == "sysdba")
	{
		return true;
	}

	auto users = user::loadUsers();
	for (const auto& u : users)
	{
		if (u.username == username.toStdString() && u.password == password.toStdString())
		{
			user::setCurrentUser(u);
			return true;
		}
	}
	return false;
}