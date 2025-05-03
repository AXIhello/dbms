#include "BTree.h"

// B树构造
BTree::BTree(const IndexBlock* indexBlock) : m_index(indexBlock) {
    root = new BTreeNode(true, nullptr);  // 根节点是叶子节点，父节点为nullptr
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
    }
    else {
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
        BTreeNode* newRoot = new BTreeNode(false, nullptr);
        newRoot->children.push_back(r);
        splitChild(newRoot, 0);
        root = newRoot;
    }

    FieldPointer fp{ fieldValue, recordPtr };
    insertNonFull(root, fp);
}

#include <QDebug>  // 引入 QDebug

// 向非满节点插入
void BTree::insertNonFull(BTreeNode* node, const FieldPointer& fieldPtr) {
    int i = node->fields.size() - 1;

    // 打印进入 insertNonFull 函数的调试信息
    qDebug() << "Entering insertNonFull: Node isLeaf=" << node->isLeaf << ", Fields size=" << node->fields.size();

    // 如果是叶子节点
    if (node->isLeaf) {
        qDebug() << "Node is a leaf. Starting search for insert position...";

        while (i >= 0 && node->fields[i].fieldValue > fieldPtr.fieldValue) {
            qDebug() << "Comparing fieldValue: " << node->fields[i].fieldValue << " > " << fieldPtr.fieldValue;
            i--;
        }

        // 断言调试
        assert(node != nullptr);
        assert(i + 1 >= 0 && i + 1 <= node->fields.size());

        // 插入到 fields 中
        qDebug() << "Inserting field at index " << i + 1 << ": " << fieldPtr.fieldValue;
        node->fields.insert(node->fields.begin() + i + 1, fieldPtr);

        // 输出插入后的节点信息
        qDebug() << "After insertion, fields size: " << node->fields.size();
        for (const auto& field : node->fields) {
            qDebug() << "Field: " << field.fieldValue;
        }
    }
    else {
        // 如果是非叶子节点，进入递归处理子节点
        qDebug() << "Node is not a leaf. Searching for correct child to insert into...";

        while (i >= 0 && node->fields[i].fieldValue > fieldPtr.fieldValue) {
            qDebug() << "Comparing fieldValue: " << node->fields[i].fieldValue << " > " << fieldPtr.fieldValue;
            i--;
        }

        i++; // 移动到正确的子节点位置

        qDebug() << "Correct child index for insertion: " << i;

        // 检查子节点是否需要分裂
        if (node->children[i]->fields.size() == degree - 1) {
            qDebug() << "Child node " << i << " is full, splitting...";
            splitChild(node, i);

            // 如果需要，更新 i
            if (fieldPtr.fieldValue > node->fields[i].fieldValue) {
                qDebug() << "Field value greater than split value, incrementing i.";
                i++;
            }
        }

        // 递归调用插入子节点
        qDebug() << "Inserting into child node at index " << i;
        insertNonFull(node->children[i], fieldPtr);
    }
}

// 分裂子节点
void BTree::splitChild(BTreeNode* parent, int index) {
    BTreeNode* fullNode = parent->children[index];
    BTreeNode* newNode = new BTreeNode(fullNode->isLeaf, nullptr);

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

void BTree::loadBTreeIndex() {
    std::ifstream in(m_index->index_file, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("无法打开索引文件！");
    }

    deleteNodes(root);
    root = new BTreeNode(true, nullptr);  // 设置根节点为叶子节点，父节点为空

    size_t total;
    in.read(reinterpret_cast<char*>(&total), sizeof(size_t));
    std::vector<BTreeNode*> nodes(total);

    // 加载所有节点
    for (size_t i = 0; i < total; ++i) {
        nodes[i] = new BTreeNode(true, nullptr);  // 默认为叶子节点，父节点为空
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

    // 恢复父子关系
    for (size_t i = 0; i < total; ++i) {
        if (!nodes[i]->isLeaf) {
            for (size_t j = 0; j < nodes[i]->children.size(); ++j) {
                nodes[i]->children[j]->parent = nodes[i];  // 设置子节点的父节点
            }
        }
    }

    // 设置根节点
    root = nodes[0];

    in.close();
}
