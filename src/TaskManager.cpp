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
#include <sstream> // 新增：用于命令拼接和输出
#include <SFML/Audio.hpp>
#include <unistd.h> // 用于getcwd（Linux/macOS
#include <cstdlib>  // 用于system函数

#ifdef EMBEDDED_RESOURCES_ENABLED
#include "embedded_resources.h"
#endif

using namespace std;
// 获取当前工作目录（跨平台）
string getCurrentWorkingDir()
{
    char buffer[1024];
#ifdef _WIN32
    if (_getcwd(buffer, sizeof(buffer)) != nullptr)
    {
        return string(buffer);
    }
#else
    if (getcwd(buffer, sizeof(buffer)) != nullptr)
    {
        return string(buffer);
    }
#endif
    return "未知目录";
}

// 在新线程中播放MP3（使用mpg123命令行工具）
void playMp3InThread(const string &mp3Path)
{
    try
    {
        // 检查文件是否存在
        ifstream fileCheck(mp3Path);
        if (!fileCheck.good())
        {
            cerr << "音频文件不存在: " << mp3Path << endl;
            cerr << "当前工作目录: " << getCurrentWorkingDir() << endl;
            return;
        }

        // 跨平台播放命令（使用mpg123）
        string cmd;
#ifdef _WIN32
        // Windows: 假设mpg123已添加到环境变量，或使用绝对路径
        cmd = "mpg123 \"" + mp3Path + "\" 2>&1";
#else
        // Linux/macOS: 直接使用系统安装的mpg123
        cmd = "mpg123 \"" + mp3Path + "\" 2>&1";
#endif

        // 执行命令并输出结果（方便调试）
        cout << "执行播放命令: " << cmd << endl;
        int exitCode = system(cmd.c_str());
        if (exitCode != 0)
        {
            cerr << "音频播放失败，退出码: " << exitCode << endl;
        }
        else
        {
            cout << "音频播放完成" << endl;
        }
    }
    catch (const exception &e)
    {
        cerr << "播放线程错误: " << e.what() << endl;
    }
}

// 启动音频播放线程（外部调用入口）
void TaskManager::playNotificationSound()
{
#ifdef EMBEDDED_RESOURCES_ENABLED
    // 使用嵌入的音频资源
    thread mp3Thread([]()
                     {
        try {
            // 创建临时文件路径
            string tempPath = "/tmp/notification_temp.mp3";
            
            // 将嵌入的音频数据写入临时文件
            if (EmbeddedResources::writeAudioToTempFile(tempPath)) {
                playMp3InThread(tempPath);
                
                // 播放完成后删除临时文件
                std::remove(tempPath.c_str());
            } else {
                cerr << "无法创建临时音频文件" << endl;
            }
        } catch (const exception &e) {
            cerr << "嵌入音频播放错误: " << e.what() << endl;
        } });
    mp3Thread.detach();
#else
    // 使用外部音频文件
    const string mp3Path = "notification.mp3";

    // 启动新线程播放，避免阻塞
    thread mp3Thread(playMp3InThread, mp3Path);
    mp3Thread.detach(); // 线程后台运行，无需等待
#endif
}

// TaskManager 构造函数
TaskManager::TaskManager() : next_id(1), m_running(false)
{
    // 互斥锁、条件变量和线程对象会自动默认构造
    // 原子布尔值 m_running 初始化为 false
}

