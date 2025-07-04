#pragma once

#include "Task.h"
#include <vector>
#include <string>
#include <thread>   // 用于线程
#include <mutex>    // 用于互斥锁
#include <atomic>   // 用于原子操作

class TaskManager {
public:
    TaskManager();
    void setCurrentUser(const string& username);
    bool addTask(const Task& task);
    bool deleteTask(long long taskId);
    vector<Task> getAllTasks() const;
    void startReminderThread();
    void stopReminderThread();

private:
    string tasks_file;
    vector<Task> tasks;
    long long next_id;
    string current_user;

       

    // 文件操作
    void loadTasks();
    void saveTask(const Task& task);
    void rewriteTasksFile();
    
    // 提醒线程相关
    void reminderCheckLoop();

    // 线程和互斥锁成员
    thread reminder_thread;
    mutable mutex tasks_mutex; // 可变的互斥锁，以便在const成员函数中使用
    atomic<bool> m_running;
};