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
    Priority priority;
    Category category;
    string customCategory;
    time_t reminderTime;

    bool reminded = false;  // 是否已提醒(modified by 王博宇)
    
    // 缺省构造函数
    Task() : id(-1), startTime(0), priority(Priority::MEDIUM), category(Category::STUDY), reminderTime(0) {}
};