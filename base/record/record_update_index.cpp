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

//void Record::updateIndexesAfterInsert() {
//    Table* table = dbManager::getInstance().get_current_database()->getTable(table_name);
//    const auto& indexes = table->getIndexes();  // 获取所有索引
//    std::vector<FieldBlock> fields = read_field_blocks(table_name);
//
//    // 获取刚插入记录的磁盘位置信息
//    RecordPointer lastInsertedPtr = get_last_inserted_record_pointer(); // 你需要实现这个方法
//
//    for (const auto& index : indexes) {
//        BTree btree(&index);
//        btree.loadBTreeIndex();
//
//        FieldPointer fieldPtr;
//        fieldPtr.recordPtr = lastInsertedPtr;
//
//        // 获取索引字段的值
//        for (int i = 0; i < index.field_num; ++i) {
//            const std::string& fieldName = index.field[i];
//            auto it = std::find(columns.begin(), columns.end(), fieldName);
//            if (it != columns.end()) {
//                size_t pos = std::distance(columns.begin(), it);
//                fieldPtr.fieldValue = values[pos];  // 简化，仅支持单字段索引
//                btree.insert(fieldPtr.fieldValue, fieldPtr.recordPtr);
//            }
//        }
//
//        btree.saveBTreeIndex();
//    }
//}
