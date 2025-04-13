#include "Debug.h"
#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <QString>
#include "fieldBlock.h"

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
