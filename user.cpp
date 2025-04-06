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