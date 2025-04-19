#pragma once

#ifndef FIELDBLOCK_H
#define FIELDBLOCK_H

#include <ctime>  // for time_t

struct FieldBlock {
    int order;            // 字段顺序
    char name[128];       // 字段名称
    int type;             // 字段类型（如整型、字符串等） 
                          // 1: INT(4), 2: BOOL(1), 3: DOUBLE(2), 4: VARCHAR(最长255) 5:DATETIME (16)

    int param;            // 类型参数（如 VARCHAR/CHAR 的长度）
    time_t mtime;         // 最后修改时间
    int integrities;      // 完整性约束信息
    // 1: primary key ，2：foreign key , 3:NOT NULL , 4: UNIQUE, 5: DEFAULT 6.AUTO_INCREMENT 7.DEFAULT

};

#endif // FIELDBLOCK_H
