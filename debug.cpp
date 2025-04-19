#include "Debug.h"
#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <QString>
#include "fieldBlock.h"
#include "constraintBlock.h"
#include "tableBlock.h"
#include"databaseBlock.h"

#include <ctime>
#include <QString>
#include <QFile>
#include <QDataStream>
#include <QDebug>

void debug::printDB(const std::string& dbPath) {
    // 将 std::string 转换为 QString
    QString path = QString::fromStdString(dbPath);

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开文件:" << path;
        return;
    }

    int index = 0;
    bool isEmpty = true;

    // 使用 QDataStream 读取数据库信息
    QDataStream in(&file);
    while (!file.atEnd()) {
        DatabaseBlock dbRecord;

        // 读取每个数据库的记录
        in.readRawData(reinterpret_cast<char*>(&dbRecord), sizeof(DatabaseBlock));

        // 转换 time_t 为可读的字符串格式
        std::time_t createTime = dbRecord.crtime;  // crtime 是 time_t 类型
        std::tm timeInfo;
        errno_t err = localtime_s(&timeInfo, &createTime);  // 使用 thread-safe 的 localtime_s

        // 如果 localtime_s 执行成功，则格式化时间
        if (err == 0) {
            char buffer[20];  // 用于保存格式化的时间字符串
            std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeInfo);

            // 输出数据库信息
            qDebug() << "数据库" << index++;
            qDebug() << "  名称:" << QString::fromUtf8(dbRecord.dbName);
            qDebug() << "  类型:" << (dbRecord.type ? "用户数据库" : "系统数据库");
            qDebug() << "  数据文件夹路径:" << QString::fromUtf8(dbRecord.filepath);
            qDebug() << "  创建时间:" << QString::fromUtf8(buffer);
        }
        else {
            qDebug() << "时间转换失败";
        }

        isEmpty = false;
    }

    if (isEmpty) {
        qDebug() << "文件为空:" << path;
    }

    file.close();
}

void debug::printTB(const std::string& tbPath) {
    QString path = QString::fromStdString(tbPath);

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开文件:" << path;
        return;
    }

    int index = 0;
    bool isEmpty = true;
    QDataStream in(&file);

    while (!file.atEnd()) {
        TableBlock table;
        in.readRawData(reinterpret_cast<char*>(&table), sizeof(TableBlock));

        // 转换创建时间
        std::tm create_tm;
        char create_buf[20];
        if (localtime_s(&create_tm, &table.crtime) == 0) {
            std::strftime(create_buf, sizeof(create_buf), "%Y-%m-%d %H:%M:%S", &create_tm);
        }
        else {
            strcpy_s(create_buf, "转换失败");
        }

        // 转换修改时间
        std::tm modify_tm;
        char modify_buf[20];
        if (localtime_s(&modify_tm, &table.mtime) == 0) {
            std::strftime(modify_buf, sizeof(modify_buf), "%Y-%m-%d %H:%M:%S", &modify_tm);
        }
        else {
            strcpy_s(modify_buf, "转换失败");
        }

        // 输出内容
        qDebug() << "表" << index++;
        qDebug() << "  名称:" << table.name;
        qDebug() << "  字段数:" << table.field_num;
        qDebug() << "  记录数:" << table.record_num;
        qDebug() << "  结构文件路径:" << table.tdf;
        qDebug() << "  约束文件路径:" << table.tic;
        qDebug() << "  记录文件路径:" << table.trd;
        qDebug() << "  索引文件路径:" << table.tid;
        qDebug() << "  创建时间:" << create_buf;
        qDebug() << "  修改时间:" << modify_buf;

        isEmpty = false;
    }

    if (isEmpty) {
        qDebug() << "文件为空:" << path;
    }

    file.close();
}

void debug::printTDF(const std::string& tdfPath) {
    // 将 std::string 转换为 QString
    QString path = QString::fromStdString(tdfPath);

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开文件:" << path;
        return;
    }

    int index = 0;
    bool isEmpty = true;
    while (!file.atEnd()) {
        FieldBlock field;
        QDataStream in(&file);
        in.readRawData(reinterpret_cast<char*>(&field), sizeof(FieldBlock));

        qDebug() << "字段" << index++;
        qDebug() << "  名称:" << field.name;
        qDebug() << "  类型:" << field.type;
        qDebug() << "  参数:" << field.param;
        qDebug() << "  顺序:" << field.order;
        qDebug() << "  修改时间:" << field.mtime;
        qDebug() << "  完整性:" << field.integrities;

        isEmpty = false;
    }

    if (isEmpty) {
        qDebug() << "文件为空:" << path;
    }

    file.close();
}
void debug::printTIC(const std::string& ticPath)
{
    // 将 std::string 转换为 QString
    QString path = QString::fromStdString(ticPath);

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开文件:" << path;
        return;
    }

    int index = 0;
    bool isEmpty = true;
    while (!file.atEnd()) {
        ConstraintBlock constraint;
        QDataStream in(&file);
        in.readRawData(reinterpret_cast<char*>(&constraint), sizeof(ConstraintBlock));

        qDebug() << "约束" << index++;
        qDebug() << "  约束名称:" << constraint.name;
        qDebug() << "  字段名称:" << constraint.field;
        qDebug() << "  约束类型:" << constraint.type;
        qDebug() << "  参数:" << constraint.param;

        isEmpty = false;
    }

    if (isEmpty) {
        qDebug() << "文件为空:" << path;
    }

    file.close();
}
