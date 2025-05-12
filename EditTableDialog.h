#ifndef EDITTABLEDIALOG_H
#define EDITTABLEDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

class EditTableDialog : public QDialog {
    Q_OBJECT

public:
    explicit EditTableDialog(QWidget* parent = nullptr, const QString& tableName = "");

private:
    QTableWidget* table;
    QPushButton* addRowBtn;
    QPushButton* deleteRowBtn;
    QPushButton* saveBtn;
};

#endif // EDITTABLEDIALOG_H
