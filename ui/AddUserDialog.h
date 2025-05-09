#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QFormLayout>

class AddUserDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddUserDialog(QWidget* parent = nullptr);
    QString getUsername() const;
    QString getPassword() const;
    QString getPermission() const;
    QString getDatabaseName() const;
    QString getTableName() const;

private:
    QLineEdit* usernameEdit;
    QLineEdit* passwordEdit;
    QComboBox* permissionComboBox;
    QLineEdit* databaseEdit;
    QLineEdit* tableEdit;
    QPushButton* okButton;
    QPushButton* cancelButton;
};
