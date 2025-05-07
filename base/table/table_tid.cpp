#include "table.h"
#include <iostream>
#include <ctime>
#include "parse/parse.h"
#include <cstring>
#include <iomanip>
#include"manager/dbManager.h"


void Table::saveIndex() {

	std::ofstream out(m_tid, std::ios::binary);
	if (!out.is_open()) {
		throw std::runtime_error("无法保存定义文件: " + m_tableName + " .tid ");
	}

	for (int i = 0; i < indexes.size(); ++i) {
		IndexBlock& index = indexes[i];

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
	indexes.clear();
	while (in.peek() != EOF) {
		IndexBlock index;
		in.read(reinterpret_cast<char*>(&index), sizeof(IndexBlock));
		if (in.gcount() < sizeof(IndexBlock)) break;
		indexes.push_back(index);
	}
	in.close();
}

void Table::createIndex(const IndexBlock& index) {
    // 1. 查找字段
    std::string fieldName1(index.field[0]);  // 第一个索引字段名
    std::string fieldName2(index.field[1]);  // 第二个索引字段名，如果有的话
    FieldBlock* field1 = getFieldByName(fieldName1);
    FieldBlock* field2 = nullptr;  // 如果是两个字段索引，查找第二个字段
    if (index.field_num == 2) {
        field2 = getFieldByName(fieldName2);
    }

    if (!field1) {
        throw std::runtime_error("字段 " + fieldName1 + " 不存在！");
    }
    if (index.field_num == 2 && !field2) {
        throw std::runtime_error("字段 " + fieldName2 + " 不存在！");
    }

    // 2. 创建 BTree 对象
    IndexBlock* indexCopy = new IndexBlock(index);
    std::unique_ptr<BTree> btree = std::make_unique<BTree>(indexCopy);

    // 3. 读取所有记录
    auto records = Record::read_records(m_tableName); // m_name 是表名
    for (const auto& [row_id, data] : records) {  // row_id 是 uint64_t 类型
        auto it1 = data.find(fieldName1);
        if (it1 == data.end()) continue;
        std::string field_value1 = it1->second;
        std::string field_value2;

        if (index.field_num == 2) {
            auto it2 = data.find(fieldName2);
            if (it2 == data.end()) continue;
            field_value2 = it2->second;
        }

        RecordPointer recordPtr;
        recordPtr.row_id = row_id;  // 这里的 row_id 是 uint64_t 类型

        if (index.field_num == 1) {
            btree->insert(field_value1, recordPtr);
        }
        else {
            btree->insert(field_value1 + "," + field_value2, recordPtr);
        }
    }

    // 4. 保存 B 树到文件
    btree->saveBTreeIndex();

    // 5. 添加到表中的 B 树索引列表
    m_btrees.push_back(std::move(btree));

    // 6. 提示信息
    std::cout << "为字段 " << fieldName1 << (index.field_num == 2 ? " 和 " + fieldName2 : "") << " 创建索引成功！" << std::endl;
}



void Table::addIndex(const IndexBlock& index){
	createIndex(index);
	// 创建索引
	IndexBlock newIndex = index;
	// 添加到索引列表
	indexes.push_back(newIndex);
	// 更新时间戳
	m_lastModifyTime = std::time(nullptr);
	// 保存到索引文件和元数据文件
	saveIndex();
	saveMetadataBinary();
}

void Table::dropIndex(const std::string indexName) {
    // 查找索引
    auto it = std::find_if(indexes.begin(), indexes.end(), [&](const IndexBlock& index) {
        return index.name == indexName;
    });

    if (it == indexes.end()) {
        throw std::runtime_error("索引 " + indexName + " 不存在！");
    }

    // 获取索引的文件路径
    std::string indexFilePath = it->index_file;

    // 删除索引文件 (.ix 文件)
    if (std::remove(indexFilePath.c_str()) == 0) {
        std::cout << "索引文件 " << indexFilePath << " 删除成功。" << std::endl;
    } else {
        std::perror(("索引文件 " + indexFilePath + " 删除失败。").c_str());
    }

    // 删除索引
    indexes.erase(it);

	// 删除 B 树对象
	auto btreeIt = std::find_if(m_btrees.begin(), m_btrees.end(), [&](const std::unique_ptr<BTree>& btree) {
		return btree->getIndexName() == indexName;
		});
	if (btreeIt != m_btrees.end()) {
		m_btrees.erase(btreeIt);
	}

    // 更新时间戳
    m_lastModifyTime = std::time(nullptr);

    // 保存到索引文件和元数据文件
    saveIndex();
    saveMetadataBinary();
}


void Table::updateIndex(const std::string indexName, const IndexBlock& updatedIndex)
{
	// 查找索引
	auto it = std::find_if(indexes.begin(), indexes.end(), [&](const IndexBlock& index) {
		return index.name == indexName;
		});
	if (it != indexes.end()) {
		*it = updatedIndex;
		saveIndex(); // 保存到索引文件
		m_lastModifyTime = std::time(nullptr); // 更新时间戳
		saveMetadataBinary();
	}
}