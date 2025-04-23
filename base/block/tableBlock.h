#pragma once

#ifndef TABLEBLOCK_H
#define TABLEBLOCK_H

#include <ctime>   // for std::time_t

struct TableBlock {
    char name[128];     // 表格名称
    int record_num;     // 当前记录数
    int field_num;      // 字段数

    char tdf[256];      // 表结构文件路径
    char tic[256];      // 完整性约束路径
    char trd[256];      // 记录文件路径
    char tid[256];      // 索引路径

    std::time_t crtime; // 创建时间
    std::time_t mtime;  // 最后修改时间
};

#endif // TABLEBLOCK_H
