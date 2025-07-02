#pragma once

#include "Task.h"
#include <vector>
#include <string>

class TaskManager {
public:
    TaskManager();
    void setCurrentUser(const std::string& username);
    bool addTask(const Task& task);
    bool deleteTask(long long taskId);
    std::vector<Task> getAllTasks() const;
    void startReminderThread();
    void stopReminderThread();

private:
    std::string tasks_file;
    std::vector<Task> tasks;
    long long next_id;
    std::string current_user;

    void loadTasks();
    void saveTask(const Task& task);
    void rewriteTasksFile();
    void reminderCheckLoop();

    // TODO: 添加线程和互斥锁成员
    // std::thread reminder_thread;
    // std::mutex tasks_mutex;
    // std::atomic<bool> running;
};
