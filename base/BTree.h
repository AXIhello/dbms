#ifndef BTREE_H
#define BTREE_H

#include <vector>
#include <string>
#include <ctime>
#include "block/indexBlock.h"
#include "block/fieldBlock.h"

// B树节点
struct BTreeNode {
    std::vector<std::string> fields;      // 节点中的字段值（字符串形式）
    std::vector<BTreeNode*> children;     // 子节点
    bool isLeaf;                          // 是否是叶子节点

    BTreeNode(bool isLeaf = true);
};

// B树类
class BTree {
private:
    BTreeNode* root;             // B树的根节点
    int degree = 3;              // B树的度（每个节点最多存储的元素数量）
    const IndexBlock* m_index;  // B树索引块

public:
    BTree(const IndexBlock* indexBlock);       // 构造函数
    ~BTree();                                 // 析构函数

    void insert(const std::string& fieldValue); // 插入字段值到B树
    std::string* find(const std::string& fieldName); // 查找字段值
    void saveBTreeIndex() const;               // 保存B树索引到文件
    void serializeNode(BTreeNode* node, std::vector<BTreeNode*>& nodesToWrite) const; // 扁平化序列化节点
    void loadBTreeIndex();                     // 从文件加载B树索引

private:
    void splitChild(BTreeNode* parent, int index);  // 分裂子节点
    void insertNonFull(BTreeNode* node, const std::string& fieldValue); // 向非满节点插入字段值
    std::string* findInNode(BTreeNode* node, const std::string& fieldName); // 在节点中查找字段值
};

#endif // BTREE_H