// 设置当前用户，加载其任务列表
void TaskManager::setCurrentUser(const string &username)
{
    lock_guard<mutex> lock(tasks_mutex);

    current_user = username;
    tasks_file = username + "_tasks.dat";
    tasks.clear();
    next_id = 1;

    // 1. 正常加载任务
    loadTasks();

    // 修复程序关闭期间错过提醒的BUG
    // 2. 清理过期的、未提醒的任务
    time_t now = time(nullptr);
    bool changes_made = false;
    for (auto &task : tasks)
    {
        // 条件：需要提醒 + 提醒时间已过去 + 未提醒
        if (task.reminderTime > 0 && task.reminderTime <= now && !task.reminded)
        {
            task.reminded = true; // 将其标记为已提醒
            changes_made = true;  // 标记有变动，需要存盘
            cout << "过期提醒: 任务 '" << task.name << "' (ID: " << task.id << ") 未能成功提醒，被强制标记为已提醒。" << endl;
        }
    }

    // 3. 如果有任何状态被更新，立即重写文件以持久化
    if (changes_made)
    {
        rewriteTasksFile();
    }

    cout << "任务管理器已为用户 " << username << " 设置。任务将从 " << tasks_file << " 加载。" << endl;
}

// 添加一个新任务
bool TaskManager::addTask(const Task &task)
{
    lock_guard<mutex> lock(tasks_mutex);

    // 只检查完全相同的任务（同名且同开始时间）
    for (const auto &existing_task : tasks)
    {
        if (existing_task.name == task.name && existing_task.startTime == task.startTime)
        {
            cerr << "错误: 一个同名且同开始时间的任务已存在。" << endl;
            return false;
        }
    }

    Task newTask = task;
    newTask.id = next_id++;
    tasks.push_back(newTask);
    saveTask(newTask);
    sort(tasks.begin(), tasks.end(), [](const Task &a, const Task &b)
         { return a.startTime < b.startTime; });

    cout << "正在添加任务: " << newTask.name << ", ID为: " << newTask.id << endl;
    return true;
}

// 根据ID删除一个任务
bool TaskManager::deleteTask(long long taskId)
{
    lock_guard<mutex> lock(tasks_mutex);

    auto it = find_if(tasks.begin(), tasks.end(), [taskId](const Task &task)
                      { return task.id == taskId; });

    if (it != tasks.end())
    {
        tasks.erase(it);
        rewriteTasksFile();
        cout << "成功删除ID为 " << taskId << " 的任务。" << endl;
        return true;
    }

    cerr << "错误: 未找到ID为 " << taskId << " 的任务。" << endl;
    return false;
}

// 根据ID修改任务
bool TaskManager::updateTask(const Task &task)
{
    lock_guard<mutex> lock(tasks_mutex);

    auto it = find_if(tasks.begin(), tasks.end(), [task](const Task &t)
                      { return t.id == task.id; });

    if (it != tasks.end())
    {
        *it = task; // 用新任务替换旧任务
        // 重新排序任务列表
        sort(tasks.begin(), tasks.end(), [](const Task &a, const Task &b)
             { return a.startTime < b.startTime; });
        rewriteTasksFile(); // 重写整个文件
        cout << "成功修改ID为 " << task.id << " 的任务。" << endl;
        return true;
    }

    cerr << "错误: 未找到ID为 " << task.id << " 的任务。" << endl;
    return false;
}

// 根据ID获取任务指针
Task *TaskManager::getTaskById(long long taskId)
{
    lock_guard<mutex> lock(tasks_mutex);

    auto it = find_if(tasks.begin(), tasks.end(), [taskId](const Task &task)
                      { return task.id == taskId; });

    if (it != tasks.end())
    {
        return &(*it);
    }

    return nullptr;
}

// 获取所有任务的副本
vector<Task> TaskManager::getAllTasks() const
{
    lock_guard<mutex> lock(tasks_mutex);
    return tasks;
}

