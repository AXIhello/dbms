#include "table.h"
#include <iostream>
#include <ctime>
#include "manager/parse.h"
#include <cstring>
#include <iomanip>
#include"manager/dbManager.h"


void Table::saveIndex() {

	std::ofstream out(m_tid, std::ios::binary);
	if (!out.is_open()) {
		throw std::runtime_error("无法保存定义文件: " + m_tableName + " .tid ");
	}

	for (int i = 0; i < m_indexes.size(); ++i) {
		IndexBlock& index = m_indexes[i];

		// 将当前字段块写入文件
		out.write(reinterpret_cast<const char*>(&index), sizeof(IndexBlock));
	}

	out.close();
}

void Table::loadIndex() {

	std::ifstream in(m_tid, std::ios::binary);
	if (!in.is_open()) {
		throw std::runtime_error("无法打开索引文件: " + m_tid);
	}
	m_indexes.clear();
	while (in.peek() != EOF) {
		IndexBlock index;
		in.read(reinterpret_cast<char*>(&index), sizeof(IndexBlock));
		if (in.gcount() < sizeof(IndexBlock)) break;
		m_indexes.push_back(index);
	}
	in.close();
}

void Table::createIndex(const IndexBlock& index)
{
}

// 为指定字段创建 B 树索引
//void Table::createIndex(const IndexBlock& index) {
//    // 1. 查找字段
//	std::string fieldName = index.
//        //先查找是否有该字段
//        auto colNames = getColNames();
//    if (std::find(colNames.begin(), colNames.end(), fieldName) == colNames.end()) {
//        throw std::runtime_error("字段 '" + fieldName + "' 不存在。");
//    }
//    // 检查完整性约束；只有 NOT NULL / DEFAULT / 无约束 情况下才能删
//
//    FieldBlock* field = findFieldByName(fieldName);
//
//    for (const auto& f : m_fields) {
//        if (f.name == fieldName) {
//        }
//    }
//    if (!field) {
//        throw std::runtime_error("字段 " + fieldName + " 不存在！");
//    }
//
//    // 2. 创建 BTree 对象
//    BTree* btree = new BTree(m_index);
//
//    // 3. 将字段值插入 B 树（假设我们将所有记录的字段插入到 B 树）
//    for (const auto& record : m_records) {
//        // 假设每个记录都有一个 FieldBlock 类型的字段
//        FieldBlock fieldBlock;
//        fieldBlock.name = field->name;
//        fieldBlock.value = record.getFieldValue(fieldName);
//
//        // 将字段插入到 B 树中
//        btree->insert(fieldBlock);
//    }
//
//    // 4. 保存 B 树到文件
//    btree->saveBTreeIndex();
//	// 5. 将索引信息保存到索引文件
//
//    std::cout << "为字段 " << fieldName << " 创建索引成功！" << std::endl;
//}

void Table::addIndex(const IndexBlock& index)
{
	createIndex(index);
	// 创建索引
	IndexBlock newIndex = index;
	// 添加到索引列表
	m_indexes.push_back(newIndex);
	// 更新时间戳
	m_lastModifyTime = std::time(nullptr);
	// 保存到索引文件和元数据文件
	saveIndex();
	saveMetadataBinary();
}

void Table::dropIndex(const std::string indexName)
{
	auto it = std::remove_if(m_indexes.begin(), m_indexes.end(), [&](const IndexBlock& index) {
		return index.name == indexName;
		});
	m_indexes.erase(it, m_indexes.end());
	saveIndex();
	saveMetadataBinary();
}

void Table::updateIndex(const std::string indexName, const IndexBlock& updatedIndex)
{
	// 查找索引
	auto it = std::find_if(m_indexes.begin(), m_indexes.end(), [&](const IndexBlock& index) {
		return index.name == indexName;
		});
	if (it != m_indexes.end()) {
		*it = updatedIndex;
		saveIndex(); // 保存到索引文件
		m_lastModifyTime = std::time(nullptr); // 更新时间戳
		saveMetadataBinary();
	}
}