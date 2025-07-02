#include "TaskManager.h"
#include <iostream>
#include <algorithm>

TaskManager::TaskManager() : next_id(1) {
    // TODO: 初始化线程和原子布尔值
}

void TaskManager::setCurrentUser(const std::string& username) {
    current_user = username;
    tasks_file = username + "_tasks.dat";
    tasks.clear();
    // TODO: 调用 loadTasks()
    std::cout << "Task manager set for user: " << username << ". Tasks will be loaded from " << tasks_file << std::endl;
}

bool TaskManager::addTask(const Task& task) {
    // TODO: 1. 检查任务名称+开始时间是否唯一
    // TODO: 2. 分配唯一ID
    // TODO: 3. 将任务添加到内存中的 tasks 向量
    // TODO: 4. 调用 saveTask() 将新任务追加到文件
    // TODO: 5. 排序任务列表
    std::cout << "Adding task: " << task.name << std::endl;
    return true;
}

bool TaskManager::deleteTask(long long taskId) {
    // TODO: 1. 在内存中找到并删除任务
    // TODO: 2. 调用 rewriteTasksFile() 重写整个任务文件
    std::cout << "Deleting task with ID: " << taskId << std::endl;
    return true;
}

std::vector<Task> TaskManager::getAllTasks() const {
    // TODO: 返回排序后的任务列表
    return tasks;
}

void TaskManager::loadTasks() {
    // TODO: 从当前用户的 tasks_file 加载任务列表到 tasks 向量
    // 加载后需要找到最大的ID，更新 next_id
}

void TaskManager::saveTask(const Task& task) {
    // TODO: 以追加模式打开文件，将单个任务写入文件
}

void TaskManager::rewriteTasksFile() {
    // TODO: 以写入模式打开文件，将内存中所有的任务写入，用于删除操作后更新文件
}

void TaskManager::startReminderThread() {
    // TODO: 创建并启动一个后台线程，该线程运行 reminderCheckLoop()
    std::cout << "Reminder thread would be started here." << std::endl;
}

void TaskManager::stopReminderThread() {
    // TODO: 设置原子布尔值为false，并join线程
     std::cout << "Reminder thread would be stopped here." << std::endl;
}

void TaskManager::reminderCheckLoop() {
    // TODO: 这是一个循环:
    // 1. sleep一段时间 (例如 30s)
    // 2. 检查 m_running 标志
    // 3. 遍历所有任务，如果 reminderTime 到达且未提醒，则打印提醒信息
}