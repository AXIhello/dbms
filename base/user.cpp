#include "user.h"
#include <ui/output.h>

QTextEdit* user::outputEdit = nullptr;
// 读取用户列表
std::vector<user::User> user::loadUsers() {
    std::vector<User> users;
    std::ifstream file("D:/Files/Study/Assignment/DB_PracticalTraining/group/3/users.dat", std::ios::binary);
    if (!file) {
        // 文件不存在，创建空文件
        std::ofstream newFile("D:/Files/Study/Assignment/DB_PracticalTraining/group/3/users.dat",
            std::ios::binary | std::ios::trunc);
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
    std::ifstream file("D:/Files/Study/Assignment/DB_PracticalTraining/group/3/users.dat", std::ios::binary);
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
    if (userExists("sysdba")) {
        Output::printMessage(outputEdit, "sysdba 用户已存在");
        return;
    }

    User sysdba;
    strcpy_s(sysdba.username, sizeof(sysdba.username), "sysdba");
    strcpy_s(sysdba.password, sizeof(sysdba.password), "admin123");
    strcpy_s(sysdba.permissions, sizeof(sysdba.permissions), "conn|resource");


    std::ofstream file("D:/Files/Study/Assignment/DB_PracticalTraining/group/3/users.dat", std::ios::binary | std::ios::trunc);
    if (!file) {
        Output::printMessage(outputEdit, "无法打开文件创建用户");
        return;
    }
    file.write(reinterpret_cast<char*>(&sysdba), sizeof(User));
    file.close();

    Output::printMessage(outputEdit, "sysdba 用户创建成功，并赋予 conn 和 resource 权限");
}

user::User user::currentUser = {};
void user::setCurrentUser(const User& u) {
	currentUser = u;
}

void user::getCurrentUser() {
    std::cout << "Current User: " << currentUser.username << "\n";
    std::cout << "Permissions: " << currentUser.permissions << "\n";
}

bool user::hasPermission(const std::string& permission) {
    std::string permissions(currentUser.permissions);
    return permissions.find(permission) != std::string::npos;
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
    

    std::ofstream file("D:/Files/Study/Assignment/DB_PracticalTraining/group/3/users.dat", std::ios::binary | std::ios::app);
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
bool user::grantPermission(const std::string& username, const std::string& permission) {
    std::vector<User> users = loadUsers();
    bool updated = false;

    for (auto& u : users) {
        if (username == u.username) {
            std::string perms(u.permissions);
            if (perms.find(permission) == std::string::npos) {
                if (!perms.empty()) perms += "|";
                perms += permission;
                strcpy_s(u.permissions, sizeof(u.permissions), perms.c_str());
                updated = true;
            }
            break;
        }
    }

    if (updated) {
        std::ofstream file("users.dat", std::ios::binary | std::ios::trunc);
        if (!file) {
            Output::printMessage(outputEdit, "授权时无法打开文件");
            return false;
        }

        for (const auto& u : users) {
            file.write(reinterpret_cast<const char*>(&u), sizeof(User));
        }
        file.close();
        Output::printMessage(outputEdit, "已授权 " + QString::fromStdString(permission) + " 给用户 " + QString::fromStdString(username));
        return true;
    }

    Output::printMessage(outputEdit, "授权失败，用户不存在或已有该权限");
    return false;
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
