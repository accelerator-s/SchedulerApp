#include "TaskManager.h"
#include "SchedulerApp.h"
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
    //TODO
    // 互斥锁和线程对象会自动默认构造
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
        // 2. 调用 rewriteTasksFile() 重写整个任务文件
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

/**
 * @brief 从文件加载任务。
 * @note 使用二进制格式来处理包含空格等特殊字符的任务名称。
 * 格式：[id][startTime][priority][category][reminderTime][name_len][name_data]
 */
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

        if (file.fail()) break; // 防止文件末尾不完整的记录导致读取失败

        size_t name_len;
        file.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
        
        vector<char> name_buf(name_len);
        file.read(name_buf.data(), name_len);
        t.name.assign(name_buf.data(), name_len);
        
        if (file.fail()) break;

        tasks.push_back(t);
        if (t.id > max_id) {
            max_id = t.id;
        }
    }
    file.close();

    // 更新 next_id 为已存在任务的最大ID + 1
    next_id = max_id + 1;

    // 加载后排序
    sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) {
        return a.startTime < b.startTime;
    });

    cout << "Loaded " << tasks.size() << " tasks. Next ID is " << next_id << endl;
}

/**
 * @brief 将单个任务以追加模式写入文件。
 * @note 格式与 loadTasks 中读取的格式一致。
 */
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
    file.write(task.name.c_str(), name_len);

    file.close();
}

/**
 * @brief 重写任务文件，用于删除或批量更新后。
 */
void TaskManager::rewriteTasksFile() {
    // 以截断模式打开文件，清空原有内容
    ofstream file(tasks_file, ios::binary | ios::trunc);
    if (!file.is_open()) {
        cerr << "Error: Could not open task file for rewriting: " << tasks_file << endl;
        return;
    }

    // 循环写入内存中的所有任务
    for (const auto& task : tasks) {
        file.write(reinterpret_cast<const char*>(&task.id), sizeof(task.id));
        file.write(reinterpret_cast<const char*>(&task.startTime), sizeof(task.startTime));
        file.write(reinterpret_cast<const char*>(&task.priority), sizeof(task.priority));
        file.write(reinterpret_cast<const char*>(&task.category), sizeof(task.category));
        file.write(reinterpret_cast<const char*>(&task.reminderTime), sizeof(task.reminderTime));
        
        size_t name_len = task.name.length();
        file.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        file.write(task.name.c_str(), name_len);
    }
    file.close();
}

/* --- 守护进程部分 ---(modified by 王博宇)

void TaskManager::startReminderThread() {
    // TODO: 创建并启动一个后台线程，该线程运行 reminderCheckLoop()
    cout << "Reminder thread would be started here." << endl;
}

void TaskManager::stopReminderThread() {
    // TODO: 设置原子布尔值为false，并join线程
     cout << "Reminder thread would be stopped here." << endl;
}

void TaskManager::reminderCheckLoop() {
    // TODO: 这是一个循环:
    // 1. sleep一段时间 (例如 30s)
    // 2. 检查 m_running 标志
    // 3. 遍历所有任务，如果 reminderTime 到达且未提醒，则打印提醒信息
}*/
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
    
}

// 停止提醒线程
void TaskManager::stopReminderThread() {
    if (!m_running) {
        
        return;
    }

    // 设置运行标志为false，等待线程结束
    m_running = false;
    if (reminder_thread.joinable()) {
        reminder_thread.join();  // 阻塞等待线程退出
    }
    
}

// 提醒检查循环（后台线程执行函数）
void TaskManager::reminderCheckLoop() {
    // 循环检查直到m_running为false
    while (m_running) {
        // 1. 休眠30秒（可调整检查间隔）
        this_thread::sleep_for(chrono::seconds(30));

        // 2. 检查运行标志（防止休眠期间被终止）
        if (!m_running) break;

        // 3. 线程安全地获取当前任务列表
        vector<Task> current_tasks;
        {
            lock_guard<mutex> lock(tasks_mutex);  // 加锁保护任务列表
            current_tasks = tasks;  // 复制任务列表到局部变量，减少锁持有时间
        }

        // 4. 遍历任务检查提醒时间
        time_t now = time(nullptr);  // 当前时间戳
        for (auto& task : current_tasks) {
            // 检查条件：提醒时间有效 + 未过期 + 未提醒过
            if (task.reminderTime > 0 && task.reminderTime <= now && !task.reminded) {
                // 显示提醒信息
                //showReminder(task);
                SchedulerApp s;
                s.show_message("提醒", task.name + " 任务提醒！\n开始时间: " + 
                             ctime(&task.startTime) + "提醒时间: " + ctime(&task.reminderTime));
                // 标记任务为已提醒（线程安全地更新）
                lock_guard<mutex> lock(tasks_mutex);
                // 再次查找任务（防止期间任务被删除）
                auto it = find_if(tasks.begin(), tasks.end(), 
                                 [&task](const Task& t) { return t.id == task.id; });
                if (it != tasks.end()) {
                    it->reminded = true;
                }
            }
        }
    }
}


    
