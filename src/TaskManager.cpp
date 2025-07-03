#include "TaskManager.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <mutex>

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

// --- 守护进程部分（占位符） ---

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
}