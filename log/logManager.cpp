// LogManager.cpp
#include "LogManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <set>
#include <unordered_set>
#include "base/record/Record.h"

#include <json.hpp>
using json = nlohmann::json;

// 单例实现

LogManager& LogManager::instance() {
    static LogManager instance;
    return instance;
}

// 构造函数
LogManager::LogManager() : nextTransactionId(1), initialized(false) {}

// 初始化日志管理器
bool LogManager::initialize(const std::string& dbName) {
    std::lock_guard<std::mutex> lock(logMutex);

    if (initialized) return true;

    this->dbName = dbName;
    logFilePath = "DBMS_ROOT/data/" + dbName + "/" + dbName + ".log";

    // 1. 打开日志文件（以追加模式）
    logFile.open(logFilePath, std::ios::app);
    if (!logFile.is_open()) {
        return false;
    }

    // 2. 解析日志文件
    std::vector<LogEntry> entries = parseLogFile();

    // 3. 检查是否有未提交事务（崩溃恢复）
    /*if (isSystemCrashed(entries)) {
        recoverFromCrash();
    }*/

    initialized = true;
    return true;
}

// 关闭日志管理器
void LogManager::shutdown() {
    std::lock_guard<std::mutex> lock(logMutex);

    if (!initialized) return;

    // 写入SHUTDOWN日志
    LogEntry entry;
    entry.type = LogType::SHUTDOWN;
    entry.timestamp = getCurrentTimestamp();
    writeLogEntry(entry);

    logFile.close();
    initialized = false;
}



// 日志记录插入操作
void LogManager::logInsert( 
    const std::string& tableName, 
    uint64_t rowId, 
    const std::vector<std::pair<std::string, std::string>>& insertedValues)
{
    std::lock_guard<std::mutex> lock(logMutex);

    if (!initialized) {
        std::cerr << "LogManager not initialized!" << std::endl;
        return;
    }

    LogEntry entry;
    entry.type = LogType::INSERT;
    entry.tableName = tableName;
    entry.rowId = rowId;
    entry.newValues = insertedValues;
    entry.timestamp = getCurrentTimestamp();

    writeLogEntry(entry);
}

 //日志记录删除操作
void LogManager::logDelete(
    const std::string& tableName,
    uint64_t rowId,
    const std::vector<std::pair<std::string, std::string>>& values_to_delete) {
    std::lock_guard<std::mutex> lock(logMutex);

    if (!initialized) {
        std::cerr << "LogManager not initialized!" << std::endl;
        return;
    }

    LogEntry entry;
    entry.type = LogType::DELETE;
    entry.tableName = tableName;
    entry.rowId = rowId;
    entry.oldValues = values_to_delete;
    entry.timestamp = getCurrentTimestamp();

    writeLogEntry(entry);
}

 //日志记录更新操作
void LogManager::logUpdate(const std::string& tableName, uint64_t rowId,
    const std::vector<std::pair<std::string, std::string>>& oldValues,
    const std::vector<std::pair<std::string, std::string>>& newValues) {
    std::lock_guard<std::mutex> lock(logMutex);

    if (!initialized) {
        std::cerr << "LogManager not initialized!" << std::endl;
        return;
    }

    LogEntry entry;
    entry.type = LogType::UPDATE;
    entry.tableName = tableName;
    entry.rowId = rowId;
    entry.oldValues = oldValues;
    entry.newValues = newValues;
    entry.timestamp = getCurrentTimestamp();

    writeLogEntry(entry);
}

// 记录BEGIN日志
void LogManager::logBegin() {
    std::lock_guard<std::mutex> lock(logMutex);

    if (!initialized) {
        std::cerr << "LogManager not initialized!" << std::endl;
        return;
    }

    LogEntry entry;
    entry.type = LogType::BEGIN;
    entry.rowId = 0; //为空
    entry.timestamp = getCurrentTimestamp();
    writeLogEntry(entry);
}
// 记录事务提交
void LogManager::logCommit() {
    std::lock_guard<std::mutex> lock(logMutex);

    if (!initialized) {
        std::cerr << "LogManager not initialized!" << std::endl;
        return;
    }

    LogEntry entry;
    entry.type = LogType::COMMIT;
    entry.rowId = 0; //为空
    entry.timestamp = getCurrentTimestamp();

    writeLogEntry(entry);
}

// 记录事务回滚
void LogManager::logRollback() {
    std::lock_guard<std::mutex> lock(logMutex);

    if (!initialized) {
        std::cerr << "LogManager not initialized!" << std::endl;
        return;
    }

    LogEntry entry;
    entry.type = LogType::ROLLBACK;
    entry.rowId = 0; //为空
    entry.timestamp = getCurrentTimestamp();

    writeLogEntry(entry);
}

