#include "UserManager.h"
#include "md5.h" // 用于密码加密
#include <iostream>
#include <fstream>
#include <sstream>

UserManager::UserManager() {
    // 在构造函数中调用 loadUsers()，加载已存在的用户数据
    loadUsers();
}

bool UserManager::registerUser(const string& username, const string& password) {
    cout << "Attempting to register user: " << username << endl;
    
    // 1. 检查用户名是否已存在
    if (users.find(username) != users.end()) {
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

// 实现返回 LoginResult 的版本
UserManager::LoginResult UserManager::login(const string& username, const string& password) {
    cout << "Attempting to login user: " << username << endl;

    // 1. 检查用户是否存在
    auto it = users.find(username);
    if (it == users.end()) {
        cerr << "Login failed: User '" << username << "' not found." << endl;
        return LoginResult::USER_NOT_FOUND; // 用户未找到
    }

    // 2. 如果存在，使用md5()加密输入密码
    string inputHashedPassword = md5(password);

    // 3. 与存储的加密密码进行比对
    if (it->second == inputHashedPassword) {
        cout << "User '" << username << "' logged in successfully." << endl;
        return LoginResult::SUCCESS; // 密码匹配，登录成功
    } else {
        cerr << "Login failed: Incorrect password for user '" << username << "'." << endl;
        return LoginResult::INCORRECT_PASSWORD; // 密码错误
    }
}

void UserManager::loadUsers() {
    ifstream file(users_file);
    if (!file.is_open()) {
        // 如果文件不存在，这是正常情况（首次运行或文件被删除）。
        // 无需创建空文件，因为第一次注册成功后，saveUsers()会自动创建它。
        cerr << "Could not open users file: " << users_file 
             << ". It will be created upon first user registration." << endl;
        return;
    }

    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        string username, hashedPassword;
        if (ss >> username >> hashedPassword) {
            users[username] = hashedPassword;
        }
    }
    file.close();
    cout << "Loaded " << users.size() << " users from " << users_file << endl;
}

void UserManager::saveUsers() {
    // 以输出模式打开文件。如果文件不存在，会自动创建。如果存在，会清空内容。
    ofstream file(users_file, ios::out | ios::trunc);
    if (!file.is_open()) {
        cerr << "Error: Could not open users file for writing: " << users_file << endl;
        return;
    }

    for (const auto& pair : users) {
        file << pair.first << " " << pair.second << endl;
    }
    file.close();
    cout << "Saved " << users.size() << " users to " << users_file << endl;
}