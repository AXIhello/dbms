#include "user.h"

// ��ȡ�û��б�
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

// ����û��Ƿ����
bool user::userExists(const std::string& username) {
    auto users = loadUsers();
    for (const auto& u : users) {
        if (std::string(u.username) == username) {
            return true;
        }
    }
    return false;
}

// ���� sysdba �û�
void user::createSysDBA() {  // ��� user:: ������
    if (userExists("sysdba")) {
        std::cout << "sysdba �û��Ѵ���" << std::endl;
        return;
    }

    User sysdba;
    strcpy_s(sysdba.username, sizeof(sysdba.username), "sysdba");
    strcpy_s(sysdba.password, sizeof(sysdba.password), "admin123");
    strcpy_s(sysdba.permissions, sizeof(sysdba.permissions), "conn|resource");

    std::ofstream file("users.dat", std::ios::binary | std::ios::app);
    file.write(reinterpret_cast<char*>(&sysdba), sizeof(User));
    file.close();

    std::cout << "sysdba �û������ɹ��������� conn �� resource Ȩ��" << std::endl;
}