// 创建检查点
void LogManager::createCheckpoint() {
    std::lock_guard<std::mutex> lock(logMutex);

    if (!initialized) {
        std::cerr << "LogManager not initialized!" << std::endl;
        return;
    }

    // 此处应将当前数据库状态持久化到磁盘
    // 具体实现取决于数据库的存储方式...

    // 记录检查点日志
    LogEntry entry;
    entry.type = LogType::CHECKPOINT;
    entry.timestamp = getCurrentTimestamp();

    writeLogEntry(entry);
}


// 写入日志条目
void LogManager::writeLogEntry(const LogEntry& entry) {
    if (!logFile.is_open()) {
        std::cerr << "Log file not open!" << std::endl;
        return;
    }

    json j;
    j["type"] = static_cast<int>(entry.type);  // 然后写入 type
    j["tableName"] = entry.tableName;
    j["rowId"] = entry.rowId;
    j["oldValues"] = entry.oldValues;
    j["newValues"] = entry.newValues;
    j["timestamp"] = entry.timestamp;

    logFile << j.dump() << std::endl;  // 一行一个 JSON 对象
    logFile.flush();  // 确保写入磁盘
}


 //解析日志文件
std::vector<LogEntry> LogManager::parseLogFile() {
    std::vector<LogEntry> entries;

    std::ifstream inFile(logFilePath);
    if (!inFile.is_open()) {
        std::cerr << "Failed to open log file for parsing!" << std::endl;
        return entries;
    }

    std::string line;
    while (std::getline(inFile, line)) {
        if (line.empty()) continue;

        try {
            json j = json::parse(line);

            LogEntry entry;
            entry.type = static_cast<LogType>(j.value("type", 0));
            entry.tableName = j.value("tableName", "");
            entry.rowId = j.value("rowId", 0);
            entry.timestamp = j.value("timestamp", "");

            // 解析 oldValues 和 newValues 为键值对
            entry.oldValues.clear();
            if (j.contains("oldValues")) {
                for (const auto& item : j["oldValues"].items()) {
                    entry.oldValues.push_back({ item.key(), item.value() });
                }
            }

            entry.newValues.clear();
            if (j.contains("newValues")) {
                for (const auto& item : j["newValues"].items()) {
                    entry.newValues.push_back({ item.key(), item.value() });
                }
            }

            entries.push_back(entry);
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to parse log line: " << e.what() << std::endl;
            continue;  // 跳过解析失败的行，继续处理其他行
        }
    }

    return entries;
}

void LogManager::recoverUndo() {

}


// 从崩溃中恢复
//bool LogManager::recoverFromCrash() {
//    std::cout << "Starting crash recovery..." << std::endl;
//
//    // 解析日志文件
//    std::vector<LogEntry> entries = parseLogFile();
//
//    if (entries.empty()) {
//        std::cout << "No log entries found. Nothing to recover." << std::endl;
//        return true;
//    }
//
//    // 查找最后一个检查点位置
//    size_t checkpointPos = entries.size();
//    for (int i = entries.size() - 1; i >= 0; --i) {
//        if (entries[i].type == LogType::CHECKPOINT) {
//            checkpointPos = i;
//            break;
//        }
//    }
//
//    // 从检查点位置开始恢复
//    std::cout << "Performing REDO operations..." << std::endl;
//    redoOperations(std::vector<LogEntry>(entries.begin() + checkpointPos, entries.end()));
//
//    // 处理未提交的事务
//    std::cout << "Performing UNDO operations for uncommitted transactions..." << std::endl;
//    undoOperations(entries);
//
//    std::cout << "Recovery completed successfully." << std::endl;
//    return true;
//}

