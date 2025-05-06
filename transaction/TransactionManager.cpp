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
    undoStack.clear();  // 提交事务时清空UNDO栈;TODO:将标识为1的数据真正删除后再清空？
}

void TransactionManager::rollback() {
    // 遍历undoStack逆序回滚
    for (auto it = undoStack.rbegin(); it != undoStack.rend(); ++it) {
        const UndoOperation& op = *it;
        switch (op.type) {
        case DmlType::INSERT:
        {
            Record record;
            record.delete_by_rowid(op.tableName, op.rowId);
            break;
        }
        
        case DmlType::DELETE:
        {

            Record record;
			record.rollback_delete_by_rowid(op.tableName, op.rowId);
            break;
        }
        case DmlType::UPDATE:
        {
            Record record;
            record.update_by_rowid(op.tableName, { { op.rowId, op.oldValues } });
            break;
        }
        }
    }
    undoStack.clear();  // 完成回滚，清空UNDO栈
    active = false;
}

#include <unordered_map> // 添加此行以确保 std::unordered_map 被正确包含

void TransactionManager::addUndo(DmlType type, const std::string& tableName, uint64_t rowId) {
    if (!active) return;

    UndoOperation op;
    op.type = type;
    op.tableName = tableName;
    op.rowId = rowId;


    undoStack.push_back(op);
}

void TransactionManager::addUndo(DmlType type, const std::string& tableName, uint64_t rowId,
    const std::vector<std::pair<std::string, std::string>>& oldValues) {
    if (!active) return;

    UndoOperation op;
    op.type = type;
    op.tableName = tableName;
    op.rowId = rowId;
    op.oldValues = oldValues;

    undoStack.push_back(op);
}

bool TransactionManager::isActive() const {
    return active;  // 返回事务是否正在进行
}
