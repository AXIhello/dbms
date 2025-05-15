// LogManager.h
//TODO:
#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <optional>
#include "transaction/TransactionManager.h"
#include "manager/dbManager.h"
// 日志记录类型
enum class LogType {
    BEGIN,          // 事务开始 0
    INSERT,         // 插入操作 1
    DELETE,         // 删除操作 2
    UPDATE,         // 更新操作 3
    COMMIT,         // 事务提交 4
    ROLLBACK,       // 事务回滚 5
};

// 日志项结构
struct LogEntry {
    LogType type;                   // 日志类型
    //uint64_t transactionId;         // 事务ID
    std::string tableName;          // 表名
    uint64_t rowId;                 // 行ID
    std::vector<std::pair<std::string, std::string>> oldValues;  // 旧值
    std::vector<std::pair<std::string, std::string>> newValues;  // 新值
    std::string timestamp;          // 时间戳
};

class LogManager {
public:
    static LogManager& instance();  // 单例模式

    // 初始化日志管理器
    bool initialize(const std::string& 
    );

    // 关闭日志管理器
    void shutdown();


    // 记录DML操作日志
    void logInsert(const std::string& tableName, uint64_t rowId, 
        const std::vector<std::pair<std::string, std::string>>& insertedValues);

    void logDelete(const std::string& tableName, uint64_t rowId,
        const std::vector<std::pair<std::string, std::string>>& values_to_delete); //实际上还未删除，只是把flag=1
    void logUpdate(const std::string& tableName, uint64_t rowId,
        const std::vector<std::pair<std::string, std::string>>& oldValues,
        const std::vector<std::pair<std::string, std::string>>& newValues);

	//记录事务开始
    void logBegin();
    // 记录事务提交
    void logCommit();
    // 记录事务回滚
    void logRollback();

    // 创建检查点
    void createCheckpoint();

    // 崩溃恢复
    void recoverFromCrash();



private:
    LogManager();  // 构造函数私有化

    // 写入日志条目
    void writeLogEntry(const LogEntry& entry);

    // 从日志文件中解析日志条目
    std::vector<LogEntry> parseLogFile();

    // 执行redo操作
    void redoOperations(const std::vector<LogEntry>& entries);

    // 执行undo操作
    void undoOperations(const std::vector<LogEntry>& entries);

    void recoverUndo();

    // 获取当前时间戳
    std::string getCurrentTimestamp();

    // 检查系统是否崩溃
    bool isSystemCrashed(const std::vector<LogEntry>& entries);

    std::string dbName;              // 数据库名
    std::string logFilePath;         // 日志文件路径
    std::ofstream logFile;           // 日志文件流
    uint64_t nextTransactionId;      // 下一个事务ID
    std::mutex logMutex;             // 日志互斥锁，用于多客户端并发
    bool initialized;                // 是否初始化
};