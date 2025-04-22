#pragma once

#ifndef	INDEXBLOCK_H
#define INDEXBLOCK_H

struct IndexBlock {
	char name[128];			// 索引名称
	bool unique;			// 是否唯一索引，true为唯一索引，false为非唯一索引
	bool asc;				// 是否升序，true为升序，false为降序
	int field_num;			// 索引字段数(1或2）
	char field[128][2];		// 索引字段字段值，最多可以保存2个
	char record_file[256];	// 索引对应记录文件的路径
	char index_file[256];	// 索引数据文件的路径
};
#endif // INDEXBLOCK_H

#pragma once
