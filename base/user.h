#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <QTextEdit>

class user {
public:
    struct User {
        char username[20];
        char password[20];
        char permissions[20]; // "conn|resource"
    };
    //用户管理
    static std::vector<User> loadUsers();
    static bool userExists(const std::string& username);
    static bool createUser(const std::string& username, const std::string& password);
    static bool grantPermission(const std::string& username, const std::string& permission);
    static bool revokePermission(const std::string& username, const std::string& permission);
    static void createSysDBA();  

    //当前登录用户
    static void setCurrentUser(const User& user);
    static const User& getCurrentUser();
    static bool hasPermission(const std::string& permission);

    //ui控件
    //把 outputEdit 存成静态成员变量,不用传参，便于调用
    static QTextEdit* outputEdit;

private:
    static User currentUser;

};
