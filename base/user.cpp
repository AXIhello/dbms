#include <filesystem>  
#include "user.h"
#include "manager/dbManager.h"
#include <ui/output.h>

user::User user::currentUser = {};  // 初始化
QTextEdit* user::outputEdit = nullptr;
// 读取用户列表
std::vector<user::User> user::loadUsers() {
    std::vector<User> users;
    std::ifstream file("users.dat", std::ios::binary);
    if (!file) {
        // 文件不存在，创建空文件
        std::ofstream newFile("users.dat",std::ios::binary | std::ios::trunc);
        newFile.close();
        return users;
    }

    // 获取文件大小以验证完整性
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();

    if (fileSize == 0) {
        file.close();
        return users;  // 空文件
    }
    

    if (fileSize % sizeof(User) != 0) {
        Output::printMessage(outputEdit, QString::fromStdString("警告：用户文件大小异常，可能已损坏"));
        file.close();
        return users;
    }
    file.seekg(0, std::ios::beg);

    User u = {};
    while (file.read(reinterpret_cast<char*>(&u), sizeof(User))) {
        // 确保字符串正确终止
        u.username[sizeof(u.username) - 1] = '\0';
        u.password[sizeof(u.password) - 1] = '\0';
        u.permissions[sizeof(u.permissions) - 1] = '\0';
       
        users.push_back(u);
    }
    // 检查是否因错误而提前结束读取
    if (file.bad() && !file.eof()) {
        Output::printMessage(outputEdit, QString::fromStdString("读取用户文件时发生错误"));
    }
    file.close();
    return users;
}

// 检查用户是否存在
bool user::userExists(const std::string& username) {
    std::ifstream file("users.dat", std::ios::binary);
    if (!file) {
        return false;  // 文件不存在，用户肯定不存在
    }

    User u;
    while (file.read(reinterpret_cast<char*>(&u), sizeof(User))) {
        // 确保字符串正确终止
        u.username[sizeof(u.username) - 1] = '\0';

        // 使用安全的字符串比较
        if (strncmp(u.username, username.c_str(), sizeof(u.username)) == 0) {
            file.close();
            return true;
        }
    }
    file.close();
    return false;
}

// 创建 sysdba 用户
void user::createSysDBA() {  // 添加 user:: 作用域
    if (userExists("sys")) {
        Output::printMessage(outputEdit, "sys 用户已存在");
        qDebug() << "sys 用户已存在";
        return;
    }

    User sysdba;
    strcpy_s(sysdba.username, sizeof(sysdba.username), "sys");
    strcpy_s(sysdba.password, sizeof(sysdba.password), "1");
    strcpy_s(sysdba.permissions, sizeof(sysdba.permissions), "conn|resource");

    std::ofstream file("users.dat", std::ios::binary | std::ios::app);

    if (!file) {
        Output::printMessage(outputEdit, "无法打开文件创建用户");
        return;
    }
    file.write(reinterpret_cast<char*>(&sysdba), sizeof(User));
    file.close();

    qDebug() << "sysdba 用户创建成功，并赋予 conn 和 resource 权限";
    Output::printMessage(outputEdit, "sysdba 用户创建成功，并赋予 resource 权限");

    // 读取并输出文件内容，确认是否写入成功
    std::ifstream readFile("users.dat", std::ios::binary);
    if (!readFile) {
        Output::printMessage(outputEdit, "无法打开文件读取用户");
        return;
    }

    User tempUser;
    while (readFile.read(reinterpret_cast<char*>(&tempUser), sizeof(User))) {
        qDebug() << "文件中的用户信息：";
        qDebug() << "用户名:" << tempUser.username;
        qDebug() << "密码:" << tempUser.password;
        qDebug() << "权限:" << tempUser.permissions;
    }
    readFile.close();
}

//user::User user::currentUser = {};
void user::setCurrentUser(const User& u) {
	//currentUser = u;
    memset(&currentUser, 0, sizeof(User));  // 清空所有字段，防止残留
    std::memcpy(&currentUser, &u, sizeof(User));  // 安全复制结构体
}

