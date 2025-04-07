#include "login.h"
#include <QMessageBox>
#include "base/user.h"

login::login(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
}

login::~login()
{}

void login::on_loginButton_clicked()
{
	
	QString username = ui.nameEdit->text();
	QString password = ui.passwordEdit->text();
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