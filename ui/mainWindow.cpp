#include "mainwindow.h"
#include "ui_mainwindow.h"  // 包含 UI 头文件
#include <QSplitter>
#include <QBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QMessageBox>
#include"ui/output.h"
#include "manager/parse.h" 

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)  // 初始化 UI
{
    ui->setupUi(this);  // 让 UI 组件和窗口关联
    setWindowTitle("My Database Client"); // 设置窗口标题
    setGeometry(100, 100, 1000, 600);  // 设置窗口大小

    // 设置菜单栏的样式
    menuBar()->setStyleSheet("QMenuBar { background-color: lightgray; }");
    ui->dockWidget->setMinimumWidth(200);
    ui->outputEdit->setReadOnly(true);

    // 设置按钮的宽度
    ui->runButton->setMinimumWidth(150);  // 设置按钮最小宽度
    ui->runButton->setMaximumWidth(150);  // 设置按钮最大宽度

    // 创建一个单独的QWidget来包裹runButton
    buttonWidget = new QWidget(this);  // 把 buttonWidget 声明为成员变量
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->addWidget(ui->runButton);
    buttonWidget->setLayout(buttonLayout);

    // 输入框和输出框使用垂直分割
    QSplitter* ioSplitter = new QSplitter(Qt::Vertical);
    ioSplitter->addWidget(ui->inputEdit);  // 输入框
    ioSplitter->addWidget(buttonWidget);   // 按钮放在单独的布局里
    ioSplitter->addWidget(ui->outputEdit); // 输出框
    ioSplitter->setStretchFactor(0, 5);  // 输入框占较多空间
    ioSplitter->setStretchFactor(1, 1);  // 按钮占较少空间
    ioSplitter->setStretchFactor(2, 3);  // 输出框占较少空间

    // 左侧导航栏使用 DockWidget
    QDockWidget* dockWidget = new QDockWidget("Navigation", this);
    dockWidget->setWidget(ui->treeWidget);
    dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    // 主内容区水平分割
    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->addWidget(ioSplitter);
    mainSplitter->setStretchFactor(0, 3);  // 主要区域占较多空间

    // 主窗口部件
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addWidget(mainSplitter);
    setCentralWidget(centralWidget);

    // 将DockWidget添加到主窗口
    addDockWidget(Qt::LeftDockWidgetArea, dockWidget);

    // 连接按钮点击事件到槽函数
    connect(ui->runButton, &QPushButton::clicked, this, &MainWindow::onRunButtonClicked);
}

MainWindow::~MainWindow() {
    delete ui;  // 释放 UI 资源
}

void MainWindow::onRunButtonClicked() {
    QString sql = ui->inputEdit->toPlainText().trimmed(); // 获取 SQL 语句

    if (sql.isEmpty())
    {
        QMessageBox::warning(this, "警告", "SQL 语句不能为空！");
        return;
    }
    
    Parse parser(ui->outputEdit);
    parser.execute(sql);
   /* QString message;
    if (sql.startsWith("SELECT", Qt::CaseInsensitive))
    {
        message = "解析结果：这是一个 SELECT 语句。";
    }
    else if (sql.startsWith("INSERT", Qt::CaseInsensitive) ||
        sql.startsWith("UPDATE", Qt::CaseInsensitive) ||
        sql.startsWith("DELETE", Qt::CaseInsensitive) ||
        sql.startsWith("CREATE", Qt::CaseInsensitive) ||
        sql.startsWith("DROP", Qt::CaseInsensitive))
    {
        message = "解析结果：这是一个 数据修改 语句。";
    }
    else
    {
        message = "解析结果：不支持的 SQL 语句！";
    }

    QMessageBox::information(this, "SQL 解析", message);*/
}
