#pragma once

#include "Task.h"
#include <vector>
#include <string>
<<<<<<< HEAD
#include <thread>   // 用于线程
#include <mutex>    // 用于互斥锁
#include <atomic>   // 用于原子操作
#include <functional> // 用于 std::function
=======
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
>>>>>>> db775d4e398de3b2778aff80a0b74f2c1814ef1c

class TaskManager {
public:
    TaskManager();
    void setCurrentUser(const string& username);
    bool addTask(const Task& task);
    bool deleteTask(long long taskId);
    vector<Task> getAllTasks() const;
    void startReminderThread();
    void stopReminderThread();

// 定义提醒回调函数类型（参数：提醒标题、内容）
    using ReminderCallback = std::function<void(const std::string&, const std::string&)>;

    // 设置回调函数（由SchedulerApp注册）
    void setReminderCallback(ReminderCallback callback) {
        reminder_callback = std::move(callback);
    }//modify just now

private:
    string tasks_file;
    vector<Task> tasks;
    long long next_id;
    string current_user;

<<<<<<< HEAD
  ReminderCallback reminder_callback;  // 保存回调函数     

=======
>>>>>>> db775d4e398de3b2778aff80a0b74f2c1814ef1c
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
    
    // **新增：条件变量，用于唤醒睡眠中的线程**
    condition_variable m_cv;
};