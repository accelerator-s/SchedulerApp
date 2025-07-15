#include "UserManager.h"
#include "md5.h" // 用于密码加密
#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>  // for isupper, islower, isdigit
#include <cwctype> // 宽字符处理
#include <locale>  // 本地化支持
#include <cstdio>  // for remove()

UserManager::UserManager()
{
    // 在构造函数中调用 loadUsers()，加载已存在的用户数据
    loadUsers();
}

// modified by wby

bool UserManager::isPasswordValid(const string &password)
{
    if (password.length() < 6)
        return false;

    bool hasUpper = false, hasLower = false, hasDigit = false;

    for (char c : password)
    {
        wchar_t wc = static_cast<wchar_t>(c);
        if (iswupper(wc))
            hasUpper = true;
        if (iswlower(wc))
            hasLower = true;
        if (iswdigit(wc))
            hasDigit = true;
    }

    // 调试信息：打印当前密码的验证结果
    cerr << "密码验证 - 大写:" << hasUpper
         << " 小写:" << hasLower
         << " 数字:" << hasDigit << endl;

    // 3. 必须同时满足三个条件
    return hasUpper && hasLower && hasDigit;
}

bool UserManager::registerUser(const string &username, const string &password)
{
    cout << "Attempting to register user: " << username << endl;

    // 1. 检查用户名是否已存在
    if (users.find(username) != users.end())
    {
        cerr << "Registration failed: Username '" << username << "' already exists." << endl;
        return false; // 用户名已存在
    }

    // 2. 如果不存在，使用md5()加密密码
    string hashedPassword = md5(password);

    // 3. 将新用户存入内存
    users[username] = hashedPassword;

    // 4. 调用 saveUsers() 写入文件
    saveUsers();

    cout << "User '" << username << "' registered successfully." << endl;
    return true;
}

UserManager::LoginResult UserManager::login(const string &username, const string &password)
{
    cout << "Attempting to login user: " << username << endl;

    // 1. 检查用户是否存在
    auto it = users.find(username);
    if (it == users.end())
    {
        cerr << "Login failed: User '" << username << "' not found." << endl;
        return LoginResult::USER_NOT_FOUND; // 用户未找到
    }

    // 2. 如果存在，使用md5()加密输入密码
    string inputHashedPassword = md5(password);

    // 3. 与存储的加密密码进行比对
    if (it->second == inputHashedPassword)
    {
        cout << "User '" << username << "' logged in successfully." << endl;
        return LoginResult::SUCCESS; // 密码匹配，登录成功
    }
    else
    {
        cerr << "Login failed: Incorrect password for user '" << username << "'." << endl;
        return LoginResult::INCORRECT_PASSWORD; // 密码错误
    }
}

UserManager::ChangePasswordResult UserManager::changePassword(const string &username, const string &oldPassword, const string &newPassword)
{
    cout << "Attempting to change password for user: " << username << endl;

    // 1. 检查用户是否存在
    auto it = users.find(username);
    if (it == users.end())
    {
        cerr << "Password change failed: User '" << username << "' not found." << endl;
        return ChangePasswordResult::USER_NOT_FOUND;
    }

    // 2. 验证旧密码
    string oldHashedPassword = md5(oldPassword);
    if (it->second != oldHashedPassword)
    {
        cerr << "Password change failed: Incorrect old password for user '" << username << "'." << endl;
        return ChangePasswordResult::INCORRECT_PASSWORD;
    }

    // 3. 检查新密码的合法性
    if (!isPasswordValid(newPassword))
    {
        cerr << "Password change failed: New password does not meet complexity requirements." << endl;
        return ChangePasswordResult::INVALID_PASSWORD;
    }

    // 4. 更新为新密码
    string newHashedPassword = md5(newPassword);
    it->second = newHashedPassword;

    // 5. 保存到文件
    saveUsers();

    cout << "Password for user '" << username << "' changed successfully." << endl;
    return ChangePasswordResult::SUCCESS;
}

UserManager::ChangePasswordResult UserManager::updatePassword(const string &username, const string &newPassword)
{
    cout << "Attempting to update password for logged-in user: " << username << endl;

    // 1. 检查用户是否存在（双重保险）
    auto it = users.find(username);
    if (it == users.end())
    {
        cerr << "Password update failed: User '" << username << "' not found." << endl;
        return ChangePasswordResult::USER_NOT_FOUND;
    }

    // 2. 检查新密码的合法性
    if (!isPasswordValid(newPassword))
    {
        cerr << "Password update failed: New password does not meet complexity requirements." << endl;
        return ChangePasswordResult::INVALID_PASSWORD;
    }

    // 3. 直接更新为新密码
    string newHashedPassword = md5(newPassword);
    it->second = newHashedPassword;

    // 4. 保存到文件
    saveUsers();

    cout << "Password for user '" << username << "' updated successfully." << endl;
    return ChangePasswordResult::SUCCESS;
}

UserManager::DeleteResult UserManager::deleteUser(const string &username)
{
    cout << "Attempting to delete user: " << username << endl;

    // 1. 先检查用户是否存在于内存中
    auto it = users.find(username);
    if (it == users.end())
    {
        // 如果内存中不存在，也尝试删除文件以防万一，但报告操作失败
        cerr << "Deletion failed: User '" << username << "' not found in map." << endl;
        string task_file = username + "_tasks.dat";
        remove(task_file.c_str()); // 清理可能存在的孤立文件
        return DeleteResult::FAILURE;
    }

    // 2. 如果存在，则从内存的map中擦除
    users.erase(it);
    cout << "User '" << username << "' erased from memory map." << endl;

    // 3. 重写整个用户文件，此时被删除的用户已不在map中
    saveUsers();

    // 4. 删除该用户的任务文件
    string task_file = username + "_tasks.dat";
    if (remove(task_file.c_str()) != 0)
    {
        // 如果文件不存在，打印 "No such file or directory"
        perror(("Info: Could not remove task file " + task_file).c_str());
    }

    cout << "User '" << username << "' and associated data deleted successfully." << endl;
    return DeleteResult::SUCCESS;
}

void UserManager::loadUsers()
{
    ifstream file(users_file);
    if (!file.is_open())
    {
        cerr << "Could not open users file: " << users_file
             << ". It will be created upon first user registration." << endl;
        return;
    }

    string line;
    while (getline(file, line))
    {
        stringstream ss(line);
        string username, hashedPassword;
        if (ss >> username >> hashedPassword)
        {
            users[username] = hashedPassword;
        }
    }
    file.close();
    cout << "Loaded " << users.size() << " users from " << users_file << endl;
}

void UserManager::saveUsers()
{
    ofstream file(users_file, ios::out | ios::trunc);
    if (!file.is_open())
    {
        cerr << "Error: Could not open users file for writing: " << users_file << endl;
        return;
    }

    for (const auto &pair : users)
    {
        file << pair.first << " " << pair.second << endl;
    }
    file.close();
    cout << "Saved " << users.size() << " users to " << users_file << endl;
}