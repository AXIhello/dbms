#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QMap>
#include <QVariant>

class LogManager {
public:
    // 构造函数，指定数据库路径
    LogManager(const QString& dbPath);

    // 开始一个事务
    void startTransaction();

    // 提交事务
    void commit();

    // 回滚事务
    void rollback();

    // 记录 INSERT 操作
    void logInsert(const QString& table, int rowid);

    // 记录 UPDATE 操作
    void logUpdate(const QString& table, int rowid, const QMap<QString, QVariant>& oldValues);

    // 记录 DELETE 操作
    void logDelete(const QString& table, const QMap<QString, QVariant>& fullRow);

private:
    // 日志文件路径
    QString logFilePath;

    // 当前事务中的所有操作
    QList<QJsonObject> currentTransaction;

    // 事务标志：是否在一个事务中
    bool inTransaction;

    // 加载日志文件
    void loadLogFile();

    // 保存事务日志
    void saveTransactionLog();

    // 生成一个新的事务 ID
    int generateTransactionId();

    // 获取当前时间戳（用于日志记录）
    QString getCurrentTimestamp();
};

#endif // LOGMANAGER_H
