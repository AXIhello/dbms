#include "mainwindow.h"
#include "ui_mainwindow.h"  // 包含 UI 头文件
#include <QSplitter>
#include <QBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QMessageBox>
#include"ui/output.h"
#include "parse/parse.h" 
#include "manager/dbManager.h"
#include "AddDatabaseDialog.h"
#include "AddTableDialog.h"
#include <QGroupBox>
#include "AddUserDialog.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)// 初始化 UI
{
    //qDebug() << "GUI模式下的basePath:" << QString::fromStdString(dbManager::basePath);
    ui->setupUi(this);  // 让 UI 组件和窗口关联
    // 设置样式表
    QString styleSheet = R"(
        /* 设置整个应用程序的字体 */
        * {
            font-size: 13pt;
            font-family: "Arial";
        }

        /* 设置菜单栏的字体 */
        QMenuBar {
            font-size: 11pt;
            font-weight: bold;
            background-color: lightgray;
        }

        /* 设置菜单项的字体 */
        QMenu {
            font-size: 12pt;
        }

        /* 设置按钮的字体 */
        QPushButton {
            font-size: 12pt;
            font-weight: bold;
        }

        /* 设置文本框的字体 */
        QTextEdit {
            font-size: 14pt;
        }

        /* 设置状态栏的字体 */
        QStatusBar {
            font-size: 12pt;
        }
    )";
    ui->inputEdit->setInputMethodHints(Qt::ImhPreferLatin); // 设置为英文输入
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);//为QTreeWidget 设置右键菜单策略
    this->setStyleSheet(styleSheet);

    // 连接按钮点击事件到槽函数
    connect(ui->runButton, &QPushButton::clicked, this, &MainWindow::onRunButtonClicked);
    connect(ui->treeWidget, &QTreeWidget::itemClicked, this, &MainWindow::onTreeItemClicked);
    connect(ui->treeWidget, &QTreeWidget::customContextMenuRequested,
        this, &MainWindow::onTreeWidgetContextMenu);
    setWindowTitle("DBMS 工作台"); // 设置窗口标题
    setGeometry(100, 100, 1000, 600);  // 设置窗口大小

    // 设置菜单栏
    //menuBar()->setStyleSheet("QMenuBar { background-color: lightgray; }");
    QAction* switchUserAction = new QAction("切换用户", this);
    menuBar()->addAction(switchUserAction);
    connect(switchUserAction, &QAction::triggered, this, &MainWindow::onSwitchUser);
    ui->outputEdit->setReadOnly(true);

    // 设置按钮的宽度
    ui->runButton->setMinimumWidth(91);  // 设置按钮最小宽度
    ui->runButton->setMaximumWidth(91);  // 设置按钮最大宽度
    ui->cleanButton->setMinimumWidth(91);  // 设置按钮最小宽度
    ui->cleanButton->setMaximumWidth(91);  // 设置按钮最大宽度
    //设置按钮高度
    ui->runButton->setFixedHeight(31);
    ui->cleanButton->setFixedHeight(31);

    // 创建一个单独的QWidget来包裹runButton
    buttonWidget = new QWidget(this);  // 把 buttonWidget 声明为成员变量
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonWidget);
    // 添加弹性空间，使按钮居中对称排列
    buttonLayout->addStretch();
    buttonLayout->addWidget(ui->runButton);
    buttonLayout->addSpacing(150);                // 中间间距（可调整）
    buttonLayout->addWidget(ui->cleanButton);
    buttonLayout->addStretch();                  // 右侧空白
    buttonWidget->setLayout(buttonLayout);
    buttonWidget->setFixedHeight(50);
    //连接cleanButton槽函数
    connect(ui->cleanButton, &QPushButton::clicked, this, [=]() {
        ui->inputEdit->setPlainText("SQL>> ");
        ui->outputEdit->clear();
        });


    ui->treeWidget->setHeaderHidden(true); // 隐藏表头
    ui->treeWidget->setMinimumWidth(200);  // 设置宽度


    // 输入框和输出框使用垂直分割
    QSplitter* ioSplitter = new QSplitter(Qt::Vertical);
    ioSplitter->addWidget(ui->inputEdit);  // 输入框
    ioSplitter->addWidget(buttonWidget);   // 按钮放在单独的布局里
    ioSplitter->addWidget(ui->outputEdit); // 输出框
    ioSplitter->setStretchFactor(0, 5);  // 输入框占较多空间
    ioSplitter->setStretchFactor(1, 1);  // 按钮占较少空间
    ioSplitter->setStretchFactor(2, 3);  // 输出框占较少空间

    // ===== 创建左侧：数据库资源管理器 =====
    QGroupBox* treeGroupBox = new QGroupBox("数据库资源管理器");
    QVBoxLayout* treeLayout = new QVBoxLayout(treeGroupBox);
    // 添加刷新按钮
    QPushButton* refreshButton = new QPushButton("🔄 刷新", this);
    refreshButton->setFixedHeight(28);
    refreshButton->setStyleSheet("font-size: 11pt; font-weight: bold;");
    treeLayout->addWidget(refreshButton);  // 放在 treeWidget 上方
    // 添加资源树
    treeLayout->addWidget(ui->treeWidget);
    treeGroupBox->setMinimumWidth(200);
    treeGroupBox->setStyleSheet("QGroupBox { font-weight: bold; font-size: 14pt; }");
    // 绑定刷新按钮槽函数
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::refreshTree);

    // ===== 创建右侧：控制台区域 =====
    QGroupBox* consoleGroupBox = new QGroupBox;
    QVBoxLayout* consoleLayout = new QVBoxLayout(consoleGroupBox);
    consoleLayout->addWidget(ioSplitter);  // ioSplitter 里包含输入框、按钮、输出框
    consoleGroupBox->setStyleSheet("QGroupBox { font-weight: bold; font-size: 14pt; }");

    // ===== 主内容区水平分割器 =====
    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->addWidget(treeGroupBox);
    mainSplitter->addWidget(consoleGroupBox);
    mainSplitter->setStretchFactor(0, 1); // 左边占小
    mainSplitter->setStretchFactor(1, 4); // 右边占大



    // 主窗口部件
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addWidget(mainSplitter);
    setCentralWidget(centralWidget);

    ui->inputEdit->setPlainText("SQL>> ");

    refreshTree(); 
   }

