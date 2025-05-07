#include "AddDatabaseDialog.h"
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

AddDatabaseDialog::AddDatabaseDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("添加数据库");

    label = new QLabel("数据库名：", this);
    lineEditDatabaseName = new QLineEdit(this);
    lineEditDatabaseName->setObjectName("lineEditDatabaseName");

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QHBoxLayout* inputLayout = new QHBoxLayout;
    inputLayout->addWidget(label);
    inputLayout->addWidget(lineEditDatabaseName);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(inputLayout);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
    setModal(true);
}

QString AddDatabaseDialog::getDatabaseName() const
{
    return lineEditDatabaseName->text().trimmed();
}