void TaskManager::loadTasks()
{
    ifstream file(tasks_file, ios::binary);
    if (!file.is_open())
    {
        cout << "用户 " << current_user << " 没有已存在的任务文件。将创建一个新的。" << endl;
        return;
    }

    tasks.clear();
    long long max_id = 0;

    while (file.peek() != EOF && !file.fail())
    {
        Task t;
        file.read(reinterpret_cast<char *>(&t.id), sizeof(t.id));
        file.read(reinterpret_cast<char *>(&t.startTime), sizeof(t.startTime));
        file.read(reinterpret_cast<char *>(&t.duration), sizeof(t.duration));
        file.read(reinterpret_cast<char *>(&t.priority), sizeof(t.priority));
        file.read(reinterpret_cast<char *>(&t.category), sizeof(t.category));
        file.read(reinterpret_cast<char *>(&t.reminderTime), sizeof(t.reminderTime));
        file.read(reinterpret_cast<char *>(&t.reminded), sizeof(t.reminded));

        if (file.fail())
            break;

        size_t name_len;
        file.read(reinterpret_cast<char *>(&name_len), sizeof(name_len));
        if (name_len > 0)
        {
            vector<char> name_buf(name_len);
            file.read(name_buf.data(), name_len);
            t.name.assign(name_buf.data(), name_len);
        }
        if (file.fail())
            break;

        size_t custom_cat_len;
        file.read(reinterpret_cast<char *>(&custom_cat_len), sizeof(custom_cat_len));
        if (custom_cat_len > 0)
        {
            vector<char> custom_cat_buf(custom_cat_len);
            file.read(custom_cat_buf.data(), custom_cat_len);
            t.customCategory.assign(custom_cat_buf.data(), custom_cat_len);
        }
        if (file.fail())
            break;

        size_t reminder_opt_len;
        file.read(reinterpret_cast<char *>(&reminder_opt_len), sizeof(reminder_opt_len));
        if (reminder_opt_len > 0)
        {
            vector<char> reminder_opt_buf(reminder_opt_len);
            file.read(reminder_opt_buf.data(), reminder_opt_len);
            t.reminderOption.assign(reminder_opt_buf.data(), reminder_opt_len);
        }
        if (file.fail())
            break;

        tasks.push_back(t);
        if (t.id > max_id)
        {
            max_id = t.id;
        }
    }
    file.close();

    next_id = max_id + 1;

    sort(tasks.begin(), tasks.end(), [](const Task &a, const Task &b)
         { return a.startTime < b.startTime; });

    cout << "已加载 " << tasks.size() << " 个任务。下一个ID是 " << next_id << endl;
}

void TaskManager::saveTask(const Task &task)
{
    ofstream file(tasks_file, ios::binary | ios::app);
    if (!file.is_open())
    {
        cerr << "错误: 无法打开任务文件进行追加: " << tasks_file << endl;
        return;
    }

    file.write(reinterpret_cast<const char *>(&task.id), sizeof(task.id));
    file.write(reinterpret_cast<const char *>(&task.startTime), sizeof(task.startTime));
    file.write(reinterpret_cast<const char *>(&task.duration), sizeof(task.duration));
    file.write(reinterpret_cast<const char *>(&task.priority), sizeof(task.priority));
    file.write(reinterpret_cast<const char *>(&task.category), sizeof(task.category));
    file.write(reinterpret_cast<const char *>(&task.reminderTime), sizeof(task.reminderTime));
    file.write(reinterpret_cast<const char *>(&task.reminded), sizeof(task.reminded));

    size_t name_len = task.name.length();
    file.write(reinterpret_cast<const char *>(&name_len), sizeof(name_len));
    if (name_len > 0)
    {
        file.write(task.name.c_str(), name_len);
    }

    size_t custom_cat_len = task.customCategory.length();
    file.write(reinterpret_cast<const char *>(&custom_cat_len), sizeof(custom_cat_len));
    if (custom_cat_len > 0)
    {
        file.write(task.customCategory.c_str(), custom_cat_len);
    }

    size_t reminder_opt_len = task.reminderOption.length();
    file.write(reinterpret_cast<const char *>(&reminder_opt_len), sizeof(reminder_opt_len));
    if (reminder_opt_len > 0)
    {
        file.write(task.reminderOption.c_str(), reminder_opt_len);
    }

    file.close();
}