MainWindow::~MainWindow() {
    delete ui;  // 释放 UI 资源
    dbManager::getInstance().clearCache();
}

void MainWindow::onSwitchUser() {
    emit requestSwitchUser(); // 自定义信号
    close(); // 关闭主窗口
}


void MainWindow::onRunButtonClicked() {
    QString fullText = ui->inputEdit->toPlainText().trimmed(); // 获取全部语句

    // 从最后一个 "SQL>>" 开始截取
    int lastPromptIndex = fullText.lastIndexOf("SQL>>");
    if (lastPromptIndex == -1) {
        QMessageBox::warning(this, "警告", "找不到 SQL>> 提示符，请先输入命令！");
        return;
    }

    // 获取 SQL>> 后面的文本内容
    QString sql = fullText.mid(lastPromptIndex + 6).trimmed(); // +6 是跳过 "SQL>>"

    if (sql.isEmpty()) {
        QMessageBox::warning(this, "警告", "SQL 语句不能为空！");
        return;
    }

    //if (sql.isEmpty()) {
      //  QMessageBox::warning(this, "警告", "SQL 语句不能为空！");
      //  return;
    //}

    // 分割多个 SQL 语句，以分号为分隔符
    QStringList sqlStatements = sql.split(";", Qt::SkipEmptyParts); // 按分号分割，跳过空部分

    if (sqlStatements.isEmpty()) {
        QMessageBox::warning(this, "警告", "没有有效的 SQL 语句！");
        return;
    }

    Parse parser(ui->outputEdit, this);

    // 对每条 SQL 语句进行处理
    for (QString statement : sqlStatements) {  // 注意，这里不用 const QString&，改成 QString，方便后面改内容
        QString trimmedSql = statement.trimmed(); // 去除前后空格

        if (!trimmedSql.isEmpty()) {
            if (!trimmedSql.endsWith(';')) {  // 如果没以分号结尾
                trimmedSql += ";";             // 手动加上分号
            }
            parser.execute(trimmedSql);  // 执行每条 SQL 语句
        }
    }
    QString currentText = ui->inputEdit->toPlainText();
    if (!currentText.endsWith("\n") && !currentText.isEmpty())
        currentText += "\n";
    currentText += "\nSQL>>\x20" ;  // 自动加提示符
    ui->inputEdit->setPlainText(currentText);
}


