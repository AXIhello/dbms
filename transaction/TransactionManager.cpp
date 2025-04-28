// TransactionManager.cpp
#include"TransactionManager.h"

TransactionManager::TransactionManager() : active(false),autoCommit(true) {}

TransactionManager& TransactionManager::instance() {
    static TransactionManager instance;
    return instance;
}

void TransactionManager::begin() {
    active = true;
    undoStack.clear();  // 开始新事务时清空上次的UNDO栈
}

void TransactionManager::commit() {
    active = false;
    undoStack.clear();  // 提交事务时清空UNDO栈
}

void TransactionManager::rollback() {
    // 遍历undoStack逆序回滚
    for (auto it = undoStack.rbegin(); it != undoStack.rend(); ++it) {
        const UndoOperation& op = *it;
        switch (op.type) {
        case DmlType::INSERT:
            // 回滚INSERT：删除对应行
            // e.g. Table::deleteRow(op.tableName, op.rowId);
            break;
        case DmlType::DELETE:
            // 回滚DELETE：恢复行数据
            // e.g. Table::insertRow(op.tableName, op.oldValues);
            break;
        case DmlType::UPDATE:
            // 回滚UPDATE：恢复旧值
            // e.g. Table::updateRow(op.tableName, op.rowId, op.oldValues);
            break;
        }
    }
    undoStack.clear();  // 完成回滚，清空UNDO栈
    active = false;
}

void TransactionManager::addUndo(const UndoOperation& op) {
    if (active) {
        undoStack.push_back(op);  // 记录UNDO操作
    }
}

bool TransactionManager::isActive() const {
    return active;  // 返回事务是否正在进行
}
