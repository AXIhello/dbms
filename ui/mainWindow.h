
#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "ui_mainwindow.h"
#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include"manager/dbManager.h"
#include <QWidget> 
#include <QListWidget>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }  // 这里声明 Ui::MainWindow
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    void refreshTree();
private slots:
    void onRunButtonClicked();  // 声明槽函数
    void onTreeItemClicked(QTreeWidgetItem* item, int column);
    void onTreeWidgetContextMenu(const QPoint& pos);
    void onSwitchUser();

private:
    Ui::MainWindow* ui;  // 声明一个 Ui::MainWindow 指针
    QWidget* buttonWidget;  // 声明 buttonWidget
    QGroupBox* userInfoGroupBox;
    QListWidget* userListWidget;
    QPushButton* toggleUserListButton;
    bool userListExpanded = false;

    QDialog* userListDialog;
    void createUserListDialog();

signals:
    void requestSwitchUser();

};

#endif // MAINWINDOW_H
