#include "mainwindow.h"
#include "ui_mainwindow.h"  // 包含 UI 头文件
#include <qsplitter.h>
#include <qboxlayout.h>

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
    // 输入框和输出框使用垂直分割
    QSplitter* ioSplitter = new QSplitter(Qt::Vertical);
    ioSplitter->addWidget(ui->inputEdit);
    ioSplitter->addWidget(ui->runButton);
    ioSplitter->addWidget(ui->outputEdit);
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

}

MainWindow::~MainWindow() {
    delete ui;  // 释放 UI 资源
}