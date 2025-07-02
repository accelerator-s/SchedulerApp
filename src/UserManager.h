#pragma once
#include <string>

class UserManager {
public:
    UserManager();
    bool registerUser(const std::string& username, const std::string& password);
    bool login(const std::string& username, const std::string& password);

private:
    const std::string users_file = "users.dat";
    void loadUsers();
    void saveUsers();
    // TODO: 使用std::map<std::string, std::string>存储用户名和加密后的密码
};
