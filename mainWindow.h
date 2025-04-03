
#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "ui_mainwindow.h"
#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QWidget> 

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }  // 这里声明 Ui::MainWindow
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
private slots:
    void onRunButtonClicked();  // 声明槽函数

private:
    Ui::MainWindow* ui;  // 声明一个 Ui::MainWindow 指针
    QWidget* buttonWidget;  // 声明 buttonWidget
};

#endif // MAINWINDOW_H
