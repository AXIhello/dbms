#include "AddUserDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <manager/dbManager.h>
#include <QTimer>
#include <QLineEdit>
#include <QStandardItemModel>
#include <QMessageBox>


AddUserDialog::AddUserDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("创建用户并授权");
    resize(600, 400);  // 设置对话框初始宽高

    QIcon dbIcon(":/image/icons_database.png");
    QIcon tableIcon(":/image/icons_table.png");

    auto* layout = new QVBoxLayout(this);
    auto* formLayout = new QFormLayout;

    usernameEdit = new QLineEdit;
    passwordEdit = new QLineEdit;
    passwordEdit->setEchoMode(QLineEdit::Password);
    //resourceList = getAllDatabaseAndTableResources(); 

    formLayout->addRow("用户名：", usernameEdit);
    formLayout->addRow("密码：", passwordEdit);
   
    layout->addLayout(formLayout);

    // 添加“权限”文本
    QLabel* grantLabel = new QLabel("授权：", this); // 创建文本标签
    layout->addWidget(grantLabel); // 将标签添加到布局中

    // 按钮区域
    auto* buttonLayout = new QHBoxLayout;
    addGrantButton = new QPushButton("➕", this);
    removeGrantButton = new QPushButton("➖", this);
    // 设置按钮的最小大小，缩小按钮
    addGrantButton->setFixedSize(30, 30);  // 设置按钮的固定大小
    removeGrantButton->setFixedSize(30, 30);
    // 将按钮添加到水平布局并使它们居中
    buttonLayout->addWidget(addGrantButton);
    buttonLayout->addWidget(removeGrantButton);
    buttonLayout->setAlignment(Qt::AlignLeft);  // 设置按钮居中显示
    layout->addLayout(buttonLayout);  // 将按钮布局添加到主布局中
    connect(addGrantButton, &QPushButton::clicked, this, &AddUserDialog::onAddGrantButtonClicked);
    connect(removeGrantButton, &QPushButton::clicked, this, &AddUserDialog::onRemoveGrantButtonClicked);

    // --- 授权区域 ---
    grantTableWidget = new QTableWidget(this);
    grantTableWidget->setColumnCount(2);
    grantTableWidget->setHorizontalHeaderLabels(QStringList() << "资源" << "权限");
    // 设置两列平均分配宽度
    QHeaderView* header = grantTableWidget->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    grantTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    grantTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    grantTableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(grantTableWidget);
    
    //底部按钮
    auto* buttonLayout2 = new QHBoxLayout;
    okButton = new QPushButton("确定");
    cancelButton = new QPushButton("取消");

    buttonLayout2->addStretch();
    buttonLayout2->addWidget(okButton);
    buttonLayout2->addWidget(cancelButton);

    layout->addLayout(buttonLayout2);

    connect(okButton, &QPushButton::clicked, this, &AddUserDialog::onOkButtonClicked);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void AddUserDialog::onAddGrantButtonClicked()
{
    int row = grantTableWidget->rowCount();
    grantTableWidget->insertRow(row);

    // 资源下拉框
    QComboBox* resourceCombo = new QComboBox(this);
    QStandardItemModel* model = createResourceModel();
    if (model->rowCount() == 0) {
        qDebug() << "资源模型为空！";
    }
    resourceCombo->setModel(createResourceModel());
    // 设置默认显示内容
    resourceCombo->setEditable(true);
    resourceCombo->lineEdit()->setPlaceholderText("object_name");  // 初始提示
    //resourceCombo->lineEdit()->setReadOnly(true); // 防止用户直接编辑
    resourceCombo->setEditable(false); // 设置完提示后恢复不可编辑状态
    //grantTableWidget->setCellWidget(row, 0, resourceCombo);//放入表格
    
    // 权限下拉框
    QComboBox* permissionCombo = new QComboBox(this);
    permissionCombo->addItem("No grant");
    permissionCombo->addItem("connect");
    permissionCombo->addItem("connect,resource");
    //grantTableWidget->setCellWidget(row, 1, permissionCombo);
}

void AddUserDialog::onRemoveGrantButtonClicked() {
    // 删除选中的行
    int row = grantTableWidget->currentRow();
    if (row >= 0) {
        grantTableWidget->removeRow(row);
    }
}

QString AddUserDialog::getUsername() const {
    return usernameEdit->text();
}

QString AddUserDialog::getPassword() const {
    return passwordEdit->text();
}

QList<QPair<QString, QString>> AddUserDialog::getGrants() const
{
    QList<QPair<QString, QString>> grants;

    for (int i = 0; i < grantTableWidget->rowCount(); ++i) {
        auto* resourceCombo = qobject_cast<QComboBox*>(grantTableWidget->cellWidget(i, 0));
        auto* permissionCombo = qobject_cast<QComboBox*>(grantTableWidget->cellWidget(i, 1));

        if (resourceCombo && permissionCombo) {
            QString resource = resourceCombo->currentText();
            QString permission = permissionCombo->currentText();

            if (permission != "No grant"&& !resource.isEmpty()&& !permission.isEmpty()) {
                grants.append({ resource, permission });
            }
        }
    }

    return grants;
}

QStandardItemModel* AddUserDialog::createResourceModel() {
    QStandardItemModel* model = new QStandardItemModel(this);

    QIcon dbIcon(":/image/icons_database.png");
    QIcon tableIcon(":/image/icons_table.png");

    const auto& dbList = dbManager::getInstance().get_database_list_by_db();
    for (const std::string& dbName : dbList) {
        Database* db = dbManager::getInstance().get_database_by_name(dbName);
        if (!db) continue;
        db->loadTables();

        // 添加数据库项
        QStandardItem* dbItem = new QStandardItem(dbIcon, QString::fromStdString(dbName));
        model->appendRow(dbItem);

        // 添加表项
        std::vector<std::string> tableNames = db->getAllTableNames();
        for (const std::string& tableName : tableNames) {
            if (!tableName.empty()) {
                QString fullName = QString::fromStdString(dbName + "." + tableName);
                QStandardItem* tableItem = new QStandardItem(tableIcon, fullName);
                model->appendRow(tableItem);
            }
        }
    }

    return model;
}

void AddUserDialog::onOkButtonClicked()
{
    if (usernameEdit->text().isEmpty()) {
        QMessageBox::warning(this, "警告", "用户名不能为空！");
        return;
    }

    // 可以在这里做其他校验，比如权限是否为空等...
    
    done(QDialog::Accepted);  // 更加直接，不会进入死循环
}
