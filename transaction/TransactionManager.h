// TransactionManager.h
#pragma once
#include <stack>
#include <vector>
#include <string>

enum class DmlType {
    INSERT,
    DELETE,
    UPDATE
};

struct UndoOperation {
    DmlType type;
    std::string tableName;
    int rowId;
    std::vector<std::pair<std::string, std::string>> oldValues; // 记录更新/删除时的旧值
};

class TransactionManager {
public:
    static TransactionManager& instance(); // 单例模式

    void begin();        // 开始事务
    void commit();       // 提交事务
    void rollback();     // 回滚事务

    void addUndo(const UndoOperation& op);   // 记录UNDO操作
    bool isActive() const;  // 判断事务是否正在进行

private:
    TransactionManager();  // 构造函数私有化
    bool active;  // 事务是否处于激活状态
    bool autoCommit;  // 是否启用自动提交

    std::vector<UndoOperation> undoStack;  // 存储UNDO操作
};
