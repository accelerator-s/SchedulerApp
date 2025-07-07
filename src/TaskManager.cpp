#include "TaskManager.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <vector>
#include <ctime>

using namespace std;

// TaskManager 构造函数
TaskManager::TaskManager() : next_id(1), m_running(false) {
    // 互斥锁、条件变量和线程对象会自动默认构造
    // 原子布尔值 m_running 初始化为 false
}

// 设置当前用户，加载其任务列表
void TaskManager::setCurrentUser(const string& username) {
    lock_guard<mutex> lock(tasks_mutex); // 锁定以修改内部状态
    
    current_user = username;
    tasks_file = username + "_tasks.dat";
    tasks.clear();
    next_id = 1; // 重置ID计数器
    
    // 调用 loadTasks() 加载该用户的任务
    loadTasks();
    
    cout << "任务管理器已为用户 " << username << " 设置。任务将从 " << tasks_file << " 加载。" << endl;
}

// 添加一个新任务
bool TaskManager::addTask(const Task& task) {
    lock_guard<mutex> lock(tasks_mutex);

    // 1. 检查任务名称+开始时间是否唯一
    for (const auto& existing_task : tasks) {
        if (existing_task.name == task.name && existing_task.startTime == task.startTime) {
            cerr << "错误: 一个同名且同开始时间的任务已存在。" << endl;
            return false;
        }
    }

    Task newTask = task;
    // 2. 分配唯一ID
    newTask.id = next_id++;
    
    // 3. 将任务添加到内存中的 tasks 向量
    tasks.push_back(newTask);
    
    // 4. 调用 saveTask() 将新任务追加到文件
    saveTask(newTask);
    
    // 5. 排序任务列表（按开始时间升序）
    sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) {
        return a.startTime < b.startTime;
    });

    cout << "正在添加任务: " << newTask.name << ", ID为: " << newTask.id << endl;
    return true;
}

// 根据ID删除一个任务
bool TaskManager::deleteTask(long long taskId) {
    lock_guard<mutex> lock(tasks_mutex);

    // 1. 在内存中找到并删除任务
    auto it = find_if(tasks.begin(), tasks.end(), [taskId](const Task& task) {
        return task.id == taskId;
    });

    if (it != tasks.end()) {
        tasks.erase(it);
        // 2. 重写整个任务文件
        rewriteTasksFile();
        cout << "成功删除ID为 " << taskId << " 的任务。" << endl;
        return true;
    }

    cerr << "错误: 未找到ID为 " << taskId << " 的任务。" << endl;
    return false;
}

// 获取所有任务的副本
vector<Task> TaskManager::getAllTasks() const {
    lock_guard<mutex> lock(tasks_mutex);
    return tasks;
}

void TaskManager::loadTasks() {
    ifstream file(tasks_file, ios::binary);
    if (!file.is_open()) {
        cout << "用户 " << current_user << " 没有已存在的任务文件。将创建一个新的。" << endl;
        return;
    }

    tasks.clear();
    long long max_id = 0;

    while (file.peek() != EOF && !file.fail()) {
        Task t;
        file.read(reinterpret_cast<char*>(&t.id), sizeof(t.id));
        file.read(reinterpret_cast<char*>(&t.startTime), sizeof(t.startTime));
        file.read(reinterpret_cast<char*>(&t.duration), sizeof(t.duration));
        file.read(reinterpret_cast<char*>(&t.priority), sizeof(t.priority));
        file.read(reinterpret_cast<char*>(&t.category), sizeof(t.category));
        file.read(reinterpret_cast<char*>(&t.reminderTime), sizeof(t.reminderTime));
        file.read(reinterpret_cast<char*>(&t.reminded), sizeof(t.reminded));

        if (file.fail()) break;

        size_t name_len;
        file.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
        if (name_len > 0) {
            vector<char> name_buf(name_len);
            file.read(name_buf.data(), name_len);
            t.name.assign(name_buf.data(), name_len);
        }
        if (file.fail()) break;
        
        size_t custom_cat_len;
        file.read(reinterpret_cast<char*>(&custom_cat_len), sizeof(custom_cat_len));
        if (custom_cat_len > 0) {
            vector<char> custom_cat_buf(custom_cat_len);
            file.read(custom_cat_buf.data(), custom_cat_len);
            t.customCategory.assign(custom_cat_buf.data(), custom_cat_len);
        }
        if (file.fail()) break;

        // 读取新增的 reminderOption 字段
        size_t reminder_opt_len;
        file.read(reinterpret_cast<char*>(&reminder_opt_len), sizeof(reminder_opt_len));
        if (reminder_opt_len > 0) {
            vector<char> reminder_opt_buf(reminder_opt_len);
            file.read(reminder_opt_buf.data(), reminder_opt_len);
            t.reminderOption.assign(reminder_opt_buf.data(), reminder_opt_len);
        }
        if (file.fail()) break;

        tasks.push_back(t);
        if (t.id > max_id) {
            max_id = t.id;
        }
    }
    file.close();

    next_id = max_id + 1;

    sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) {
        return a.startTime < b.startTime;
    });

    cout << "已加载 " << tasks.size() << " 个任务。下一个ID是 " << next_id << endl;
}

