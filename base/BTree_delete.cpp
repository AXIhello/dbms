#include "BTree.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <stdexcept>

//入口函数
void BTree::remove(const std::string& fieldValue) {
    if (!root) return;

    removeFromNode(root, fieldValue);

    // 如果根节点没有键了，处理特殊情况
    if (root->fields.empty()) {
        BTreeNode* oldRoot = root;
        if (root->isLeaf) {
            root = nullptr;
        }
        else {
            root = root->children[0];
        }
        delete oldRoot;
    }
}

// 递归删除逻辑
void BTree::removeFromNode(BTreeNode* node, const std::string& fieldValue) {
    int idx = 0;
    // 用 fieldValue 字段比较
    while (idx < node->fields.size() &&
        node->fields[idx].fieldValue < fieldValue) {
        idx++;
    }

    // 找到要删除的 key
    if (idx < node->fields.size() &&
        node->fields[idx].fieldValue == fieldValue) {
        if (node->isLeaf) {
            removeFromLeaf(node, idx);
        }
        else {
            removeFromNonLeaf(node, idx);
        }
    }
    else {
        if (node->isLeaf) {
            // 没找到，直接返回
            return;
        }

        bool atLastChild = (idx == node->fields.size());

        // 确保子节点字段够多
        if (node->children[idx]->fields.size() < (degree + 1) / 2) {
            fill(node, idx);
        }

        // 递归到正确的子节点
        if (atLastChild && idx > node->fields.size()) {
            removeFromNode(node->children[idx - 1], fieldValue);
        }
        else {
            removeFromNode(node->children[idx], fieldValue);
        }
    }
}

// 叶子节点删除：直接删掉那个 FieldPointer
void BTree::removeFromLeaf(BTreeNode* node, int idx) {
    node->fields.erase(node->fields.begin() + idx);
}

// 非叶子节点删除
void BTree::removeFromNonLeaf(BTreeNode* node, int idx) {
    // 取出要删除的原 key
    std::string key = node->fields[idx].fieldValue;

    // (1) 左子节点有足够多的字段，用前驱替换
    if (node->children[idx]->fields.size() >= (degree + 1) / 2) {
        std::string pred = getPredecessor(node, idx);
        node->fields[idx].fieldValue = pred;
        removeFromNode(node->children[idx], pred);
    }
    // (2) 右子节点有足够多的字段，用后继替换
    else if (node->children[idx + 1]->fields.size() >= (degree + 1) / 2) {
        std::string succ = getSuccessor(node, idx);
        node->fields[idx].fieldValue = succ;
        removeFromNode(node->children[idx + 1], succ);
    }
    // (3) 两边都不够，则合并
    else {
        merge(node, idx);
        removeFromNode(node->children[idx], key);
    }
}

// 获取前驱：一直走到最右叶子，取 fieldValue
std::string BTree::getPredecessor(BTreeNode* node, int idx) {
    BTreeNode* cur = node->children[idx];
    while (!cur->isLeaf) {
        cur = cur->children.back();
    }
    return cur->fields.back().fieldValue;
}

// 获取后继：一直走到最左叶子，取 fieldValue
std::string BTree::getSuccessor(BTreeNode* node, int idx) {
    BTreeNode* cur = node->children[idx + 1];
    while (!cur->isLeaf) {
        cur = cur->children.front();
    }
    return cur->fields.front().fieldValue;
}

//填充子节点
void BTree::fill(BTreeNode* node, int idx) {
    if (idx != 0 && node->children[idx - 1]->fields.size() >= (degree + 1) / 2) {
        borrowFromPrev(node, idx);
    }
    else if (idx != node->fields.size() && node->children[idx + 1]->fields.size() >= (degree + 1) / 2) {
        borrowFromNext(node, idx);
    }
    else {
        if (idx != node->fields.size()) {
            merge(node, idx);
        }
        else {
            merge(node, idx - 1);
        }
    }
}

//借用前一个或后一个兄弟节点
void BTree::borrowFromPrev(BTreeNode* node, int idx) {
    BTreeNode* child = node->children[idx];
    BTreeNode* sibling = node->children[idx - 1];

    child->fields.insert(child->fields.begin(), node->fields[idx - 1]);

    if (!child->isLeaf) {
        child->children.insert(child->children.begin(), sibling->children.back());
        sibling->children.pop_back();
    }

    node->fields[idx - 1] = sibling->fields.back();
    sibling->fields.pop_back();
}

void BTree::borrowFromNext(BTreeNode* node, int idx) {
    BTreeNode* child = node->children[idx];
    BTreeNode* sibling = node->children[idx + 1];

    child->fields.push_back(node->fields[idx]);

    if (!child->isLeaf) {
        child->children.push_back(sibling->children.front());
        sibling->children.erase(sibling->children.begin());
    }

    node->fields[idx] = sibling->fields.front();
    sibling->fields.erase(sibling->fields.begin());
}

//合并兄弟节点
void BTree::merge(BTreeNode* node, int idx) {
    BTreeNode* child = node->children[idx];
    BTreeNode* sibling = node->children[idx + 1];

    child->fields.push_back(node->fields[idx]);

    for (auto& f : sibling->fields) {
        child->fields.push_back(f);
    }

    if (!child->isLeaf) {
        for (auto* c : sibling->children) {
            child->children.push_back(c);
        }
    }

    node->fields.erase(node->fields.begin() + idx);
    node->children.erase(node->children.begin() + idx + 1);

    delete sibling;
}
