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

//指定字段创建 B 树索引
void Table::createIndex(const IndexBlock& index) {
    // 1. 查找字段
    std::string fieldName1(index.field[0]);  // 第一个索引字段名
    std::string fieldName2(index.field[1]);  // 第二个索引字段名，如果有的话

    FieldBlock* field1 = getFieldByName(fieldName1);
    FieldBlock* field2 = nullptr;

    // 如果索引字段是两个字段，查找第二个字段
    if (index.field_num == 2) {
        field2 = getFieldByName(fieldName2);
    }

    if (!field1) {
        throw std::runtime_error("字段 " + fieldName1 + " 不存在！");
    }

    // 如果是两个字段索引，确保第二个字段存在
    if (index.field_num == 2 && !field2) {
        throw std::runtime_error("字段 " + fieldName2 + " 不存在！");
    }

    // 2. 创建 BTree 对象
    BTree* btree = new BTree(&index);

    // 3. 将字段值插入 B 树（假设我们将所有记录的字段插入到 B 树）
    std::ifstream file(m_trd, std::ios::binary); // 假设数据文件是以二进制方式存储
    if (!file.is_open()) {
        throw std::runtime_error("无法打开表记录文件以插入BTree！");
    }

    // 遍历所有记录
    while (file.peek() != EOF) {
        std::vector<std::string> record_values;
        std::string field_value1;
        std::string field_value2;

        for (size_t i = 0; i < m_fields.size(); ++i) {
            FieldBlock& current_field = m_fields[i];
            bool is_null = false;
            file.read(reinterpret_cast<char*>(&is_null), sizeof(bool));

            if (is_null) {
                record_values.push_back("NULL");
                continue;
            }

            std::string value;
            switch (current_field.type) {
            case 1: { // INTEGER
                int int_val;
                file.read(reinterpret_cast<char*>(&int_val), sizeof(int));
                value = std::to_string(int_val);
                break;
            }
            case 2: { // DOUBLE
                double double_val;
                file.read(reinterpret_cast<char*>(&double_val), sizeof(double));
                value = std::to_string(double_val);
                break;
            }
            case 3: { // VARCHAR
            case 4: { // CHAR
                char* buffer = new char[current_field.param];
                file.read(buffer, current_field.param);
                value = std::string(buffer, current_field.param);
                delete[] buffer;
                break;
            }
            case 5: { // DATETIME
                std::time_t timestamp;
                file.read(reinterpret_cast<char*>(&timestamp), sizeof(std::time_t));
                value = std::to_string(timestamp); // 或者转化为日期字符串
                break;
            }
            default:
                throw std::runtime_error("未知字段类型");
            }
                  record_values.push_back(value);

                  // 如果是我们要为其创建索引的字段，将字段值存储到相应变量
                  if (current_field.name == fieldName1) {
                      field_value1 = value;  // 存储第一个字段的值
                  }
                  else if (field2 && current_field.name == fieldName2) {
                      field_value2 = value;  // 存储第二个字段的值
                  }
            }

            // 根据字段数量，将字段值插入到 B 树中
            if (index.field_num == 1) {
                // 如果只有一个字段索引，插入第一个字段的值
                btree->insert(field_value1);
            }
            else if (index.field_num == 2) {
                // 如果是两个字段索引，插入两个字段的组合值
                btree->insert(field_value1 + "," + field_value2);  // 将两个字段的值组合为一个字符串插入
            }
        }

        // 4. 保存 B 树到文件
        btree->saveBTreeIndex();

        // 5. 将索引信息保存到索引文件
        std::cout << "为字段 " << fieldName1 << (index.field_num == 2 ? " 和 " + fieldName2 : "") << " 创建索引成功！" << std::endl;

        // 释放内存
        delete btree;
    }

}

void Table::addIndex(const IndexBlock& index){
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