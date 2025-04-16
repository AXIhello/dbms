#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>

class user {
public:
    struct User {
        char username[20];
        char password[20];
        char permissions[100];  // "conn|resource"
    };

    static std::vector<User> loadUsers();
    static bool userExists(const std::string& username);
    static void createSysDBA();  // ȷ��������

    //当前登录用户
    static User currentUser;
    static void setCurrentUser(const User& user);
    static void getCurrentUser();
    static bool hasPermission(const std::string& permission);

    //用户管理操作
    static bool createUser(const std::string& username, const std::string& password);
    static bool grantPermission(const std::string& username, const std::string& permission);
    static bool revokePermission(const std::string& username, const std::string& permission);

};
