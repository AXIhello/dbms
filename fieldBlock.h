#pragma once

#ifndef FIELDBLOCK_H
#define FIELDBLOCK_H

#include <ctime>  // for time_t

struct FieldBlock {
    int order;            // 字段顺序
    char name[128];       // 字段名称
    int type;             // 字段类型（如整型、字符串等） 
                          // 1: INT, 2: FLOAT, 3: VARCHAR, 4: BOOL 5:DATETIME
    int param;            // 类型参数（如 VARCHAR/CHAR 的长度）
    time_t mtime;         // 最后修改时间
    int integrities;      // 完整性约束信息（如是否非空、主键等）
};

#endif // FIELDBLOCK_H
