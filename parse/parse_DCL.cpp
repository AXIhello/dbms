#include "parse/parse.h"
void Parse::handleUseDatabase(const std::smatch& m) {
    std::string dbName = m[1];

    try {
        // 尝试切换数据库
        dbManager::getInstance().useDatabase(dbName);
        // 成功后输出信息
        Output::printMessage(outputEdit, "已成功切换到数据库 '" + QString::fromStdString(dbName) + "'.");

    }
    catch (const std::exception& e) {
        // 捕获异常并输出错误信息
        Output::printError(outputEdit, e.what());
    }
}