void TaskManager::saveTask(const Task& task) {
    ofstream file(tasks_file, ios::binary | ios::app);
    if (!file.is_open()) {
        cerr << "错误: 无法打开任务文件进行追加: " << tasks_file << endl;
        return;
    }

    file.write(reinterpret_cast<const char*>(&task.id), sizeof(task.id));
    file.write(reinterpret_cast<const char*>(&task.startTime), sizeof(task.startTime));
    file.write(reinterpret_cast<const char*>(&task.duration), sizeof(task.duration));
    file.write(reinterpret_cast<const char*>(&task.priority), sizeof(task.priority));
    file.write(reinterpret_cast<const char*>(&task.category), sizeof(task.category));
    file.write(reinterpret_cast<const char*>(&task.reminderTime), sizeof(task.reminderTime));
    file.write(reinterpret_cast<const char*>(&task.reminded), sizeof(task.reminded));
    
    size_t name_len = task.name.length();
    file.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
    if (name_len > 0) {
        file.write(task.name.c_str(), name_len);
    }
    
    size_t custom_cat_len = task.customCategory.length();
    file.write(reinterpret_cast<const char*>(&custom_cat_len), sizeof(custom_cat_len));
    if (custom_cat_len > 0) {
        file.write(task.customCategory.c_str(), custom_cat_len);
    }

    // 中文注释: 保存新增的 reminderOption 字段
    size_t reminder_opt_len = task.reminderOption.length();
    file.write(reinterpret_cast<const char*>(&reminder_opt_len), sizeof(reminder_opt_len));
    if (reminder_opt_len > 0) {
        file.write(task.reminderOption.c_str(), reminder_opt_len);
    }

    file.close();
}

void TaskManager::rewriteTasksFile() {
    ofstream file(tasks_file, ios::binary | ios::trunc);
    if (!file.is_open()) {
        cerr << "错误: 无法打开任务文件进行重写: " << tasks_file << endl;
        return;
    }

    for (const auto& task : tasks) {
        file.write(reinterpret_cast<const char*>(&task.id), sizeof(task.id));
        file.write(reinterpret_cast<const char*>(&task.startTime), sizeof(task.startTime));
        file.write(reinterpret_cast<const char*>(&task.duration), sizeof(task.duration));
        file.write(reinterpret_cast<const char*>(&task.priority), sizeof(task.priority));
        file.write(reinterpret_cast<const char*>(&task.category), sizeof(task.category));
        file.write(reinterpret_cast<const char*>(&task.reminderTime), sizeof(task.reminderTime));
        file.write(reinterpret_cast<const char*>(&task.reminded), sizeof(task.reminded));
        
        size_t name_len = task.name.length();
        file.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        if (name_len > 0) {
            file.write(task.name.c_str(), name_len);
        }
        
        size_t custom_cat_len = task.customCategory.length();
        file.write(reinterpret_cast<const char*>(&custom_cat_len), sizeof(custom_cat_len));
        if (custom_cat_len > 0) {
            file.write(task.customCategory.c_str(), custom_cat_len);
        }

        // 中文注释: 保存新增的 reminderOption 字段
        size_t reminder_opt_len = task.reminderOption.length();
        file.write(reinterpret_cast<const char*>(&reminder_opt_len), sizeof(reminder_opt_len));
        if (reminder_opt_len > 0) {
            file.write(task.reminderOption.c_str(), reminder_opt_len);
        }
    }
    file.close();
}

// 启动提醒线程
void TaskManager::startReminderThread() {
    if (m_running) {
        cout << "提醒线程已在运行。" << endl;
        return;
    }
    m_running = true;
    reminder_thread = thread(&TaskManager::reminderCheckLoop, this);
    cout << "提醒线程已启动。" << endl;
}

// 停止提醒线程
void TaskManager::stopReminderThread() {
    if (!m_running) {
        return;
    }
    m_running = false;
    m_cv.notify_one();
    if (reminder_thread.joinable()) {
        reminder_thread.join();
    }
    cout << "提醒线程已停止。" << endl;
}

// 提醒检查循环（后台线程执行函数）
void TaskManager::reminderCheckLoop() {
    cout << "进入提醒检查循环。" << endl;
    
    while (m_running) {
        unique_lock<mutex> lock(tasks_mutex);

        if (m_cv.wait_for(lock, chrono::seconds(30), [this]{ return !m_running.load(); })) {
            break; 
        }

        if (!m_running) break;
        
        vector<Task> reminders_to_fire;
        bool changes_made = false;
        time_t now = time(nullptr);
        
        for (auto& task : tasks) {
            if (task.reminderTime > 0 && task.reminderTime <= now && !task.reminded) {
                task.reminded = true;
                changes_made = true;
                reminders_to_fire.push_back(task);
            }
        }

        if (changes_made) {
            cout << "为 " << reminders_to_fire.size() << " 个任务持久化'已提醒'状态。" << endl;
            rewriteTasksFile();
        }

        lock.unlock();

        for (const auto& task_to_remind : reminders_to_fire) {
            if (reminder_callback) {
                string startTimeStr = ctime(&task_to_remind.startTime);
                if (!startTimeStr.empty() && startTimeStr.back() == '\n') {
                    startTimeStr.pop_back();
                }

                string reminderTimeStr = ctime(&task_to_remind.reminderTime);
                if (!reminderTimeStr.empty() && reminderTimeStr.back() == '\n') {
                    reminderTimeStr.pop_back();
                }

                string msg = task_to_remind.name + " 任务提醒！\n开始时间: " + startTimeStr + 
                             "\n提醒时间: " + reminderTimeStr;
                
                reminder_callback("提醒", msg);
            }
        }
    }
    cout << "退出提醒检查循环。" << endl;
}