#include "AddUserDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

AddUserDialog::AddUserDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("创建用户并授权");

    auto* layout = new QVBoxLayout(this);
    auto* formLayout = new QFormLayout;

    usernameEdit = new QLineEdit;
    passwordEdit = new QLineEdit;
    passwordEdit->setEchoMode(QLineEdit::Password);
    permissionComboBox = new QComboBox;
    permissionComboBox->addItems({ "conn", "resource" });

    databaseEdit = new QLineEdit;
    tableEdit = new QLineEdit;

    formLayout->addRow("用户名：", usernameEdit);
    formLayout->addRow("密码：", passwordEdit);
    formLayout->addRow("权限类型：", permissionComboBox);
    formLayout->addRow("数据库名：", databaseEdit);
    formLayout->addRow("表名（可选）：", tableEdit);

    layout->addLayout(formLayout);

    auto* buttonLayout = new QHBoxLayout;
    okButton = new QPushButton("确定");
    cancelButton = new QPushButton("取消");

    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    layout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

QString AddUserDialog::getUsername() const {
    return usernameEdit->text();
}

QString AddUserDialog::getPassword() const {
    return passwordEdit->text();
}

QString AddUserDialog::getPermission() const {
    return permissionComboBox->currentText();
}

QString AddUserDialog::getDatabaseName() const {
    return databaseEdit->text();
}

QString AddUserDialog::getTableName() const {
    return tableEdit->text();
}
