#include "dbms.h"
#include <QMessageBox>


dbms::dbms(QWidget *parent)
    : QMainWindow(parent)

{
    ui.setupUi(this);

    // 连接按钮点击事件到槽函数
    connect(ui.runButton, &QPushButton::clicked, this, &dbms::on_runButton_clicked);
  
}

dbms::~dbms()
{}


// 运行 SQL 按钮槽函数
void dbms::on_runButton_clicked()
{
    QString sql = ui.sqlEditor->toPlainText().trimmed(); // 获取 SQL 语句

    if (sql.isEmpty())
    {
        QMessageBox::warning(this, "警告", "SQL 语句不能为空！");
        return;
    }

    QString message;
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

    QMessageBox::information(this, "SQL 解析", message);
}


