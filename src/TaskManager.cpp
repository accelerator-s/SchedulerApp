#include "TaskManager.h"
// #include "SchedulerApp.h" // TaskManager不应该依赖UI层，最好去掉
#include <iostream>
#include <algorithm>
#include <fstream>
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>
#include <mutex>
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
    
    cout << "Task manager set for user: " << username << ". Tasks will be loaded from " << tasks_file << endl;
}

// 添加一个新任务
bool TaskManager::addTask(const Task& task) {
    lock_guard<mutex> lock(tasks_mutex);

    // 1. 检查任务名称+开始时间是否唯一
    for (const auto& existing_task : tasks) {
        if (existing_task.name == task.name && existing_task.startTime == task.startTime) {
            cerr << "Error: A task with the same name and start time already exists." << endl;
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

    cout << "Adding task: " << newTask.name << " with ID: " << newTask.id << endl;
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
        cout << "Successfully deleted task with ID: " << taskId << endl;
        return true;
    }

    cerr << "Error: Task with ID " << taskId << " not found." << endl;
    return false;
}

// 获取所有任务的副本
vector<Task> TaskManager::getAllTasks() const {
    lock_guard<mutex> lock(tasks_mutex);
    // 任务列表在添加时已经排序，直接返回副本
    return tasks;
}

void TaskManager::loadTasks() {
    ifstream file(tasks_file, ios::binary);
    if (!file.is_open()) {
        cout << "No existing task file for user " << current_user << ". A new one will be created." << endl;
        return;
    }

    tasks.clear();
    long long max_id = 0;

    while (file.peek() != EOF && !file.fail()) {
        Task t;
        file.read(reinterpret_cast<char*>(&t.id), sizeof(t.id));
        file.read(reinterpret_cast<char*>(&t.startTime), sizeof(t.startTime));
        file.read(reinterpret_cast<char*>(&t.priority), sizeof(t.priority));
        file.read(reinterpret_cast<char*>(&t.category), sizeof(t.category));
        file.read(reinterpret_cast<char*>(&t.reminderTime), sizeof(t.reminderTime));

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

    cout << "Loaded " << tasks.size() << " tasks. Next ID is " << next_id << endl;
}

void TaskManager::saveTask(const Task& task) {
    ofstream file(tasks_file, ios::binary | ios::app);
    if (!file.is_open()) {
        cerr << "Error: Could not open task file for appending: " << tasks_file << endl;
        return;
    }

    file.write(reinterpret_cast<const char*>(&task.id), sizeof(task.id));
    file.write(reinterpret_cast<const char*>(&task.startTime), sizeof(task.startTime));
    file.write(reinterpret_cast<const char*>(&task.priority), sizeof(task.priority));
    file.write(reinterpret_cast<const char*>(&task.category), sizeof(task.category));
    file.write(reinterpret_cast<const char*>(&task.reminderTime), sizeof(task.reminderTime));
    
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

    file.close();
}

void TaskManager::rewriteTasksFile() {
    ofstream file(tasks_file, ios::binary | ios::trunc);
    if (!file.is_open()) {
        cerr << "Error: Could not open task file for rewriting: " << tasks_file << endl;
        return;
    }

    for (const auto& task : tasks) {
        file.write(reinterpret_cast<const char*>(&task.id), sizeof(task.id));
        file.write(reinterpret_cast<const char*>(&task.startTime), sizeof(task.startTime));
        file.write(reinterpret_cast<const char*>(&task.priority), sizeof(task.priority));
        file.write(reinterpret_cast<const char*>(&task.category), sizeof(task.category));
        file.write(reinterpret_cast<const char*>(&task.reminderTime), sizeof(task.reminderTime));
        
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
    }
    file.close();
}

// --- 守护进程部分 ---(modified by 王博宇)

// 启动提醒线程
void TaskManager::startReminderThread() {
    if (m_running) {
        cout << "Reminder thread is already running." << endl;
        return;
    }

    // 设置运行标志为true，创建并启动线程
    m_running = true;
    reminder_thread = thread(&TaskManager::reminderCheckLoop, this);
    cout << "Reminder thread started." << endl;
}

// 停止提醒线程
void TaskManager::stopReminderThread() {
    if (!m_running) {
        return;
    }

    // 1. 设置运行标志为false
    m_running = false;

    // 2. **唤醒**可能正在睡眠的后台线程
    m_cv.notify_one();

    // 3. 等待线程安全退出。因为线程已被唤醒，join()会很快返回。
    if (reminder_thread.joinable()) {
        reminder_thread.join();
    }
    cout << "Reminder thread stopped." << endl;
}

// 提醒检查循环（后台线程执行函数）
void TaskManager::reminderCheckLoop() {
    cout << "Reminder check loop entered." << endl;
    
    // 循环直到 m_running 为 false
    while (m_running) {
<<<<<<< HEAD
        // 1. 休眠30秒（可调整检查间隔）
        this_thread::sleep_for(chrono::seconds(5));
=======
        // 使用 unique_lock，因为 condition_variable 需要它
        unique_lock<mutex> lock(tasks_mutex);
>>>>>>> db775d4e398de3b2778aff80a0b74f2c1814ef1c

        // 1. 等待30秒，或者被 stopReminderThread 唤醒
        // m_cv.wait_for() 会检查 m_running 标志。如果被唤醒且 m_running 是 false，它会立即返回 true。
        // 如果是超时，它会返回 false。
        if (m_cv.wait_for(lock, chrono::seconds(30), [this]{ return !m_running.load(); })) {
            // 如果是因为 m_running 变为 false 而被唤醒，则退出循环
            break; 
        }

        // 如果是超时唤醒，锁仍被持有，可以安全地检查任务
        
        // 2. 检查运行标志（双重检查，防止在等待和检查之间状态改变）
        if (!m_running) break;

        // 3. 遍历任务检查提醒时间（无需再复制，因为一直持有锁）
        time_t now = time(nullptr);
        for (auto& task : tasks) {
            // 检查条件：提醒时间有效 + 已到期 + 未提醒过
            if (task.reminderTime > 0 && task.reminderTime <= now && !task.reminded) {
<<<<<<< HEAD
                // 显示提醒信息
                //showReminder(task);

                if (reminder_callback) {
                    std::string msg = task.name + " 任务提醒！\n开始时间: " + 
                                     ctime(&task.startTime) + "提醒时间: " + ctime(&task.reminderTime);
                    reminder_callback("提醒", msg);  // 触发回调

                /*SchedulerApp s;
                s.show_message("提醒", task.name + " 任务提醒！\n开始时间: " + 
                             ctime(&task.startTime) + "提醒时间: " + ctime(&task.reminderTime));*/
                // 标记任务为已提醒（线程安全地更新）
                lock_guard<mutex> lock(tasks_mutex);
                // 再次查找任务（防止期间任务被删除）
                auto it = find_if(tasks.begin(), tasks.end(), 
                                 [&task](const Task& t) { return t.id == task.id; });
                if (it != tasks.end()) {
                    it->reminded = true;
                }
=======
                // 显示提醒信息（可以在这里调用一个线程安全的回调函数来通知UI）
                cout << "Reminder for task: " << task.name << " for user: " << current_user << endl;
                
                // 标记任务为已提醒
                task.reminded = true;
>>>>>>> db775d4e398de3b2778aff80a0b74f2c1814ef1c
            }
        }
        
        // 当循环迭代结束时，lock 会自动释放互斥锁
    }
<<<<<<< HEAD
}
}


    
=======
    cout << "Reminder check loop exited." << endl;
}
>>>>>>> db775d4e398de3b2778aff80a0b74f2c1814ef1c