void MainWindow::refreshTree() {
    try
    {
        ui->treeWidget->clear(); // 清空旧数据

        // 图标
        QIcon dbIcon(":/image/icons_database.png");
        QIcon tableIcon(":/image/icons_table.png");

        //获取所有数据库和库名
        const auto& dbList = dbManager::getInstance().get_database_list_by_db();
        for (const std::string& dbName : dbList) {
            // 加载数据库对象
            //BUG；此时获取到的db不是最新的；createTable后尚未添加进去？
            Database* db = dbManager::getInstance().get_database_by_name(dbName);
            if (!db) continue;
            db->loadTables();

            // 顶层节点：数据库
            QTreeWidgetItem* dbItem = new QTreeWidgetItem(ui->treeWidget);
            dbItem->setText(0, QString::fromStdString(dbName));
            dbItem->setIcon(0, dbIcon);

            // 添加表作为子节点
            std::vector<std::string> tableNames = db->getAllTableNames();
            for (const std::string& tableName : tableNames) {
                if (!tableName.empty()) {
                    QTreeWidgetItem* tableItem = new QTreeWidgetItem();
                    tableItem->setText(0, QString::fromStdString(tableName));
                    tableItem->setIcon(0, tableIcon);
                    dbItem->addChild(tableItem);
                }
            }
        }

        ui->treeWidget->expandAll(); // 展开全部
    }
    catch (const std::runtime_error& e) {
        Output::printError(ui->outputEdit, QString("运行时错误: ") + e.what());
        // qDebug() << "运行时错误:" << e.what();
    }
    catch (const std::exception& e) {
        Output::printError(ui->outputEdit, QString("其他异常: ") + e.what());
        //qDebug() << "其他异常:" << e.what();
    }
}

void MainWindow::onTreeItemClicked(QTreeWidgetItem* item, int column) {
    //如果用户还没点“运行”就点击了数据库或表节点，那就自动把最后一条 SQL>> 之后的 SQL 提交执行，再执行点击触发的系统 SQL
    QString fullText = ui->inputEdit->toPlainText();
    int lastPromptIndex = fullText.lastIndexOf("SQL>>");

    if (lastPromptIndex != -1) {
        QString userInput = fullText.mid(lastPromptIndex + 6).trimmed(); // 6 是 "SQL>>" + 空格的长度
        if (!userInput.isEmpty()) {
            QStringList sqlStatements = userInput.split(";", Qt::SkipEmptyParts);
            Parse parser(ui->outputEdit, this);
            for (QString statement : sqlStatements) {
                QString trimmed = statement.trimmed();
                if (!trimmed.isEmpty()) {
                    if (!trimmed.endsWith(";"))
                        trimmed += ";";
                    parser.execute(trimmed);
                }
            }
            // 自动追加新的提示符
            fullText = fullText.trimmed() + "\nSQL>> ";
            ui->inputEdit->setPlainText(fullText);
        }
    }

    QTreeWidgetItem* parent = item->parent();

    if (!parent) {
        // === 点击数据库节点 ===
        QString dbName = item->text(0);
        
        // 显示 SQL 并执行
        QString sql = "USE DATABASE " + dbName + ";\n\n";
        QString currentText = ui->inputEdit->toPlainText();
        ui->inputEdit->setPlainText(currentText + sql +"SQL>> ");
        Parse parser(ui->outputEdit, this);
        parser.execute(sql);
        return;
    }

    // === 点击表节点 ===
    QString dbName = parent->text(0);
    QString tableName = item->text(0);
    
    QString useDb = "USE DATABASE " + dbName + ";    ";// 自动设置数据库
    QString sql = "SELECT * FROM " + tableName + ";\n\n"; 
    QString currentText = ui->inputEdit->toPlainText();
    ui->inputEdit->setPlainText(currentText  + useDb + sql + "SQL>> ");
    
        try {
            Parse parser(ui->outputEdit, this);
            parser.execute(useDb);
            parser.execute(sql);
        }
        catch (const std::exception& e) {
            Output::printError(ui->outputEdit, QString("加载表失败: ") + e.what());
        }
    }

/**
 * 在TreeWidget右键选择数据库、表、用户的添加删除、修改操作.
 * 
 * \param pos
 */
