#include "mainwindow.h"
#include "ui_mainwindow.h"  // 包含 UI 头文件
#include <QSplitter>
#include <QBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QMessageBox>
#include"ui/output.h"
#include "manager/parse.h" 
#include "manager/dbManager.h"
#include <QGroupBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)// 初始化 UI
{
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

    this->setStyleSheet(styleSheet);

    // 连接按钮点击事件到槽函数
    connect(ui->runButton, &QPushButton::clicked, this, &MainWindow::onRunButtonClicked);
    connect(ui->treeWidget, &QTreeWidget::itemClicked, this, &MainWindow::onTreeItemClicked);
    setWindowTitle("My Database Client"); // 设置窗口标题
    setGeometry(100, 100, 1000, 600);  // 设置窗口大小

    // 设置菜单栏的样式
    menuBar()->setStyleSheet("QMenuBar { background-color: lightgray; }");
    ui->outputEdit->setReadOnly(true);

    // 设置按钮的宽度
    ui->runButton->setMinimumWidth(150);  // 设置按钮最小宽度
    ui->runButton->setMaximumWidth(150);  // 设置按钮最大宽度

    // 创建一个单独的QWidget来包裹runButton
    buttonWidget = new QWidget(this);  // 把 buttonWidget 声明为成员变量
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->addWidget(ui->runButton);
    buttonWidget->setLayout(buttonLayout);

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
    treeLayout->addWidget(ui->treeWidget);
    treeGroupBox->setMinimumWidth(200);
    treeGroupBox->setStyleSheet("QGroupBox { font-weight: bold; font-size: 14pt; }");

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

    refreshTree(); 
   }

MainWindow::~MainWindow() {
    delete ui;  // 释放 UI 资源
    dbManager::getInstance().clearCache();
}

void MainWindow::onRunButtonClicked() {
    //QString sql = ui->inputEdit->toPlainText().trimmed(); // 获取 SQL 语句
    QString sql = ui->inputEdit->toPlainText().split('\n', Qt::SkipEmptyParts).last().trimmed();//只获取最后一行的语句

    if (sql.isEmpty())
    {
        QMessageBox::warning(this, "警告", "SQL 语句不能为空！");
        return;
    }

    Parse parser(ui->outputEdit,this);
    parser.execute(sql);
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
        qDebug() << "运行时错误:" << e.what();
    }
    catch (const std::exception& e) {
        Output::printError(ui->outputEdit, QString("其他异常: ") + e.what());
        qDebug() << "其他异常:" << e.what();
    }
}

void MainWindow::onTreeItemClicked(QTreeWidgetItem* item, int column) {
    QTreeWidgetItem* parent = item->parent();

    if (!parent) {
        // === 点击数据库节点 ===
        QString dbName = item->text(0);
        
        // 显示 SQL 并执行
        QString sql = "USE DATABASE " + dbName + ";\n";
        ui->inputEdit->setPlainText(sql);
        Parse parser(ui->outputEdit, this);
        parser.execute(sql);
        return;
    }

    // === 点击表节点 ===
    QString tableName = item->text(0);
    QString sql = "SELECT * FROM " + tableName + ";\n";
    ui->inputEdit->setPlainText(sql);
    QString dbName = parent->text(0);

        try {
            Parse parser(ui->outputEdit, this);
            parser.execute(sql);
        }
        catch (const std::exception& e) {
            Output::printError(ui->outputEdit, QString("加载表失败: ") + e.what());
        }
    }