// 执行重做操作
//void LogManager::redoOperations(const std::vector<LogEntry>& entries) {
//    // 找出所有已提交的事务
//    std::set<uint64_t> committedTxns;
//    for (const auto& entry : entries) {
//        if (entry.type == LogType::COMMIT) {
//            committedTxns.insert(entry.transactionId);
//        }
//    }
//
//    Record record;
//
//    // 重做所有已提交事务的操作
//    for (const auto& entry : entries) {
//        if (committedTxns.find(entry.transactionId) == committedTxns.end()) {
//            continue;  // 跳过未提交的事务
//        }
//
//        switch (entry.type) {
//        case LogType::INSERT:
//            // 此处应该重做插入操作，但由于没有具体数据，只能示意
//            std::cout << "REDO: INSERT into " << entry.tableName << " at RowID " << entry.rowId << std::endl;
//            break;
//
//        case LogType::DELETE:
//            // 重做删除操作
//            std::cout << "REDO: DELETE from " << entry.tableName << " at RowID " << entry.rowId << std::endl;
//            record.rollback_delete_by_rowid(entry.tableName, entry.rowId);
//            break;
//
//        case LogType::UPDATE:
//            // 重做更新操作
//            if (!entry.newValues.empty()) {
//                std::cout << "REDO: UPDATE " << entry.tableName << " at RowID " << entry.rowId << std::endl;
//                record.rollback_update_by_rowid(entry.tableName, { {entry.rowId, entry.newValues} });
//            }
//            break;
//
//        default:
//            break;
//        }
//    }
//}
//
//// 执行撤销操作
//void LogManager::undoOperations(const std::vector<LogEntry>& entries) {
//    // 找出所有未提交的事务
//    std::unordered_set<uint64_t> uncommittedTxns;
//    std::unordered_set<uint64_t> allTxns;
//    std::unordered_set<uint64_t> committedTxns;
//
//    for (const auto& entry : entries) {
//        if (entry.type == LogType::BEGIN) {
//            allTxns.insert(entry.transactionId);
//            uncommittedTxns.insert(entry.transactionId);
//        }
//        else if (entry.type == LogType::COMMIT) {
//            uncommittedTxns.erase(entry.transactionId);
//            committedTxns.insert(entry.transactionId);
//        }
//        else if (entry.type == LogType::ROLLBACK) {
//            uncommittedTxns.erase(entry.transactionId);
//        }
//    }
//
//    // 按照逆序处理未提交事务的操作
//    Record record;
//    std::unordered_map<uint64_t, std::vector<LogEntry>> txnOperations;
//
//    // 收集每个未提交事务的操作
//    for (const auto& entry : entries) {
//        if (uncommittedTxns.find(entry.transactionId) != uncommittedTxns.end()) {
//            if (entry.type == LogType::INSERT || entry.type == LogType::DELETE || entry.type == LogType::UPDATE) {
//                txnOperations[entry.transactionId].push_back(entry);
//            }
//        }
//    }
//
//    // 对每个未提交事务执行撤销操作
//    for (const auto& txnPair : txnOperations) {
//        uint64_t txId = txnPair.first;
//        const auto& ops = txnPair.second;
//
//        std::cout << "UNDO: Rolling back transaction " << txId << std::endl;
//
//        // 逆序遍历操作
//        for (auto it = ops.rbegin(); it != ops.rend(); ++it) {
//            const LogEntry& entry = *it;
//
//            switch (entry.type) {
//            case LogType::INSERT:
//                // 撤销插入 -> 删除
//                std::cout << "UNDO: Removing inserted row from " << entry.tableName << " at RowID " << entry.rowId << std::endl;
//                record.rollback_insert_by_rowid(entry.tableName, entry.rowId);
//                break;
//
//            case LogType::DELETE:
//                // 撤销删除 -> 恢复
//                std::cout << "UNDO: Restoring deleted row in " << entry.tableName << " at RowID " << entry.rowId << std::endl;
//                record.rollback_delete_by_rowid(entry.tableName, entry.rowId);
//                break;
//
//            case LogType::UPDATE:
//                // 撤销更新 -> 恢复旧值
//                if (!entry.oldValues.empty()) {
//                    std::cout << "UNDO: Reverting update in " << entry.tableName << " at RowID " << entry.rowId << std::endl;
//                    record.rollback_update_by_rowid(entry.tableName, { {entry.rowId, entry.oldValues} });
//                }
//                break;
//
//            default:
//                break;
//            }
//        }
//
//        // 记录回滚日志
//        logRollback(txId);
//    }
//}

//// 检查系统是否崩溃
//bool LogManager::isSystemCrashed(const std::vector<LogEntry>& entries) {
//    if (entries.empty()) {
//        return false;
//    }
//
//    // 检查最后一条日志是否为SHUTDOWN
//    const LogEntry& lastEntry = entries.back();
//    return lastEntry.type != LogType::SHUTDOWN;
//}

// 获取当前时间戳
std::string LogManager::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto nowTime = std::chrono::system_clock::to_time_t(now);

    std::tm localTime;
    localtime_s(&localTime, &nowTime); // 使用 localtime_s 获取线程安全的本地时间

    std::stringstream ss;
    ss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S"); // 格式化时间戳

    return ss.str();
}