void MainWindow::onTreeWidgetContextMenu(const QPoint& pos) {
    QTreeWidgetItem* item = ui->treeWidget->itemAt(pos);
    QMenu menu(this);

    if (!item) {
        // === 空白区域右键 ===

        menu.addAction("添加数据库", [=]() {
            AddDatabaseDialog dlg(this);
            if (dlg.exec() == QDialog::Accepted) {
                QString dbName = dlg.getDatabaseName();
                if (!dbName.isEmpty()) {
                    QString sql = "CREATE DATABASE " + dbName + ";\n\n";
                    QString currentText = ui->inputEdit->toPlainText();
                    ui->inputEdit->setPlainText(currentText + sql + "SQL>> ");

                    try {
                        Parse parser(ui->outputEdit, this);
                        parser.execute(sql);
                    }
                    catch (const std::exception& e) {
                        Output::printError(ui->outputEdit, QString("创建数据库失败: ") + e.what());
                    }
                }
            }
            });

        if (std::string(user::getCurrentUser().username) == "sys") {
            menu.addAction("添加用户", [=]() {
                AddUserDialog dlg(this); // 你需要自定义 AddUserDialog 类
                if (dlg.exec() == QDialog::Accepted) {
                    QString username = dlg.getUsername();
                    QString password = dlg.getPassword();

                    if (!username.isEmpty()) {
                        QString sql = "CREATE USER " + username;
                        if (!password.isEmpty()) {
                            sql += " IDENTIFIED BY " + password;
                        }
                        sql += ";\n\n";

                        QString currentText = ui->inputEdit->toPlainText();
                        ui->inputEdit->setPlainText(currentText + sql + "SQL>> ");

                        try {
                            Parse parser(ui->outputEdit, this);
                            parser.execute(sql);

                            // 获取用户填写的授权列表
                            QList<QPair<QString, QString>> grants = dlg.getGrants(); 

                            for (const auto& grant : grants) {
                                const QString& object = grant.first;   // 如 db 或 db.table
                                const QString& perm = grant.second;    // 如 connect、resource

                                if (!object.isEmpty() && !perm.isEmpty()) {
                                    QString grantSQL = "GRANT " + perm + " ON " + object + " TO " + username + ";\n\n";
                                    ui->inputEdit->moveCursor(QTextCursor::End);
                                    ui->inputEdit->insertPlainText(grantSQL + "SQL>> ");
                                    parser.execute(grantSQL);
                                }
                            }
                        }
                        catch (const std::exception& e) {
                            Output::printError(ui->outputEdit, QString("创建用户失败: ") + e.what());
                        }
                    }
                    else {
                        Output::printError(ui->outputEdit, "用户名不能为空！");
                    }
                }

                }

            );
        }
    }
    else {
        QTreeWidgetItem* parent = item->parent();
        QString dbName, tableName;

        if (!parent) {
            // 数据库节点
            dbName = item->text(0);

            menu.addAction("新建表", [=]() {
                AddTableDialog dlg(this, dbName);  // 传入当前数据库名
                if (dlg.exec() == QDialog::Accepted) {
                    QString tableName = dlg.getTableName();  // 用户填写的表名
                    QStringList columns = dlg.getColumnDefinitions(); 

                    if (!tableName.isEmpty() && !columns.isEmpty()) {
                        QString sql = "USE DATABASE " + dbName + ";   ";
                        sql += "CREATE TABLE " + tableName + " (";
                        sql += columns.join(", ");
                        sql += ");\n\n";

                        QString currentText = ui->inputEdit->toPlainText();
                        ui->inputEdit->setPlainText(currentText + sql + "SQL>> ");

                        try {
                            Parse parser(ui->outputEdit, this);
                            parser.execute("USE DATABASE " + dbName + ";");
                            parser.execute("CREATE TABLE " + tableName + " (" + columns.join(", ") + ");");
                        }
                        catch (const std::exception& e) {
                            Output::printError(ui->outputEdit, QString("创建数据表失败: ") + e.what());
                        }
                    }
                    else {
                        Output::printError(ui->outputEdit, "表名或列定义不能为空！");
                    }
                }
                });

            menu.addAction("删除数据库", [=]() {
                // 弹出确认删除窗口
                QMessageBox::StandardButton reply = QMessageBox::question(this, "确认删除",
                    "您确定要删除数据库 " + dbName + " 吗？",
                    QMessageBox::Yes | QMessageBox::No);

                if (reply == QMessageBox::Yes) {
                    QString sql = "DROP DATABASE " + dbName + ";\n\n";
                    QString currentText = ui->inputEdit->toPlainText();
                    ui->inputEdit->setPlainText(currentText + sql + "SQL>> ");

                    try {
                        Parse parser(ui->outputEdit, this);
                        parser.execute(sql);
                    }
                    catch (const std::exception& e) {
                        Output::printError(ui->outputEdit, QString("删除数据库失败: ") + e.what());
                    }
                }
                });

        }
        else {
            // 表节点
            dbName = parent->text(0);
            tableName = item->text(0);

            menu.addAction("修改表", [=]() {
                QMessageBox::information(this, "修改表", "这里将来会弹出修改表窗口（表名：" + tableName + "）");

                });

            menu.addAction("删除表", [=]() {
                QMessageBox::StandardButton reply = QMessageBox::question(this, "确认删除",
                    "您确定要删除表 " + tableName + " 吗？",
                    QMessageBox::Yes | QMessageBox::No);

                if (reply == QMessageBox::Yes) {
                    QString useDb = "USE DATABASE " + dbName + ";   ";
                    QString sql = "DROP TABLE " + tableName + ";\n\n";
                    QString currentText = ui->inputEdit->toPlainText();
                    ui->inputEdit->setPlainText(currentText + useDb + sql + "SQL>> ");

                    try {
                        Parse parser(ui->outputEdit, this);
                        parser.execute(useDb);
                        parser.execute(sql);
                    }
                    catch (const std::exception& e) {
                        Output::printError(ui->outputEdit, QString("删除表失败: ") + e.what());
                    }
                }
                });
        }
    }

    menu.exec(ui->treeWidget->viewport()->mapToGlobal(pos));
}

