#include "Record.h"
#include "parse/parse.h"
#include "ui/output.h"

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <regex>
#include <map>
#include <set>

void Record::updateIndexesAfterInsert(const std::string& table_name) {
    Table* table = dbManager::getInstance().get_current_database()->getTable(table_name);
    RecordPointer recordPtr = get_last_inserted_record_pointer(table_name);
    const auto& indexes = table->getIndexes();

    for (const auto& index : indexes) {
        if (index.field_num != 1) continue;

        const std::string& fieldName = index.field[0];
        auto it = std::find(columns.begin(), columns.end(), fieldName);
        if (it == columns.end()) continue;

        std::string fieldValue = values[it - columns.begin()];
        BTree* btree = table->getBTreeByIndexName(index.name);
        if (btree) {
            btree->insert(fieldValue, recordPtr);
            btree->saveBTreeIndex();
        }
    }
}

void Record::updateIndexesAfterDelete(const std::string& table_name, const std::vector<std::string>& deletedValues, const RecordPointer& recordPtr) {
    Table* table = dbManager::getInstance().get_current_database()->getTable(table_name);
    const auto& indexes = table->getIndexes();

    for (const auto& index : indexes) {
        if (index.field_num != 1) continue;

        const std::string& fieldName = index.field[0];
        auto it = std::find(columns.begin(), columns.end(), fieldName);
        if (it == columns.end()) continue;

        std::string fieldValue = deletedValues[it - columns.begin()];
        BTree* btree = table->getBTreeByIndexName(index.name);
        if (btree) {
            btree->remove(fieldValue);
            btree->saveBTreeIndex();
        }
    }
}

void Record::updateIndexesAfterUpdate(const std::string& table_name, const std::vector<std::string>& oldValues, const std::vector<std::string>& newValues, const RecordPointer& recordPtr) {
    Table* table = dbManager::getInstance().get_current_database()->getTable(table_name);
    const auto& indexes = table->getIndexes();

    for (const auto& index : indexes) {
        if (index.field_num != 1) continue;

        const std::string& fieldName = index.field[0];
        auto it = std::find(columns.begin(), columns.end(), fieldName);
        if (it == columns.end()) continue;

        size_t pos = it - columns.begin();
        std::string oldVal = oldValues[pos];
        std::string newVal = newValues[pos];

        if (oldVal != newVal) {
            BTree* btree = table->getBTreeByIndexName(index.name);
            if (btree) {
                btree->remove(oldVal);
                btree->insert(newVal, recordPtr);
                btree->saveBTreeIndex();
            }
        }
    }
}

RecordPointer Record::get_last_inserted_record_pointer(const std::string& table_name) {
    Table* table = dbManager::getInstance().get_current_database()->getTable(table_name);
    const auto& allRecords = read_records(table_name); // 返回上面的 vector<pair<row_id, map>> 类型

    if (allRecords.empty()) {
        throw std::runtime_error("No records exist in the table.");
    }

    RecordPointer ptr;
    ptr.row_id = allRecords.back().first;  // 获取最后插入的 row_id
    return ptr;
}

