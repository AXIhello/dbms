#pragma once

#ifndef DATABASEBLOCK_H
#define DATABASEBLOCK_H

#include <ctime>  // for time_t

struct DatabaseBlock {
    char dbName[128];     // 数据库名称
    bool type;            // 0: 系统数据库；1: 用户数据库
    char filepath[256];   // 数据库数据文件夹全路径（保存记录文件与日志文件）
    time_t crtime;        // 创建时间
};

#endif // DATABASEBLOCK_H
