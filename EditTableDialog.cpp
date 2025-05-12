#include "editTableDialog.h"
#include <QHeaderView>
#include <QMessageBox>

EditTableDialog::EditTableDialog(QWidget* parent, const QString& tableName)
    : QDialog(parent)
{
    setWindowTitle("修改表：" + tableName);
    resize(800, 400);

    table = new QTableWidget(this);
    table->setColumnCount(5);  // 假设5列，你可以替换成实际列数
    table->setHorizontalHeaderLabels({ "字段1", "字段2", "字段3", "字段4", "字段5" });
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setEditTriggers(QAbstractItemView::AllEditTriggers);  // 所有格子可编辑

    addRowBtn = new QPushButton("新增行");
    deleteRowBtn = new QPushButton("删除选中行");
    saveBtn = new QPushButton("保存修改");

    connect(addRowBtn, &QPushButton::clicked, this, [=]() {
        int row = table->rowCount();
        table->insertRow(row);
        });

    connect(deleteRowBtn, &QPushButton::clicked, this, [=]() {
        int row = table->currentRow();
        if (row >= 0)
            table->removeRow(row);
        });

    connect(saveBtn, &QPushButton::clicked, this, [=]() {
        QMessageBox::information(this, "保存", "保存成功！（这里你可以连接实际保存逻辑）");
        accept();  // 或者不关闭窗口你自己决定
        });

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(addRowBtn);
    buttonLayout->addWidget(deleteRowBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(saveBtn);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(table);
    mainLayout->addLayout(buttonLayout);
}
