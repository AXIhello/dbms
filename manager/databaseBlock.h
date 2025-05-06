#pragma once

#ifndef DATABASEBLOCK_H
#define DATABASEBLOCK_H

#include <ctime>  // for time_t

// 关键点：强制按1字节对齐，避免结构体字段错位
#pragma pack(push, 1)
struct DatabaseBlock {
    char dbName[128];     // 数据库名称
    bool type;            // 0: 系统数据库；1: 用户数据库
    char filepath[256];   // 数据库数据文件夹全路径（保存记录文件与日志文件）
    time_t crtime;        // 创建时间
    char abledUsername[128];     // 能使用该数据库的用户名
};
#pragma pack(pop)

#endif // DATABASEBLOCK_H
