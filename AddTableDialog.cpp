#include "addtabledialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QHeaderView>
#include <QDateTime>

AddTableDialog::AddTableDialog(QWidget* parent, const QString& dbName)
    : QDialog(parent)
{
    setWindowTitle("新建数据表（所属数据库：" + dbName + "）");
    resize(600, 400);

    QLabel* nameLabel = new QLabel("表名：");
    tableNameEdit = new QLineEdit;

    QHBoxLayout* topLayout = new QHBoxLayout;
    topLayout->addWidget(nameLabel);
    topLayout->addWidget(tableNameEdit);

    fieldTable = new QTableWidget(0, 4, this);
    fieldTable->setHorizontalHeaderLabels({ "字段名", "类型", "长度", "完整性" });
    fieldTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    addFieldButton = new QPushButton("添加字段");
    removeFieldButton = new QPushButton("删除选中字段");

    connect(addFieldButton, &QPushButton::clicked, this, &AddTableDialog::addFieldRow);
    connect(removeFieldButton, &QPushButton::clicked, this, &AddTableDialog::removeSelectedRow);

    QHBoxLayout* fieldButtonLayout = new QHBoxLayout;
    fieldButtonLayout->addWidget(addFieldButton);
    fieldButtonLayout->addWidget(removeFieldButton);

    okButton = new QPushButton("确定");
    cancelButton = new QPushButton("取消");

    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    QHBoxLayout* bottomLayout = new QHBoxLayout;
    bottomLayout->addStretch();
    bottomLayout->addWidget(okButton);
    bottomLayout->addWidget(cancelButton);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(fieldTable);
    mainLayout->addLayout(fieldButtonLayout);
    mainLayout->addLayout(bottomLayout);
}

void AddTableDialog::addFieldRow() {
    int row = fieldTable->rowCount();
    fieldTable->insertRow(row);

    fieldTable->setCellWidget(row, 0, new QLineEdit);  // 字段名

    QComboBox* typeCombo = new QComboBox;
    typeCombo->addItem("INT", 1);
    typeCombo->addItem("DOUBLE", 2);
    typeCombo->addItem("VARCHAR", 3);
    typeCombo->addItem("BOOL", 4);
    typeCombo->addItem("DATETIME", 5);
    fieldTable->setCellWidget(row, 1, typeCombo);  // 类型

    QSpinBox* lengthSpin = new QSpinBox;
    lengthSpin->setRange(0, 255);
    fieldTable->setCellWidget(row, 2, lengthSpin);  // 长度参数

    QComboBox* integrityCombo = new QComboBox;
    integrityCombo->addItem("无", 0);
    integrityCombo->addItem("Not Null", 1);
    integrityCombo->addItem("Default", 2);
    fieldTable->setCellWidget(row, 3, integrityCombo);  // 完整性

    //QLabel* timeLabel = new QLabel(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    //fieldTable->setCellWidget(row, 4, timeLabel);  // 修改时间
}

void AddTableDialog::removeSelectedRow() {
    int row = fieldTable->currentRow();
    if (row >= 0)
        fieldTable->removeRow(row);
}

QString AddTableDialog::getTableName() const {
    return tableNameEdit->text().trimmed();
}

QStringList AddTableDialog::getColumnDefinitions() const {
    QStringList defs;

    for (int i = 0; i < fieldTable->rowCount(); ++i) {
        auto nameEdit = qobject_cast<QLineEdit*>(fieldTable->cellWidget(i, 0));
        auto typeBox = qobject_cast<QComboBox*>(fieldTable->cellWidget(i, 1));
        auto lengthSpin = qobject_cast<QSpinBox*>(fieldTable->cellWidget(i, 2));
        auto integrityBox = qobject_cast<QComboBox*>(fieldTable->cellWidget(i, 3));

        if (!nameEdit || !typeBox || !integrityBox) continue;

        QString name = nameEdit->text().trimmed();
        QString type = typeBox->currentText();
        int length = lengthSpin->value();
        QString integrity = integrityBox->currentText();

        if (name.isEmpty()) continue;

        QString def = name + " " + type;
        if (type == "VARCHAR" && length > 0) {
            def += "(" + QString::number(length) + ")";
        }

        if (integrity == "Not Null") {
            def += " NOT NULL";
        }
        else if (integrity == "Default") {
            if (type == "INT" || type == "DOUBLE") {
                def += " DEFAULT 0";
            }
            else if (type == "VARCHAR") {
                def += " DEFAULT ''";
            }
            else if (type == "BOOL") {
                def += " DEFAULT FALSE";
            }
            else if (type == "DATETIME") {
                def += " DEFAULT CURRENT_TIMESTAMP";
            }
        }

        defs << def;
    }

    return defs;
}

QList<QList<QVariant>> AddTableDialog::getFieldData() const {
    QList<QList<QVariant>> fields;

    for (int i = 0; i < fieldTable->rowCount(); ++i) {
        QList<QVariant> row;

        auto nameEdit = qobject_cast<QLineEdit*>(fieldTable->cellWidget(i, 0));
        auto typeBox = qobject_cast<QComboBox*>(fieldTable->cellWidget(i, 1));
        auto lengthSpin = qobject_cast<QSpinBox*>(fieldTable->cellWidget(i, 2));
        auto integrityBox = qobject_cast<QComboBox*>(fieldTable->cellWidget(i, 3));
        auto timeLabel = qobject_cast<QLabel*>(fieldTable->cellWidget(i, 4));

        row << (nameEdit ? nameEdit->text() : "")
            << (typeBox ? typeBox->currentData().toInt() : 0)
            << (lengthSpin ? lengthSpin->value() : 0)
            << (integrityBox ? integrityBox->currentData().toInt() : 0)
            << (timeLabel ? timeLabel->text() : "");

        fields.append(row);
    }

    return fields;
}