const user::User& user::getCurrentUser() {
    /*std::cout << "Current User: " << currentUser.username << "\n";
    std::cout << "Permissions: " << currentUser.permissions << "\n";*/
    return currentUser;
}

bool user::hasPermission(const std::string& requiredPerm, const std::string& dbName, const std::string& tableName)
{
    std::string permissions(currentUser.permissions);
    std::stringstream ss(permissions);
    std::string permEntry;

    std::string target1 = requiredPerm + ":" + dbName;  // 库级权限
    std::string target2 = target1 + "." + tableName;     // 表级权限（可选）

    while (std::getline(ss, permEntry, '|')) {
        // 若指定表，则查表级权限
        if (!tableName.empty()) {
            if (permEntry == target2) return true;
        }
        else {
            // 仅判断库级权限
            if (permEntry == target1) return true;
        }
    }
    return false;
}



// 创建用户
bool user::createUser(const std::string& username, const std::string& password) {
    user::outputEdit = outputEdit;
    if (userExists(username)) {
        Output::printMessage(outputEdit, QString::fromStdString("用户 " + username + " 已存在，创建失败"));
        return false;
    }

    User newUser = {};
    // 使用 std::string 来确保字符串正确管理
    strncpy_s(newUser.username, username.c_str(), sizeof(newUser.username) - 1);
    newUser.username[sizeof(newUser.username) - 1] = '\0';  // 确保以 '\0' 结尾

    strncpy_s(newUser.password, password.c_str(), sizeof(newUser.password) - 1);
    newUser.password[sizeof(newUser.password) - 1] = '\0';  // 确保以 '\0' 结尾

    strncpy_s(newUser.permissions, "", sizeof(newUser.permissions) - 1);  // 默认无权限
    newUser.permissions[sizeof(newUser.permissions) - 1] = '\0';  // 确保以 '\0' 结尾
    
    
    std::ofstream file("users.dat", std::ios::binary | std::ios::app);
    if (!file) {
        Output::printMessage(outputEdit, QString::fromStdString("创建用户时无法打开文件"));
        return false;
    }

    file.write(reinterpret_cast<char*>(&newUser), sizeof(User));
    if (!file) {
        Output::printMessage(outputEdit, QString::fromStdString("写入用户数据失败"));
        file.close();
        return false;
    }
    file.close();
    Output::printMessage(outputEdit, QString::fromStdString("用户 "+username+ " 创建成功") );

    return true;
}