void TaskManager::rewriteTasksFile()
{
    ofstream file(tasks_file, ios::binary | ios::trunc);
    if (!file.is_open())
    {
        cerr << "错误: 无法打开任务文件进行重写: " << tasks_file << endl;
        return;
    }

    for (const auto &task : tasks)
    {
        file.write(reinterpret_cast<const char *>(&task.id), sizeof(task.id));
        file.write(reinterpret_cast<const char *>(&task.startTime), sizeof(task.startTime));
        file.write(reinterpret_cast<const char *>(&task.duration), sizeof(task.duration));
        file.write(reinterpret_cast<const char *>(&task.priority), sizeof(task.priority));
        file.write(reinterpret_cast<const char *>(&task.category), sizeof(task.category));
        file.write(reinterpret_cast<const char *>(&task.reminderTime), sizeof(task.reminderTime));
        file.write(reinterpret_cast<const char *>(&task.reminded), sizeof(task.reminded));

        size_t name_len = task.name.length();
        file.write(reinterpret_cast<const char *>(&name_len), sizeof(name_len));
        if (name_len > 0)
        {
            file.write(task.name.c_str(), name_len);
        }

        size_t custom_cat_len = task.customCategory.length();
        file.write(reinterpret_cast<const char *>(&custom_cat_len), sizeof(custom_cat_len));
        if (custom_cat_len > 0)
        {
            file.write(task.customCategory.c_str(), custom_cat_len);
        }

        size_t reminder_opt_len = task.reminderOption.length();
        file.write(reinterpret_cast<const char *>(&reminder_opt_len), sizeof(reminder_opt_len));
        if (reminder_opt_len > 0)
        {
            file.write(task.reminderOption.c_str(), reminder_opt_len);
        }
    }
    file.close();
}

// 启动提醒线程
void TaskManager::startReminderThread()
{
    if (m_running)
    {
        cout << "提醒线程已在运行。" << endl;
        return;
    }
    m_running = true;
    reminder_thread = thread(&TaskManager::reminderCheckLoop, this);
    cout << "提醒线程已启动。" << endl;
}

// 停止提醒线程
void TaskManager::stopReminderThread()
{
    if (!m_running)
    {
        return;
    }
    m_running = false;
    m_cv.notify_one();
    if (reminder_thread.joinable())
    {
        reminder_thread.join();
    }
    cout << "提醒线程已停止。" << endl;
}

// 提醒检查循环（后台线程执行函数）
void TaskManager::reminderCheckLoop()
{
    cout << "进入提醒检查循环。" << endl;

    while (m_running)
    {
        unique_lock<mutex> lock(tasks_mutex);

        // 后台轮询间隔：5s
        if (m_cv.wait_for(lock, chrono::seconds(5), [this]
                          { return !m_running.load(); }))
        {
            break;
        }

        if (!m_running)
            break;

        vector<Task> reminders_to_fire;
        bool changes_made = false;
        time_t now = time(nullptr);

        for (auto &task : tasks)
        {
            if (task.reminderTime > 0 && task.reminderTime <= now && !task.reminded)
            {
                task.reminded = true;
                changes_made = true;
                reminders_to_fire.push_back(task);
            }
        }

        if (changes_made)
        {
            cout << "为 " << reminders_to_fire.size() << " 个任务持久化'已提醒'状态。" << endl;
            rewriteTasksFile();
        }

        lock.unlock();

        for (const auto &task_to_remind : reminders_to_fire)
        {
            if (reminder_callback)
            {
                string startTimeStr = ctime(&task_to_remind.startTime);
                if (!startTimeStr.empty() && startTimeStr.back() == '\n')
                {
                    startTimeStr.pop_back();
                }

                string reminderTimeStr = ctime(&task_to_remind.reminderTime);
                if (!reminderTimeStr.empty() && reminderTimeStr.back() == '\n')
                {
                    reminderTimeStr.pop_back();
                }

                string msg = task_to_remind.name + " 任务提醒！\n开始时间: " + startTimeStr +
                             "\n提醒时间: " + reminderTimeStr;

                reminder_callback("提醒", msg);
                // 创建一个新的线程来播放音频

                playNotificationSound();
            }
        }
    }
    cout << "退出提醒检查循环。" << endl;
}