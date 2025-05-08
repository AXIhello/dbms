#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>

class AddTableDialog : public QDialog {
    Q_OBJECT

public:
    explicit AddTableDialog(QWidget* parent = nullptr, const QString& dbName = "");

    QString getTableName() const;
    QList<QList<QVariant>> getFieldData() const;
    QStringList getColumnDefinitions() const; 


private slots:
    void addFieldRow();
    void removeSelectedRow();

private:
    QLineEdit* tableNameEdit;
    QTableWidget* fieldTable;
    QPushButton* addFieldButton;
    QPushButton* removeFieldButton;
    QPushButton* okButton;
    QPushButton* cancelButton;
};
