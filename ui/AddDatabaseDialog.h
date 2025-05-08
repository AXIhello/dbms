#ifndef ADDDATABASEDIALOG_H
#define ADDDATABASEDIALOG_H

#include <QDialog>

class QLabel;
class QLineEdit;
class QDialogButtonBox;

class AddDatabaseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddDatabaseDialog(QWidget* parent = nullptr);
    QString getDatabaseName() const;

private:
    QLabel* label;
    QLineEdit* lineEditDatabaseName;
    QDialogButtonBox* buttonBox;
};

#endif // ADDDATABASEDIALOG_H
