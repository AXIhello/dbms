#include "BTree.h"


void BTree::findRange(const std::string& low, const std::string& high, std::vector<FieldPointer>& result) {
    findRangeInNode(root, low, high, result);
}

void BTree::findRangeInNode(BTreeNode* node, const std::string& low, const std::string& high, std::vector<FieldPointer>& result) {
    int i = 0;
    while (i < node->fields.size() && node->fields[i].fieldValue < low) {
        i++;
    }

    while (i < node->fields.size() && node->fields[i].fieldValue <= high) {
        if (!node->isLeaf) {
            findRangeInNode(node->children[i], low, high, result);
        }

        result.push_back(node->fields[i]);
        i++;
    }

    if (!node->isLeaf && i < node->children.size()) {
        findRangeInNode(node->children[i], low, high, result);
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