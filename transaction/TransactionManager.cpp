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
            // 读取所有记录
        {
            auto records = Record::read_records(op.tableName);
            

            for (const auto& [rowID, recordData] : records) {
                if (rowID == op.rowId) {
                    // 找到要删除的记录
                    Record record;
                    record.delete_by_rowid(op.tableName, op.rowId);
                    break;
                }
            }
        }
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

#include <unordered_map> // 添加此行以确保 std::unordered_map 被正确包含

void TransactionManager::addUndo(DmlType type, const std::string& tableName, uint64_t rowId) {
    if (!active) return;

    UndoOperation op;
    op.type = type;
    op.tableName = tableName;
    op.rowId = rowId;
    // INSERT 不需要 oldValues

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
