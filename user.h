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
        char permissions[50];  // "conn|resource"
    };

    static std::vector<User> loadUsers();
    static bool userExists(const std::string& username);
    static void createSysDBA();  // ȷ��������
};
