#include "user.h"

// 读取用户列表
std::vector<user::User> user::loadUsers() {
    std::vector<User> users;
    std::ifstream file("users.dat", std::ios::binary);
    if (!file) return users;

    User user;
    while (file.read(reinterpret_cast<char*>(&user), sizeof(User))) {
        users.push_back(user);
    }
    file.close();
    return users;
}

// 检查用户是否存在
bool user::userExists(const std::string& username) {
    auto users = loadUsers();
    for (const auto& u : users) {
        if (std::string(u.username) == username) {
            return true;
        }
    }
    return false;
}

// 创建 sysdba 用户
void user::createSysDBA() {  // 添加 user:: 作用域
    if (userExists("sysdba")) {
        std::cout << "sysdba 用户已存在" << std::endl;
        return;
    }

    User sysdba;
    strcpy_s(sysdba.username, sizeof(sysdba.username), "sysdba");
    strcpy_s(sysdba.password, sizeof(sysdba.password), "admin123");
    strcpy_s(sysdba.permissions, sizeof(sysdba.permissions), "conn|resource");

    std::ofstream file("users.dat", std::ios::binary | std::ios::app);
    file.write(reinterpret_cast<char*>(&sysdba), sizeof(User));
    file.close();

    std::cout << "sysdba 用户创建成功，并赋予 conn 和 resource 权限" << std::endl;
}

user::User user::currentUser = {};
void user::setCurrentUser(const User& user) {
	currentUser = user;
}

bool user::hasPermission(const std::string& permission) {
    std::string permissions(currentUser.permissions);
    return permissions.find(permission) != std::string::npos;
}


// 创建用户
bool user::createUser(const std::string& username, const std::string& password) {
    if (userExists(username)) {
        std::cout << "用户 " << username << " 已存在，创建失败" << std::endl;
        return false;
    }

    User newUser;
    strcpy_s(newUser.username, sizeof(newUser.username), username.c_str());
    strcpy_s(newUser.password, sizeof(newUser.password), password.c_str());
    strcpy_s(newUser.permissions, sizeof(newUser.permissions), ""); // 默认无权限

    std::ofstream file("users.dat", std::ios::binary | std::ios::app);
    if (!file) {
        std::cout << "创建用户时无法打开文件" << std::endl;
        return false;
    }

    file.write(reinterpret_cast<char*>(&newUser), sizeof(User));
    file.close();

    std::cout << "用户 " << username << " 创建成功" << std::endl;
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
            std::cout << "授权时无法打开文件" << std::endl;
            return false;
        }

        for (const auto& u : users) {
            file.write(reinterpret_cast<const char*>(&u), sizeof(User));
        }
        file.close();
        std::cout << "已授权 " << permission << " 给用户 " << username << std::endl;
        return true;
    }

    std::cout << "授权失败，用户不存在或已有该权限" << std::endl;
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