// 授权
bool user::grantPermission(const std::string& username,const std::string& permission,
    const std::string& dbName,const std::string& tableName, QTextEdit* outputEdit)
{
    // 权限验证：确认当前用户是否有权授权此数据库
    std::string currentUser = std::string(user::getCurrentUser().username);  
    std::string sysDBPath = dbManager::basePath + "/ruanko.db";

    std::ifstream dbFileIn1(sysDBPath, std::ios::binary);
    if (!dbFileIn1) {
        Output::printMessage(outputEdit, "授权时无法打开数据库文件");
        return false;
    }

    bool isCurrentUserAuthorized = (currentUser == "sys");
    std::vector<DatabaseBlock> dbs;
    DatabaseBlock block{};

    while (dbFileIn1.read(reinterpret_cast<char*>(&block), sizeof(block))) {
        dbs.push_back(block);
        if (block.dbName == dbName) {
            std::string abledUsers(block.abledUsername);
            std::stringstream ss(abledUsers);
            std::string user;
            while (std::getline(ss, user, '|')) {
                if (user == currentUser) {
                    isCurrentUserAuthorized = true;
                    break;
                }
            }
            break;  // 找到目标数据库后就可退出
        }
    }
    dbFileIn1.close();

    if (!isCurrentUserAuthorized) {
        Output::printMessage(outputEdit, "你没有权限授权该数据库，请联系管理员");
        return false;
    }

    // Step 1: 更新用户结构体中的权限字段
    std::vector<User> users = loadUsers();
    bool userExists = false;
    for (auto& u : users) {
        if (u.username == username) {
            std::string perms(u.permissions);
            std::string fullPerm = permission + ":" + dbName;
            if (!tableName.empty()) {
                fullPerm += "." + tableName;
            }

            // 如果权限不存在则添加
            if (perms.find(fullPerm) == std::string::npos) {
                if (!perms.empty()) perms += "|";
                perms += fullPerm;
                strcpy_s(u.permissions, sizeof(u.permissions), perms.c_str());
            }

            userExists = true;
            break;
        }
    }

    if (!userExists) return false;
    // Step 2: 写回用户数据
    std::ofstream file("users.dat", std::ios::binary | std::ios::trunc);
    if (!file) {
        Output::printMessage(outputEdit, "授权时无法打开文件");
        return false;
    }
    for (const auto& u : users) {
         file.write(reinterpret_cast<const char*>(&u), sizeof(User));
    }
    file.close();
    
    // Step 3: 更新数据库文件
    for (auto& db : dbs) {
        // 比对数据库名
        if (db.dbName == dbName) {
            // 只处理数据库级别的权限
            if (permission == "CONNECT" || (permission == "CONNECT,RESOURCE" && tableName.empty())) {
                std::string abledUsers(db.abledUsername);
                if (abledUsers.find(username) == std::string::npos) {
                    if (!abledUsers.empty()) abledUsers += "|";
                    abledUsers += username;
                    strcpy_s(db.abledUsername, sizeof(db.abledUsername), abledUsers.c_str());
                }
            }
            if (!tableName.empty()) {
                if (permission != "CONNECT" && permission != "CONNECT,RESOURCE") {
                    Output::printMessage(outputEdit, "表级权限仅支持 CONNECT 或 CONNECT,RESOURCE");
                    return false;
                }
                // --- 合并表授权逻辑 ---
                std::string tableFilePath = dbManager::basePath + "/data/" + dbName + "/" + dbName + ".tb";
                std::fstream tableFile(tableFilePath, std::ios::in | std::ios::out | std::ios::binary);
                if (!tableFile) {
                    Output::printMessage(outputEdit, QString::fromStdString("无法打开表结构文件进行授权: " + tableFilePath));
                    return false;
                }

                TableBlock block;
                bool found = false;
                while (tableFile.read(reinterpret_cast<char*>(&block), sizeof(block))) {
                    if (std::string(block.name) == tableName) {
                        std::string abledUsers(block.abledUsers);
                        if (abledUsers.find(username) == std::string::npos) {
                            if (!abledUsers.empty()) abledUsers += "|";
                            abledUsers += username;
                            strcpy_s(block.abledUsers, sizeof(block.abledUsers), abledUsers.c_str());

                            tableFile.seekp(-static_cast<int>(sizeof(block)), std::ios::cur);
                            tableFile.write(reinterpret_cast<const char*>(&block), sizeof(block));
                        }
                        found = true;
                        break;
                    }
                }
                tableFile.close();
                if (!found) {
                    Output::printMessage(outputEdit, QString::fromStdString("未找到目标表: " + tableName));
                    return false;
                }

                // 确保数据库也授权（用于展示数据库列表）
                for (auto& db : dbs) {
                    if (db.dbName == dbName) {
                        std::string abledUsers(db.abledUsername);
                        if (abledUsers.find(username) == std::string::npos) {
                            if (!abledUsers.empty()) abledUsers += "|";
                            abledUsers += username;
                            strcpy_s(db.abledUsername, sizeof(db.abledUsername), abledUsers.c_str());
                        }
                        break;
                    }
                }
            }

        }
    }
        // Step 4: 写回数据库信息
        std::ofstream dbFileOut(sysDBPath, std::ios::binary | std::ios::trunc);
        if (!dbFileOut) {
            Output::printMessage(outputEdit, "写回数据库文件失败");
            return false;
        }
        for (const auto& db : dbs) {
            dbFileOut.write(reinterpret_cast<const char*>(&db), sizeof(db));
        }
        dbFileOut.close();

        return true;
    }

    

