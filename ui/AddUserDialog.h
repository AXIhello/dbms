#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QFormLayout>
#include "AddTableDialog.h"
#include <QHeaderView>
#include <QStandardItemModel>


class AddUserDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddUserDialog(QWidget* parent = nullptr);
    QString getUsername() const;
    QString getPassword() const;
    void onAddGrantButtonClicked();
    void onRemoveGrantButtonClicked();
    QList<QPair<QString, QString>> getGrants() const;
    QStandardItemModel* createResourceModel();




private:
    QLineEdit* usernameEdit;
    QLineEdit* passwordEdit;
    QTableWidget* grantTableWidget;
    QPushButton* addGrantButton;
    QPushButton* removeGrantButton;
    QPushButton* okButton;
    QPushButton* cancelButton;
    QStringList permissionList;
    QStringList resourceList;
};
