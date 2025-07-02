#include "UserManager.h"
#include "md5.h"
#include <iostream>

UserManager::UserManager() {
    // TODO: 在构造函数中调用 loadUsers()
}

bool UserManager::registerUser(const std::string& username, const std::string& password) {
    std::cout << "Attempting to register user: " << username << std::endl;
    // TODO: 1. 检查用户名是否已存在
    // TODO: 2. 如果不存在，使用md5()加密密码
    // TODO: 3. 将新用户存入内存
    // TODO: 4. 调用 saveUsers() 写入文件
    return true; // 假设总是成功
}

bool UserManager::login(const std::string& username, const std::string& password) {
    std::cout << "Attempting to login user: " << username << std::endl;
    // TODO: 1. 检查用户是否存在
    // TODO: 2. 如果存在，使用md5()加密输入密码
    // TODO: 3. 与存储的加密密码进行比对
    return true; // 假设总是成功
}

void UserManager::loadUsers() {
    // TODO: 从 users_file 文件中加载用户数据到内存
}

void UserManager::saveUsers() {
    // TODO: 将内存中的所有用户数据写入到 users_file 文件
}