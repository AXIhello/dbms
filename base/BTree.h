#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <fstream>
#include <cstdio>

#include "base/block/indexBlock.h"
#include "base/block/fieldBlock.h"
#include "base/block/constraintBlock.h"
#include "base/block/tableBlock.h"


// 指向磁盘中记录的位置
struct RecordPointer {
    int blockId;     // 块号
    int offset;      // 块内偏移
};

// 字段值 + 记录指针
struct FieldPointer {
    std::string fieldValue;  // 索引字段的值
    RecordPointer recordPtr; // 指向磁盘记录的位置
};

class BTreeNode {
public:
    bool isLeaf;
    std::vector<FieldPointer> fields;
    std::vector<BTreeNode*> children;

    BTreeNode(bool isLeaf = true);
};

class BTree {
private:
    int degree = 3;        // B树的度
    BTreeNode* root;
    const IndexBlock* m_index;

    void splitChild(BTreeNode* parent, int index);
    void insertNonFull(BTreeNode* node, const FieldPointer& fieldPtr);
    FieldPointer* findInNode(BTreeNode* node, const std::string& fieldValue);
    void serializeNode(BTreeNode* node, std::vector<BTreeNode*>& nodes) const;
    void deleteNodes(BTreeNode* node);

    // 删除相关私有辅助函数
    void removeFromNode(BTreeNode* node, const std::string& fieldValue);
    void removeFromLeaf(BTreeNode* node, int idx);
    void removeFromNonLeaf(BTreeNode* node, int idx);
    std::string getPredecessor(BTreeNode* node, int idx);
    std::string getSuccessor(BTreeNode* node, int idx);
    void fill(BTreeNode* node, int idx);
    void borrowFromPrev(BTreeNode* node, int idx);
    void borrowFromNext(BTreeNode* node, int idx);
    void merge(BTreeNode* node, int idx);

public:
    BTree(const IndexBlock* indexBlock);
    ~BTree();

    void insert(const std::string& fieldValue, const RecordPointer& recordPtr);
    FieldPointer* find(const std::string& fieldValue);

    void remove(const std::string& fieldValue);  // 新增删除接口

    void saveBTreeIndex() const;
    void loadBTreeIndex();
};