// 收回权限
bool user::revokePermission(const std::string& username, const std::string& permission) {
    std::vector<User> users = loadUsers();
    bool updated = false;

    for (auto& u : users) {
        if (username == u.username) {
            std::string perms(u.permissions);
            size_t pos = perms.find(permission);
            if (pos != std::string::npos) {
                // 删除权限 + 前置的 '|'（如果有）
                if (pos > 0 && perms[pos - 1] == '|') {
                    pos--;
                }
                size_t len = permission.size();
                if (pos + len < perms.size() && perms[pos + len] == '|') {
                    len++; // 删除后置 '|'
                }
                perms.erase(pos, len);
                strcpy_s(u.permissions, sizeof(u.permissions), perms.c_str());
                updated = true;
            }
            break;
        }
    }

    if (updated) {
        std::ofstream file("users.dat", std::ios::binary | std::ios::trunc);
        if (!file) {
            std::cout << "撤权时无法打开文件" << std::endl;
            return false;
        }

        for (const auto& u : users) {
            file.write(reinterpret_cast<const char*>(&u), sizeof(User));
        }
        file.close();
        std::cout << "已收回用户 " << username << " 的权限 " << permission << std::endl;
        return true;
    }

    std::cout << "撤权失败，用户不存在或未拥有该权限" << std::endl;
    return false;
}
/*
bool user::grantTablePermission(const std::string& username,
    const std::string& dbName,
    const std::string& tableName,
    QTextEdit* outputEdit)
{
    std::string tableFilePath = dbManager::basePath + "/data/" + dbName + "/" + dbName + ".tb";

    std::fstream tableFile(tableFilePath, std::ios::in | std::ios::out | std::ios::binary);
    if (!tableFile) {
        Output::printMessage(outputEdit, QString::fromStdString("无法打开表结构文件进行授权: " + tableFilePath));
        return false;
    }

    TableBlock block;
    while (tableFile.read(reinterpret_cast<char*>(&block), sizeof(block))) {
        if (std::string(block.name) == tableName) {
            // 修改权限字段
            std::string abledUsers(block.abledUsers);
            if (abledUsers.find(username) == std::string::npos) {
                if (!abledUsers.empty()) abledUsers += "|";
                abledUsers += username;
                strcpy_s(block.abledUsers, sizeof(block.abledUsers), abledUsers.c_str());

                // 回写到当前记录位置
                tableFile.seekp(-static_cast<int>(sizeof(block)), std::ios::cur);
                tableFile.write(reinterpret_cast<const char*>(&block), sizeof(block));
                tableFile.close();

                // 👇 新增：自动为数据库添加此用户（不涉及其他表）
                std::string sysDBPath = dbManager::basePath + "/" + dbManager::systemDBFile;
                std::fstream sysDB(sysDBPath, std::ios::in | std::ios::out | std::ios::binary);
                if (!sysDB) {
                    Output::printMessage(outputEdit, "警告：表授权成功，但无法打开系统数据库文件以更新数据库授权");
                    return true; // 表授权成功，不强制失败
                }

                DatabaseBlock dbBlock;
                while (sysDB.read(reinterpret_cast<char*>(&dbBlock), sizeof(dbBlock))) {
                    if (std::string(dbBlock.dbName) == dbName) {
                        std::string abled(dbBlock.abledUsername);
                        if (abled.find(username) == std::string::npos) {
                            if (!abled.empty()) abled += "|";
                            abled += username;
                            strcpy_s(dbBlock.abledUsername, sizeof(dbBlock.abledUsername), abled.c_str());

                            // 回写数据库块
                            sysDB.seekp(-static_cast<int>(sizeof(dbBlock)), std::ios::cur);
                            sysDB.write(reinterpret_cast<const char*>(&dbBlock), sizeof(dbBlock));
                        }
                        break; // 找到目标库即可退出
                    }
                }
                sysDB.close();
                return true;
            }
            else {
                tableFile.close();
                return true; // 已有权限，无需重复写入
            }
        }
    }

    tableFile.close();
    Output::printMessage(outputEdit, QString::fromStdString("未找到目标表: " + tableName));
    return false;
}

*/