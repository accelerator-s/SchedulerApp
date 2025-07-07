#pragma once
#include <string>
#include <map>

using namespace std;

class UserManager {
public:
    // 定义登录结果的枚举，供UI层判断
    enum class LoginResult {
        SUCCESS,
        USER_NOT_FOUND,
        INCORRECT_PASSWORD
    };

    //modified by wby
    // 新增：密码验证工具函数
    static bool isPasswordValid(const std::string& password);  // 建议设为static

    // 新增：定义修改密码结果的枚举
    enum class ChangePasswordResult {
        SUCCESS,
        USER_NOT_FOUND,
        INCORRECT_PASSWORD
    };

    UserManager();
    bool registerUser(const string& username, const string& password);
    // 修改login函数的返回类型
    LoginResult login(const string& username, const string& password);
    // 新增：修改密码函数
    ChangePasswordResult changePassword(const string& username, const string& oldPassword, const string& newPassword);

private:
    const string users_file = "users.dat";
    map<string, string> users;


    void loadUsers();
    void saveUsers();
};