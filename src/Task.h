#pragma once

#include <string>
#include <ctime>

using namespace std;

// 任务优先级枚举
enum class Priority { HIGH, MEDIUM, LOW };

// 任务分类枚举
enum class Category { STUDY, ENTERTAINMENT, LIFE, OTHER };

struct Task {
    long long id;
    string name;
    time_t startTime;
    int duration;
    Priority priority;
    Category category;
    string customCategory;
    time_t reminderTime;
    string reminderOption; // 新增：存储提醒选项的描述，如“15分钟前”
    bool reminded = false;
    
    // 缺省构造函数
    Task() : id(-1), startTime(0), duration(30), priority(Priority::MEDIUM), category(Category::STUDY), reminderTime(0), reminded(false) {}
};