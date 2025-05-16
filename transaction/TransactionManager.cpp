// TransactionManager.cpp
#include"TransactionManager.h"

//uint64_t TransactionManager::nextTransactionId = 1;  // 初始化为 1 
TransactionManager::TransactionManager() : active(false),autoCommit(false){}

TransactionManager& TransactionManager::instance() {
    static TransactionManager instance;
    return instance;
}

void TransactionManager::begin() {
    active = true;
    lastAutoCommit = autoCommit;
    autoCommit = false;
    undoStack.clear();  // 开始新事务时清空上次的UNDO栈

	LogManager::instance().logBegin();  // 记录事务开始日志

   // transactionId = nextTransactionId++; //
}

void TransactionManager::commit() {
    
    std::unordered_set<std::string> affectedTables;
    for (const auto& op : undoStack) {
        affectedTables.insert(op.tableName);
    }

    Record record;
    for (const auto& table_name : affectedTables) {
        record.delete_by_flag(table_name);  // 删除 delete_flag == 1 的记录
    }
	

    active = false;
    autoCommit = lastAutoCommit.value();
    undoStack.clear();  // 提交事务时清空UNDO栈;将标识为1的数据真正删除
    LogManager::instance().logCommit();  // 记录事务开始日志
}

int TransactionManager::rollback() {
    // 遍历undoStack逆序回滚
    int rollback_count = 0;
    for (auto it = undoStack.rbegin(); it != undoStack.rend(); ++it) {
        const UndoOperation& op = *it;
        Record record;
        switch (op.type) {
        case DmlType::INSERT:
        {
            //将事务中插入的数据标记为待删除（1）
            int affectedRows = record.rollback_insert_by_rowid(op.tableName, op.rowId);
            rollback_count += affectedRows;
            break;
        }
        
        case DmlType::DELETE:
        {
			//将事务中删除的数据标记为未删除（0）
            int affectedRows = record.rollback_delete_by_rowid(op.tableName, op.rowId);
            rollback_count += affectedRows;
            break;
        }
        case DmlType::UPDATE:
        {
            int affectedRows= record.rollback_update_by_rowid(op.tableName, { { op.rowId, op.oldValues } });
			rollback_count += affectedRows;
            break;
        }
        }
    }

    std::unordered_set<std::string> affectedTables;
    for (const auto& op : undoStack) {
        affectedTables.insert(op.tableName);
    }

    Record record;
    for (const auto& table_name : affectedTables) {
        record.delete_by_flag(table_name);  // 删除 delete_flag == 1 的记录
    }

    undoStack.clear();  // 完成回滚，清空UNDO栈
    active = false;
    autoCommit = lastAutoCommit.value();
   // transactionId = 0;
    LogManager::instance().logRollback();  // 记录事务开始日志
	return rollback_count;  // 返回回滚的记录数
}

void TransactionManager::beginImplicitTransaction() {
    if (autoCommit && !active) {
        begin(); //因为复用，所以会把autoCommit设为false;
        autoCommit = true;//重新设为true
    }
    
}

void TransactionManager::commitImplicitTransaction() {
    if (autoCommit && active) {
        commit();
    }

}
#include <unordered_map> // 添加此行以确保 std::unordered_map 被正确包含

void TransactionManager::addUndo(DmlType type, const std::string& tableName, uint64_t rowId) {
    beginImplicitTransaction();

    if (!active) return;

    UndoOperation op;
    op.type = type;
    op.tableName = tableName;
    op.rowId = rowId;

    undoStack.push_back(op);

    //commitImplicitTransaction();
}

void TransactionManager::addUndo(DmlType type, const std::string& tableName, uint64_t rowId,
    const std::vector<std::pair<std::string, std::string>>& oldValues) {
    beginImplicitTransaction();
    if (!active) return;

    UndoOperation op;
    op.type = type;
    op.tableName = tableName;
    op.rowId = rowId;
    op.oldValues = oldValues;

    undoStack.push_back(op);
   // commitImplicitTransaction();
}


bool TransactionManager::isActive() const {
    return active;  // 返回事务是否正在进行
}
bool TransactionManager::isAutoCommit() const {
	return autoCommit;  // 返回是否启用自动提交
}// 判断是否启用自动提交

void TransactionManager::setAutoCommit(bool flag) {
	autoCommit = flag;
}  // 设置自动提交标志

//uint64_t TransactionManager::getTransactionId() const { 
//    return transactionId;
//}