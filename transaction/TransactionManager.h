// TransactionManager.h
#pragma once
#include <stack>
#include <vector>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include"base/record/Record.h"
#include"manager/dbManager.h"
#include <optional>

enum class DmlType {
    INSERT,
    DELETE,
    UPDATE
};

struct UndoOperation {
    DmlType type;
    std::string tableName;
    uint64_t rowId;  // 改为 uint64_t
    std::vector<std::pair<std::string, std::string>> oldValues;
};


class TransactionManager {
public:
    static TransactionManager& instance(); // 单例模式

    void begin();        // 开始事务
    void commit();       // 提交事务
    int rollback();     // 回滚事务

    void beginImplicitTransaction();   // 开始隐式事务
    void commitImplicitTransaction();  // 提交隐式事务
    

    void addUndo(DmlType type, const std::string& tableName, uint64_t rowId, const std::vector<std::pair<std::string, std::string>>& oldValues);
    
    void addUndo(DmlType type, const std::string& tableName, uint64_t rowId);//对于INSERT和DELETE操作

    bool isActive() const;  // 判断事务是否正在进行
	bool isAutoCommit() const;  // 判断是否启用自动提交

    void setAutoCommit(bool flag);

    uint64_t getCurrentTransactionId() const;

   
private:
    TransactionManager();  // 构造函数私有化
    bool active;  // 事务是否处于激活状态
    bool autoCommit;  // 是否启用自动提交
    std::optional<bool> lastAutoCommit;
    std::vector<UndoOperation> undoStack;  // 存储UNDO操作
    
    uint64_t txnId;  //  当前事务ID
    uint64_t txnIdCounter;  // 全局递增计数器

};
