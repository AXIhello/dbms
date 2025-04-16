#pragma once

#ifndef CONSTRAINTBLOCK_H
#define CONSTRAINTBLOCK_H

struct ConstraintBlock{
	char name[128];		// 约束名称
	char field[128];	// 约束字段
	int type;			// 约束类型（如主键、外键等）
						// 1: 主键, 2: 外键, 3: CHECK约束, 4: UNIQUE约束, 5: NOT NULL约束, 6: DEFAULT约束, 7: 自增
	char param[256];	// 约束参数（如外键引用的表名）
};
#endif // CONSTRAINTBLOCK_H

