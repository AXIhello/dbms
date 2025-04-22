#include "BTree.h"
#include <cstdio>
#include <iostream>
#include <cstring>
#include <vector>
#include <fstream>
#include <stdexcept>

// B树节点构造函数
BTreeNode::BTreeNode(bool isLeaf) : isLeaf(isLeaf) {}

// B树构造函数
BTree::BTree(const IndexBlock* indexBlock) : m_index(indexBlock) {
    root = new BTreeNode(true);  // 默认创建一个叶子节点作为根节点
}

// B树析构函数
BTree::~BTree() {
    // 释放 B 树节点内存
    std::string filename = m_index->index_file;  // 从 IndexBlock 获取文件路径

    // 删除索引数据文件
    if (remove(filename.c_str()) == 0) {
        std::cout << "索引数据文件 " << filename << " 删除成功。" << std::endl;
    }
    else {
        std::perror(("索引数据文件 " + filename + " 删除失败。").c_str());
    }

    // 删除 B 树节点
    std::vector<BTreeNode*> nodesToDelete;
    serializeNode(root, nodesToDelete);
    for (auto* node : nodesToDelete) {
        delete node;
    }
}

// 插入字段到B树
void BTree::insert(const std::string& fieldValue) {
    BTreeNode* rootNode = root;

    // 如果根节点已满，则需要分裂
    if (rootNode->fields.size() == degree - 1) {
        BTreeNode* newRoot = new BTreeNode(false);
        newRoot->children.push_back(rootNode);
        splitChild(newRoot, 0);
        root = newRoot;
    }

    insertNonFull(root, fieldValue);
}

// 查找字段
std::string* BTree::find(const std::string& fieldValue) {
    return findInNode(root, fieldValue);
}

// 保存B树索引到文件
void BTree::saveBTreeIndex() const {
    std::string index_file = m_index->index_file;  // 从 IndexBlock 获取文件路径
    std::ofstream out(index_file, std::ios::binary);
    if (!out.is_open()) {
        throw std::runtime_error("无法保存索引数据文件！");
    }

    // 扁平化保存 B 树节点
    std::vector<BTreeNode*> nodesToWrite;
    serializeNode(root, nodesToWrite);

    // 写入所有节点
    for (const auto& node : nodesToWrite) {
        out.write(reinterpret_cast<const char*>(node), sizeof(BTreeNode));
    }

    out.close();
}

// 递归扁平化序列化节点
void BTree::serializeNode(BTreeNode* node, std::vector<BTreeNode*>& nodesToWrite) const {
    if (node == nullptr) return;

    nodesToWrite.push_back(node);

    // 递归序列化所有子节点
    if (!node->isLeaf) {
        for (auto* child : node->children) {
            serializeNode(child, nodesToWrite);
        }
    }
}

// 加载B树索引
void BTree::loadBTreeIndex() {
    std::ifstream in(m_index->index_file, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("无法打开 B 树索引文件！");
    }

    std::vector<BTreeNode> nodesToRead;

    // 读取所有节点
    while (in.peek() != EOF) {
        BTreeNode node;
        in.read(reinterpret_cast<char*>(&node), sizeof(BTreeNode));
        if (in.gcount() < sizeof(BTreeNode)) break;
        nodesToRead.push_back(node);
    }

    in.close();

    // 恢复树结构
    std::vector<BTreeNode*> nodePointers(nodesToRead.size());

    // 将每个节点的指针放到一个数组中
    for (size_t i = 0; i < nodesToRead.size(); ++i) {
        nodePointers[i] = &nodesToRead[i];
    }

    // 恢复子节点指针
    for (size_t i = 0; i < nodesToRead.size(); ++i) {
        BTreeNode& node = nodesToRead[i];

        // 子节点数量是 fields.size() + 1
        size_t numChildren = node.fields.size() + 1;

        for (size_t j = 0; j < numChildren; ++j) {
            int child_index = node.children[j] - &nodesToRead[0];  // 获取子节点的索引
            if (child_index >= 0 && child_index < nodesToRead.size()) {
                node.children[j] = nodePointers[child_index];  // 恢复指向正确子节点的指针
            }
        }
    }

    // 假设根节点是第一个节点
    root = nodePointers[0];
}

// 分裂子节点
void BTree::splitChild(BTreeNode* parent, int index) {
    BTreeNode* fullNode = parent->children[index];
    BTreeNode* newNode = new BTreeNode(fullNode->isLeaf);

    // 将中间元素提升到父节点
    int mid = degree / 2;
    parent->fields.push_back(fullNode->fields[mid]);
    parent->children.push_back(newNode);

    // 将后半部分的元素移动到新节点
    for (int i = mid + 1; i < fullNode->fields.size(); ++i) {
        newNode->fields.push_back(fullNode->fields[i]);
    }

    fullNode->fields.resize(mid);

    if (!fullNode->isLeaf) {
        for (int i = mid + 1; i < fullNode->children.size(); ++i) {
            newNode->children.push_back(fullNode->children[i]);
        }
        fullNode->children.resize(mid + 1);
    }
}

// 向非满节点插入字段
void BTree::insertNonFull(BTreeNode* node, const std::string& fieldValue) {
    int i = node->fields.size() - 1;

    if (node->isLeaf) {
        // 找到插入位置并插入
        while (i >= 0 && node->fields[i] > fieldValue) {
            i--;
        }
        node->fields.insert(node->fields.begin() + i + 1, fieldValue);
    }
    else {
        // 找到子节点并递归插入
        while (i >= 0 && node->fields[i] > fieldValue) {
            i--;
        }
        i++;
        if (node->children[i]->fields.size() == degree - 1) {
            splitChild(node, i);
            if (fieldValue > node->fields[i]) {
                i++;
            }
        }
        insertNonFull(node->children[i], fieldValue);
    }
}

// 在节点中查找字段
std::string* BTree::findInNode(BTreeNode* node, const std::string& fieldValue) {
    int i = 0;
    while (i < node->fields.size() && node->fields[i] < fieldValue) {
        i++;
    }
    if (i < node->fields.size() && node->fields[i] == fieldValue) {
        return &node->fields[i];
    }
    if (node->isLeaf) {
        return nullptr;
    }
    else {
        return findInNode(node->children[i], fieldValue);
    }
}
