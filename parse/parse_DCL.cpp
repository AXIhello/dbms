#include "parse.h"
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

void Parse::handleCreateUser(const std::smatch& m) {
    std::string username = m[1].str();
    std::string password = m[2].str();
    if (user::createUser(username, password)) {
        Output::printMessage(outputEdit, "用户 '" + QString::fromStdString(username) + "' 创建成功。");
    }
    else {
        Output::printMessage(outputEdit, "用户 '" + QString::fromStdString(username) + "' 已存在。");
    }
}

void Parse::handleGrantPermission(const std::smatch& m) {
    std::string permission = m[1].str();
    std::string object = m[2].str();
    std::string username = m[3].str();

    std::string dbName, tableName;
    size_t dotPos = object.find('.');
    if (dotPos != std::string::npos) {
        dbName = object.substr(0, dotPos);
        tableName = object.substr(dotPos + 1);
    }
    else {
        dbName = object;
    }

    if (user::grantPermission(username, permission, dbName, tableName)) {
        Output::printMessage(outputEdit, "已授予 '" + QString::fromStdString(permission) +
            "' 权限给用户 '" + QString::fromStdString(username) +
            "' 在 '" + QString::fromStdString(object) + "' 上。");
    }
    else {
        Output::printMessage(outputEdit, "授权失败，用户 '" + QString::fromStdString(username) + "' 不存在。");
    }
}

void Parse::handleRevokePermission(const std::smatch& m) {
    std::string permission = m[1].str();
    std::string username = m[2].str();
    if (user::revokePermission(username, permission)) {
        Output::printMessage(outputEdit, "已从用户 '" + QString::fromStdString(username) + "' 收回 '" + QString::fromStdString(permission) + "' 权限。");
    }
    else {
        Output::printMessage(outputEdit, "收回失败，用户 '" + QString::fromStdString(username) + "' 不存在。");
    }
}

void Parse::handleShowUsers(const std::smatch& m) {
    // 获取所有用户
    std::vector<user::User> users = user::loadUsers();

    // 判断是否为空
    if (users.empty()) {
        Output::printMessage(outputEdit, "没有找到用户。");
    }
    else {
        Output::printMessage(outputEdit, "用户列表:");
        for (const auto& user : users) {
            // 确保显示正确的用户名（去除可能的垃圾数据）
            std::string username(user.username, strnlen(user.username, sizeof(user.username)));
            Output::printMessage(outputEdit, QString::fromStdString(user.username));
        }
    }
}
