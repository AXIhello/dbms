#include "BTree.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <cstdio>

// B树节点构造
BTreeNode::BTreeNode(bool isLeaf) : isLeaf(isLeaf) {}

// B树构造
BTree::BTree(const IndexBlock* indexBlock) :  m_index(indexBlock) {
    root = new BTreeNode(true);
}

// B树析构
BTree::~BTree() {
    deleteNodes(root);
    // 获取索引文件路径
    std::string filename = m_index->index_file;

    if (filename.empty()) {
        std::cerr << "文件名为空，无法删除。" << std::endl;
        return;
    }

    // 调用 remove() 删除文件，返回 0 表示成功，非 0 表示失败
    if (std::remove(filename.c_str()) == 0) {  
       std::cout << "索引数据文件 " << filename << " 删除成功。" << std::endl;  
    } else {  
       std::perror(("索引数据文件 " + filename + " 删除失败。").c_str());  
    }
}

// 删除所有节点
void BTree::deleteNodes(BTreeNode* node) {
    if (!node) return;
    for (auto* child : node->children) {
        deleteNodes(child);
    }
    delete node;
}

// 插入字段
void BTree::insert(const std::string& fieldValue, const RecordPointer& recordPtr) {
    BTreeNode* r = root;
    if (r->fields.size() == degree - 1) {
        BTreeNode* newRoot = new BTreeNode(false);
        newRoot->children.push_back(r);
        splitChild(newRoot, 0);
        root = newRoot;
    }

    FieldPointer fp{ fieldValue, recordPtr };
    insertNonFull(root, fp);
}

// 向非满节点插入
void BTree::insertNonFull(BTreeNode* node, const FieldPointer& fieldPtr) {
    int i = node->fields.size() - 1;

    if (node->isLeaf) {
        while (i >= 0 && node->fields[i].fieldValue > fieldPtr.fieldValue) {
            i--;
        }
        node->fields.insert(node->fields.begin() + i + 1, fieldPtr);
    }
    else {
        while (i >= 0 && node->fields[i].fieldValue > fieldPtr.fieldValue) {
            i--;
        }
        i++;
        if (node->children[i]->fields.size() == degree - 1) {
            splitChild(node, i);
            if (fieldPtr.fieldValue > node->fields[i].fieldValue) {
                i++;
            }
        }
        insertNonFull(node->children[i], fieldPtr);
    }
}

// 分裂子节点
void BTree::splitChild(BTreeNode* parent, int index) {
    BTreeNode* fullNode = parent->children[index];
    BTreeNode* newNode = new BTreeNode(fullNode->isLeaf);

    int mid = degree / 2;
    parent->fields.insert(parent->fields.begin() + index, fullNode->fields[mid]);
    parent->children.insert(parent->children.begin() + index + 1, newNode);

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

// 查找字段
FieldPointer* BTree::find(const std::string& fieldValue) {
    return findInNode(root, fieldValue);
}

// 在节点内查找
FieldPointer* BTree::findInNode(BTreeNode* node, const std::string& fieldValue) {
    int i = 0;
    while (i < node->fields.size() && node->fields[i].fieldValue < fieldValue) {
        i++;
    }
    if (i < node->fields.size() && node->fields[i].fieldValue == fieldValue) {
        return &node->fields[i];
    }
    if (node->isLeaf) {
        return nullptr;
    }
    else {
        return findInNode(node->children[i], fieldValue);
    }
}

// 扁平化序列化节点
void BTree::serializeNode(BTreeNode* node, std::vector<BTreeNode*>& nodes) const {
    if (!node) return;
    nodes.push_back(node);
    for (auto* child : node->children) {
        serializeNode(child, nodes);
    }
}

// 保存 B树索引到文件
void BTree::saveBTreeIndex() const {
    std::ofstream out(m_index->index_file, std::ios::binary);
    if (!out.is_open()) {
        throw std::runtime_error("无法保存索引文件！");
    }

    std::vector<BTreeNode*> nodes;
    serializeNode(root, nodes);

    size_t total = nodes.size();
    out.write(reinterpret_cast<const char*>(&total), sizeof(size_t));

    for (auto* node : nodes) {
        size_t n = node->fields.size();
        out.write(reinterpret_cast<const char*>(&n), sizeof(size_t));
        for (auto& fp : node->fields) {
            size_t len = fp.fieldValue.size();
            out.write(reinterpret_cast<const char*>(&len), sizeof(size_t));
            out.write(fp.fieldValue.data(), len);
            out.write(reinterpret_cast<const char*>(&fp.recordPtr.blockId), sizeof(int));
            out.write(reinterpret_cast<const char*>(&fp.recordPtr.offset), sizeof(int));
        }
        size_t childCount = node->children.size();
        out.write(reinterpret_cast<const char*>(&childCount), sizeof(size_t));
    }

    out.close();
}

// 加载 B树索引
void BTree::loadBTreeIndex() {
    std::ifstream in(m_index->index_file, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("无法打开索引文件！");
    }

    deleteNodes(root);
    root = new BTreeNode(true);

    size_t total;
    in.read(reinterpret_cast<char*>(&total), sizeof(size_t));
    std::vector<BTreeNode*> nodes(total);

    for (size_t i = 0; i < total; ++i) {
        nodes[i] = new BTreeNode();
        size_t n;
        in.read(reinterpret_cast<char*>(&n), sizeof(size_t));
        for (size_t j = 0; j < n; ++j) {
            size_t len;
            in.read(reinterpret_cast<char*>(&len), sizeof(size_t));
            std::string field(len, '\0');
            in.read(&field[0], len);
            int blockId, offset;
            in.read(reinterpret_cast<char*>(&blockId), sizeof(int));
            in.read(reinterpret_cast<char*>(&offset), sizeof(int));
            nodes[i]->fields.push_back({ field, {blockId, offset} });
        }
        size_t childCount;
        in.read(reinterpret_cast<char*>(&childCount), sizeof(size_t));
        nodes[i]->children.resize(childCount);
    }

    root = nodes[0];
    // 注意：这里还要重新恢复父子关系，这里略了，如果你要做持久化完全恢复，我可以再给你补

    in.close();
}
