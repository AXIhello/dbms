#ifndef BTREE_H
#define BTREE_H

#include <vector>
#include <string>
#include <ctime>
#include "indexBlock.h"
#include "fieldBlock.h"

// B树节点
struct BTreeNode {
    std::vector<FieldBlock> fields;      // 节点中的字段
    std::vector<BTreeNode*> children;    // 子节点
    bool isLeaf;                         // 是否是叶子节点

    BTreeNode(bool isLeaf = true);
};

// B树类
class BTree {
private:
    BTreeNode* root;          // B树的根节点
    int degree = 3;           // B树的度（每个节点最多存储的元素数量）
    IndexBlock* m_index; // B树索引块

public:
    BTree(IndexBlock* indexBlock);        // 构造函数
	~BTree();		                      // 析构函数

    void insert(const FieldBlock& field); // 插入字段到B树
    FieldBlock* find(const std::string& name); // 查找字段
	void saveBTreeIndex() const; // 保存B树索引到文件
    // 递归扁平化序列化节点
	void serializeNode(BTreeNode* node, std::vector<BTreeNode*>& nodesToWrite) const;
	void loadBTreeIndex(); // 从文件加载B树索引

private:
    void splitChild(BTreeNode* parent, int index); // 分裂子节点
    void insertNonFull(BTreeNode* node, const FieldBlock& field); // 向非满节点插入字段
    FieldBlock* findInNode(BTreeNode* node, const std::string& name); // 在节点中查找字段
};

#endif // BTREE_H
