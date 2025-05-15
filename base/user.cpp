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
    strcpy_s(sysdba.permissions, sizeof(sysdba.permissions), "CONNECT,RESOURCE");

    std::ofstream file("users.dat", std::ios::binary | std::ios::app);

    if (!file) {
        Output::printMessage(outputEdit, "无法打开文件创建用户");
        return;
    }
    file.write(reinterpret_cast<char*>(&sysdba), sizeof(User));
    file.close();

    qDebug() << "sysdba 用户创建成功，并赋予 connect 和 resource 权限";
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
    // sys 用户拥有所有权限
    if (strcmp(currentUser.username, "sys") == 0) {
        return true;
    }

    // 如果是 CONNECT 权限，直接从数据库块的 abledUsername 判断
    if (requiredPerm == "CONNECT" && tableName.empty()) {
        std::string sysDBPath = dbManager::basePath + "/ruanko.db";
        std::ifstream dbFile(sysDBPath, std::ios::binary);
        if (!dbFile) {
            return false;
        }

        DatabaseBlock block;
        while (dbFile.read(reinterpret_cast<char*>(&block), sizeof(block))) {
            if (block.dbName == dbName) {
                std::string abledUsers(block.abledUsername);
                std::stringstream ss(abledUsers);
                std::string user;
                while (std::getline(ss, user, '|')) {
                    if (user == currentUser.username) {
                        return true;
                    }
                }
                break;
            }
        }
        dbFile.close();
        return false;
    }

    std::string permissions(currentUser.permissions);
    std::stringstream ss(permissions);
    std::string permEntry;

    std::string targetDbPerm = requiredPerm + ":" + dbName;         // 库级权限
    std::string targetTablePerm = targetDbPerm + "." + tableName;   // 表级权限

    /*bool hasTablePerm = false;
    bool hasDbPerm = false;*/

    while (std::getline(ss, permEntry, '|')) {
        if (!tableName.empty()) {
            /*if (permEntry == targetTablePerm) {
                hasTablePerm = true;
                break; // 找到表权限了直接返回
            }
            if (permEntry == targetDbPerm) {
                hasDbPerm = true;  // 记录库权限，但继续寻找表权限
            }*/
            if (permEntry == targetTablePerm || permEntry == targetDbPerm) {
                return true;
            }
        }
        else {
            // 只检查库级权限
            if (permEntry == targetDbPerm) {
                return true;
            }
        }
    }

    //// 如果是检查表权限，允许库权限作为备选（这里你可以根据实际需求调整）
    //if (!tableName.empty()) {
    //    if (hasTablePerm) {
    //        return true;
    //    }
    //    else if (hasDbPerm) {
    //        // 如果需要表权限必须存在，则这里返回 false，  
    //        // 如果允许库权限覆盖表权限，则返回 true
    //        return true;  // 或者改成 false 视需求
    //    }
    //}

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
bool user::grantPermission(const std::string& username, const std::string& permission,
    const std::string& dbName, const std::string& tableName, QTextEdit* outputEdit)
{
    std::string currentUser = std::string(user::getCurrentUser().username);
    std::string sysDBPath = dbManager::basePath + "/ruanko.db";

    // Step 1: 读取所有数据库信息
    std::ifstream dbFileIn1(sysDBPath, std::ios::binary);
    if (!dbFileIn1) {
        Output::printMessage(outputEdit, "授权时无法打开数据库文件");
        return false;
    }

    bool isCurrentUserAuthorized = (currentUser == "sys");
    std::vector<DatabaseBlock> dbs;
    DatabaseBlock block{};

    while (dbFileIn1.read(reinterpret_cast<char*>(&block), sizeof(block))) {
        if (!isCurrentUserAuthorized && block.dbName == dbName) {
            std::string abledUsers(block.abledUsername);
            std::stringstream ss(abledUsers);
            std::string user;
            while (std::getline(ss, user, '|')) {
                if (user == currentUser) {
                    isCurrentUserAuthorized = true;
                    break;
                }
            }
        }
        dbs.push_back(block);  // push 所有数据库，不能 break
    }
    dbFileIn1.close();

    // *** 调试：打印读取到的数据库名 ***
    qDebug() << "读取数据库块列表:";
    for (const auto& db : dbs) {
        qDebug() << " - " << QString::fromStdString(db.dbName);
    }//

    if (!isCurrentUserAuthorized) {
        Output::printError(outputEdit, "你没有权限授权该数据库，请联系管理员");
        return false;
    }

    // Step 1.5: 当前用户是否有权限授权这个资源（db/table）
    if (currentUser != "sys") {
        std::string requiredPerms = permission;  // 如 "CONNECT,RESOURCE" 或 "CONNECT"
        std::stringstream permStream(requiredPerms);
        std::string singlePerm;

        // 获取当前用户权限串
        const auto currentUsers = loadUsers();
        std::string currentPerms;
        for (const auto& u : currentUsers) {
            if (std::string(u.username) == currentUser) {
                currentPerms = u.permissions;
                break;
            }
        }

        while (std::getline(permStream, singlePerm, ',')) {
            std::string fullPerm = singlePerm + ":" + dbName;
            if (!tableName.empty()) {
                fullPerm += "." + tableName;
            }

            // 如果找不到这个权限，说明当前用户无权转授
            if (currentPerms.find(fullPerm) == std::string::npos) {
                Output::printError(outputEdit, QString::fromStdString("你没有权限授权 " + fullPerm));
                return false;
            }
        }
    }


    // Step 2: 加载用户列表并修改权限字段
    std::vector<User> users = loadUsers();
    bool userExists = false;

    for (auto& u : users) {
        if (std::string(u.username) == username) {
            std::string perms(u.permissions);
            std::stringstream permStream(permission);  // 例如 "CONNECT,RESOURCE"
            std::string singlePerm;
            while (std::getline(permStream, singlePerm, ',')) {
                std::string fullPerm = singlePerm + ":" + dbName;
                if (!tableName.empty()) {
                    fullPerm += "." + tableName;
                }

                if (perms.find(fullPerm) == std::string::npos) {
                    if (!perms.empty()) perms += "|";
                    perms += fullPerm;
                }
            }

            // 表级授权要补 CONNECT:db 权限
            if (!tableName.empty()) {
                std::string connectPerm = "CONNECT:" + dbName;
                if (perms.find(connectPerm) == std::string::npos) {
                    if (!perms.empty()) perms += "|";
                    perms += connectPerm;
                }
            }

            strcpy_s(u.permissions, sizeof(u.permissions), perms.c_str());
            userExists = true;
            break;
        }
    }

    if (!userExists) {
        Output::printMessage(outputEdit, "目标用户不存在");
        return false;
    }

    // Step 3: 写回用户数据
    std::ofstream file("users.dat", std::ios::binary | std::ios::trunc);
    if (!file) {
        Output::printMessage(outputEdit, "授权时无法打开用户文件");
        return false;
    }
    for (const auto& u : users) {
        file.write(reinterpret_cast<const char*>(&u), sizeof(User));
    }
    file.close();

    // Step 4: 修改对应数据库块和表块
    for (auto& db : dbs) {
        if (db.dbName != dbName) continue;

        std::string abledUsers(db.abledUsername);
        if (abledUsers.find(username) == std::string::npos) {
            if (!abledUsers.empty()) abledUsers += "|";
            abledUsers += username;
            strcpy_s(db.abledUsername, sizeof(db.abledUsername), abledUsers.c_str());
        }

        // 表级授权要更新对应表块
        if (!tableName.empty()) {
            std::string tableFilePath = dbManager::basePath + "/data/" + dbName + "/" + dbName + ".tb";
            std::fstream tableFile(tableFilePath, std::ios::in | std::ios::out | std::ios::binary);
            if (!tableFile) {
                Output::printMessage(outputEdit, QString::fromStdString("无法打开表结构文件进行授权: " + tableFilePath));
                return false;
            }

            TableBlock tableBlock;
            bool found = false;
            while (tableFile.read(reinterpret_cast<char*>(&tableBlock), sizeof(tableBlock))) {
                if (std::string(tableBlock.name) == tableName) {
                    std::string tableUsers(tableBlock.abledUsers);
                    if (tableUsers.find(username) == std::string::npos) {
                        if (!tableUsers.empty()) tableUsers += "|";
                        tableUsers += username;
                        strcpy_s(tableBlock.abledUsers, sizeof(tableBlock.abledUsers), tableUsers.c_str());

                        tableFile.seekp(-static_cast<int>(sizeof(tableBlock)), std::ios::cur);
                        tableFile.write(reinterpret_cast<const char*>(&tableBlock), sizeof(tableBlock));
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
        }

        break;  // 授权完目标库就可以退出
    }

    // Step 5: 写回所有数据库块
    std::ofstream dbFileOut(sysDBPath, std::ios::binary | std::ios::trunc);
    if (!dbFileOut) {
        Output::printMessage(outputEdit, "写回数据库文件失败");
        return false;
    }
    for (const auto& db : dbs) {
        dbFileOut.write(reinterpret_cast<const char*>(&db), sizeof(DatabaseBlock));
    }
    dbFileOut.close();

    // *** 调试：写回后再读一次确认数据库列表 ***
    std::ifstream dbFileCheck(sysDBPath, std::ios::binary);
    if (!dbFileCheck) {
        qDebug() << "警告: 授权后无法打开数据库文件校验";
    }
    else {
        qDebug() << "写回后数据库列表:";
        DatabaseBlock dbCheckBlock{};
        while (dbFileCheck.read(reinterpret_cast<char*>(&dbCheckBlock), sizeof(dbCheckBlock))) {
            qDebug() << " - " << QString::fromStdString(dbCheckBlock.dbName);
        }
        dbFileCheck.close();
    }//

    return true;
}

bool user::revokePermission(const std::string& username, const std::string& permission,
    const std::string& dbName, const std::string& tableName, QTextEdit* outputEdit)
{
    std::string currentUser = std::string(user::getCurrentUser().username);
    std::string sysDBPath = dbManager::basePath + "/ruanko.db";

    // Step 1: 加载所有数据库块 & 检查权限
    std::ifstream dbFileIn(sysDBPath, std::ios::binary);
    if (!dbFileIn) {
        Output::printMessage(outputEdit, "撤销权限时无法打开数据库文件");
        return false;
    }

    bool isCurrentUserAuthorized = (currentUser == "sys");
    std::vector<DatabaseBlock> dbs;
    DatabaseBlock block{};

    while (dbFileIn.read(reinterpret_cast<char*>(&block), sizeof(block))) {
        dbs.push_back(block);
        if (!isCurrentUserAuthorized && block.dbName == dbName) {
            std::string abledUsers(block.abledUsername);
            std::stringstream ss(abledUsers);
            std::string user;
            while (std::getline(ss, user, '|')) {
                if (user == currentUser) {
                    isCurrentUserAuthorized = true;
                    break;
                }
            }
        }
    }
    dbFileIn.close();

    if (!isCurrentUserAuthorized) {
        Output::printMessage(outputEdit, "你没有权限撤销该数据库权限");
        return false;
    }

    // Step 2: 修改用户权限字段
    std::vector<User> users = loadUsers();
    bool userExists = false;
    std::string fullPerm = permission + ":" + dbName;
    if (!tableName.empty()) fullPerm += "." + tableName;

    for (auto& u : users) {
        if (std::string(u.username) == username) {
            userExists = true;
            std::string perms(u.permissions);
            std::vector<std::string> updatedPerms;
            std::stringstream ss(perms);
            std::string perm;
            while (std::getline(ss, perm, '|')) {
                if (perm != fullPerm) {
                    updatedPerms.push_back(perm);
                }
            }

            // 重建权限字符串
            std::string newPerms;
            for (size_t i = 0; i < updatedPerms.size(); ++i) {
                if (i > 0) newPerms += "|";
                newPerms += updatedPerms[i];
            }
            strcpy_s(u.permissions, sizeof(u.permissions), newPerms.c_str());
            break;
        }
    }

    if (!userExists) {
        Output::printMessage(outputEdit, "目标用户不存在");
        return false;
    }

    // Step 3: 写回用户文件
    std::ofstream file("users.dat", std::ios::binary | std::ios::trunc);
    if (!file) {
        Output::printMessage(outputEdit, "撤销权限时无法写入用户文件");
        return false;
    }
    for (const auto& u : users) {
        file.write(reinterpret_cast<const char*>(&u), sizeof(User));
    }
    file.close();

    // Step 4: 修改数据库块 & 表块
    for (auto& db : dbs) {
        if (db.dbName != dbName) continue;

        // 判断是否还拥有任何相关权限
        bool userHasOtherPerm = false;
        for (const auto& u : users) {
            if (std::string(u.username) == username) {
                std::stringstream ss(u.permissions);
                std::string p;
                while (std::getline(ss, p, '|')) {
                    if (p.find(dbName + ".") != std::string::npos || p == (permission + ":" + dbName)) {
                        userHasOtherPerm = true;
                        break;
                    }
                }
                break;
            }
        }

        if (!userHasOtherPerm) {
            std::string newAbled;
            std::stringstream ss(db.abledUsername);
            std::string user;
            while (std::getline(ss, user, '|')) {
                if (user != username) {
                    if (!newAbled.empty()) newAbled += "|";
                    newAbled += user;
                }
            }
            strcpy_s(db.abledUsername, sizeof(db.abledUsername), newAbled.c_str());
        }

        // Step 5: 如果是表级权限，更新表块 abledUsers 字段
        if (!tableName.empty()) {
            std::string tableFilePath = dbManager::basePath + "/data/" + dbName + "/" + dbName + ".tb";
            std::fstream tableFile(tableFilePath, std::ios::in | std::ios::binary);
            if (!tableFile) {
                Output::printMessage(outputEdit, QString::fromStdString("无法打开表文件: " + tableFilePath));
                return false;
            }

            std::vector<TableBlock> tableBlocks;
            TableBlock tableBlock;
            bool found = false;

            // 读取所有表块，找出目标表并修改权限
            while (tableFile.read(reinterpret_cast<char*>(&tableBlock), sizeof(tableBlock))) {
                if (std::string(tableBlock.name) == tableName) {
                    found = true;
                    // 移除用户权限
                    std::string updatedUsers;
                    std::stringstream uss(tableBlock.abledUsers);
                    std::string tuser;
                    while (std::getline(uss, tuser, '|')) {
                        if (tuser != username) {
                            if (!updatedUsers.empty()) updatedUsers += "|";
                            updatedUsers += tuser;
                        }
                    }
                    strcpy_s(tableBlock.abledUsers, sizeof(tableBlock.abledUsers), updatedUsers.c_str());
                }
                tableBlocks.push_back(tableBlock);
            }
            tableFile.close();

            if (!found) {
                Output::printMessage(outputEdit, QString::fromStdString("未找到目标表: " + tableName));
                return false;
            }

            // 重新写回所有表块
            std::ofstream tableOut(tableFilePath, std::ios::binary | std::ios::trunc);
            if (!tableOut) {
                Output::printMessage(outputEdit, QString::fromStdString("无法写入表文件: " + tableFilePath));
                return false;
            }
            for (const auto& tb : tableBlocks) {
                tableOut.write(reinterpret_cast<const char*>(&tb), sizeof(TableBlock));
            }
            tableOut.close();
        }

        break;
    }

    // Step 6: 写回数据库文件
    std::ofstream dbFileOut(sysDBPath, std::ios::binary | std::ios::trunc);
    if (!dbFileOut) {
        Output::printMessage(outputEdit, "无法保存数据库信息");
        return false;
    }
    for (const auto& db : dbs) {
        dbFileOut.write(reinterpret_cast<const char*>(&db), sizeof(DatabaseBlock));
    }
    dbFileOut.close();

    return true;
}


