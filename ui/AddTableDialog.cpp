#include "addtabledialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QHeaderView>
#include <QDateTime>

/**
 * 构造函数，初始化界面并设置各控件布局与信号槽连接。.
 * 
 * \param parent
 * \param dbName
 */
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

    fieldTable = new QTableWidget(0, 5, this);
    fieldTable->setHorizontalHeaderLabels({ "字段名", "类型", "长度", "完整性", "参数" });
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
/**
 * 添加一行用于编辑字段信息的控件组，并绑定相关逻辑控制。.
 * 
 */
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
    lengthSpin->setEnabled(false);  // 默认禁用
    fieldTable->setCellWidget(row, 2, lengthSpin);  // 长度参数

    QComboBox* integrityCombo = new QComboBox;
    integrityCombo->addItem("无", 0);
    integrityCombo->addItem("主键", 1);
    integrityCombo->addItem("外键", 2);
    integrityCombo->addItem("CHECK", 3);
    integrityCombo->addItem("UNIQUE", 4);
    integrityCombo->addItem("Not Null", 5);
    integrityCombo->addItem("Default", 6);
    integrityCombo->addItem("自增", 7);
    fieldTable->setCellWidget(row, 3, integrityCombo);  // 完整性

    QLineEdit* paramEdit = new QLineEdit;
    paramEdit->setPlaceholderText("额外参数");
    paramEdit->setEnabled(false);  // 默认禁用
    fieldTable->setCellWidget(row, 4, paramEdit);  // 参数

    //字段类型选择varchar时可以修改长度 别的默认禁用
    connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        [typeCombo, lengthSpin](int index) {
            QString type = typeCombo->itemText(index);
            if (type == "VARCHAR") {
                lengthSpin->setEnabled(true);
            }
            else {
                lengthSpin->setValue(0);
                lengthSpin->setEnabled(false);
            }
        });

    //完整性约束选择外键、check、default时可以继续添加额外参数 别的默认禁用
    connect(integrityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        [paramEdit, integrityCombo](int index) {
            QString text = integrityCombo->itemText(index);
            if (text == "外键" || text == "CHECK" || text == "Default") {
                paramEdit->setEnabled(true);
            }
            else {
                paramEdit->clear();
                paramEdit->setEnabled(false);
            }
        });

}
/**
 * 删除字段表中当前选中的字段行。.
 * 
 */
void AddTableDialog::removeSelectedRow() {
    int row = fieldTable->currentRow();
    if (row >= 0)
        fieldTable->removeRow(row);
}
/**
 * 返回用户输入的表名字符串。.
 * 
 * \return 
 */
QString AddTableDialog::getTableName() const {
    return tableNameEdit->text().trimmed();
}
/**
 * 将字段信息转换为标准SQL列定义字符串列表。.
 * 
 * \return 
 */
QStringList AddTableDialog::getColumnDefinitions() const {
    QStringList defs;

    for (int i = 0; i < fieldTable->rowCount(); ++i) {
        auto nameEdit = qobject_cast<QLineEdit*>(fieldTable->cellWidget(i, 0));
        auto typeBox = qobject_cast<QComboBox*>(fieldTable->cellWidget(i, 1));
        auto lengthSpin = qobject_cast<QSpinBox*>(fieldTable->cellWidget(i, 2));
        auto integrityBox = qobject_cast<QComboBox*>(fieldTable->cellWidget(i, 3));
        auto paramEdit = qobject_cast<QLineEdit*>(fieldTable->cellWidget(i, 4));

        if (!nameEdit || !typeBox || !integrityBox) continue;

        QString name = nameEdit->text().trimmed();
        QString type = typeBox->currentText();
        int length = lengthSpin->value();
        QString integrity = integrityBox->currentText();
        QString param = paramEdit ? paramEdit->text().trimmed() : "";

        if (name.isEmpty()) continue;

        QString def = name + " " + type;
        if (type == "VARCHAR" && length > 0) {
            def += "(" + QString::number(length) + ")";
        }

        if (integrity == "主键") {
            def += " PRIMARY KEY";
        }
        else if (integrity == "外键" && !param.isEmpty()) {
            def += " REFERENCES " + param;
        }
        else if (integrity == "CHECK" && !param.isEmpty()) {
            def += " CHECK (" + param + ")";
        }
        else if (integrity == "UNIQUE") {
            def += " UNIQUE";
        }
        else if (integrity == "Not Null") {
            def += " NOT NULL";
        }
        else if (integrity == "Default" && !param.isEmpty()) {
            def += " DEFAULT " + param;
        }
        else if (integrity == "自增") {
            def += " AUTOINCREMENT";
        }

        defs << def;
    }

    return defs;
}
/**
 * 返回字段表中所有字段的详细属性数据，作为QList<QVariant>列表。.
 * 
 * \return 
 */
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
