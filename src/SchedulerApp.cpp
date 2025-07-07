#include "SchedulerApp.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <ctime>
#include <string>
#include <regex>

using namespace std;

// 辅助函数，将 time_t 转换为字符串
string time_t_to_string(time_t time) {
    if (time == 0) return "N/A";
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", localtime(&time));
    return buffer;
}

// 辅助函数，将 Priority 枚举转换为字符串
string SchedulerApp::priority_to_string(Priority p) {
    switch (p) {
        case Priority::HIGH:   return "高";
        case Priority::MEDIUM: return "中";
        case Priority::LOW:    return "低";
        default:               return "未知";
    }
}

// 辅助函数，将 Category 枚举转换为字符串
string SchedulerApp::category_to_string(const Task& task) {
    switch (task.category) {
        case Category::STUDY:         return "学习";
        case Category::ENTERTAINMENT: return "娱乐";
        case Category::LIFE:          return "生活";
        case Category::OTHER:         return task.customCategory;
        default:                      return "未知";
    }
}

SchedulerApp::SchedulerApp() : Gtk::Application("org.cpp.scheduler.app") {}

Glib::RefPtr<SchedulerApp> SchedulerApp::create() {
    return Glib::RefPtr<SchedulerApp>(new SchedulerApp());
}

void SchedulerApp::on_activate() {
    try {
        m_builder = Gtk::Builder::create_from_file("GUIDesign.xml");
        
        get_widgets();
        
        if (task_tree_view) {
            m_refTreeModel = Gtk::ListStore::create(m_Columns);
            task_tree_view->set_model(m_refTreeModel);
        }
        
        connect_signals();

        if(login_window) add_window(*login_window);
        if(main_window) add_window(*main_window);
        if(register_window) add_window(*register_window);
        if(change_password_window) add_window(*change_password_window); // 新增
        
        if(login_window) login_window->show();

    } catch (const Glib::FileError& ex) {
        cerr << "FileError: " << ex.what() << endl;
    } catch (const Gtk::BuilderError& ex) {
        cerr << "BuilderError: " << ex.what() << endl;
    }
}

void SchedulerApp::get_widgets() {
    m_builder->get_widget("login_window", login_window);
    m_builder->get_widget("register_window", register_window);
    m_builder->get_widget("main_window", main_window);
    m_builder->get_widget("help_window", help_window);
    m_builder->get_widget("change_password_window", change_password_window); // 新增
    m_builder->get_widget("login_username_entry", login_username_entry);
    m_builder->get_widget("login_password_entry", login_password_entry);
    m_builder->get_widget("register_username_entry", register_username_entry);
    m_builder->get_widget("register_password_entry", register_password_entry);
    m_builder->get_widget("register_confirm_password_entry", register_confirm_password_entry);
    // 新增
    m_builder->get_widget("cp_username_entry", cp_username_entry);
    m_builder->get_widget("cp_old_password_entry", cp_old_password_entry);
    m_builder->get_widget("cp_new_password_entry", cp_new_password_entry);
    m_builder->get_widget("cp_confirm_new_password_entry", cp_confirm_new_password_entry);

    m_builder->get_widget("task_tree_view", task_tree_view);
    m_builder->get_widget("main_statusbar", main_statusbar);
    m_builder->get_widget("add_task_dialog", add_task_dialog);
    m_builder->get_widget("task_name_entry", task_name_entry);
    m_builder->get_widget("task_start_time_button", task_start_time_button);
    m_builder->get_widget("task_duration_spin", task_duration_spin);
    m_builder->get_widget("task_end_time_label", task_end_time_label);
    m_builder->get_widget("task_priority_combo", task_priority_combo);
    m_builder->get_widget("task_category_combo", task_category_combo);
    m_builder->get_widget("task_custom_category_entry", task_custom_category_entry);
    m_builder->get_widget("task_reminder_combo", task_reminder_combo);
    m_builder->get_widget("task_reminder_entry", task_reminder_entry);
    if (task_duration_spin) {
        auto adjustment = Gtk::Adjustment::create(30.0, 1.0, 1440.0, 5.0, 30.0, 0.0);
        task_duration_spin->set_adjustment(adjustment);
        task_duration_spin->set_digits(0);
    }
}

void SchedulerApp::connect_signals() {
    Gtk::Button *login_button = nullptr, *register_button = nullptr, *add_task_button = nullptr, 
                  *delete_task_button = nullptr, *logout_button = nullptr, *show_help_button = nullptr, 
                  *help_close_button = nullptr, *add_task_ok_button = nullptr, *add_task_cancel_button = nullptr,
                  *cp_confirm_button = nullptr; // 新增
    Gtk::LinkButton *show_register_button = nullptr, *show_change_password_button = nullptr; // 新增

    m_builder->get_widget("login_button", login_button);
    if(login_button) login_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_login_button_clicked));

    m_builder->get_widget("show_register_button", show_register_button);
    if(show_register_button) show_register_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_show_register_button_clicked));
    
    // 新增
    m_builder->get_widget("show_change_password_button", show_change_password_button);
    if(show_change_password_button) show_change_password_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_show_change_password_button_clicked));
    
    m_builder->get_widget("register_button", register_button);
    if(register_button) register_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_register_button_clicked));
    if(register_window) register_window->signal_delete_event().connect([this](GdkEventAny*){ if(register_window) register_window->hide(); return true; });

    // 新增
    m_builder->get_widget("cp_confirm_button", cp_confirm_button);
    if(cp_confirm_button) cp_confirm_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_cp_confirm_button_clicked));
    if(change_password_window) change_password_window->signal_delete_event().connect([this](GdkEventAny*){ if(change_password_window) change_password_window->hide(); return true; });


    m_builder->get_widget("add_task_button", add_task_button);
    if(add_task_button) add_task_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_add_task_button_clicked));

    m_builder->get_widget("delete_task_button", delete_task_button);
    if(delete_task_button) delete_task_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_delete_task_button_clicked));

    m_builder->get_widget("logout_button", logout_button);
    if(logout_button) logout_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_logout_button_clicked));
    
    m_builder->get_widget("show_help_button", show_help_button);
    if(show_help_button) show_help_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_show_help_button_clicked));

    m_builder->get_widget("help_close_button", help_close_button);
    if(help_close_button) help_close_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_help_close_button_clicked));
    if(help_window) help_window->signal_delete_event().connect([this](GdkEventAny*){ if(help_window) help_window->hide(); return true; });
    
    m_builder->get_widget("add_task_ok_button", add_task_ok_button);
    if(add_task_ok_button) add_task_ok_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_add_task_ok_button_clicked));
    
    m_builder->get_widget("add_task_cancel_button", add_task_cancel_button);
    if(add_task_cancel_button) add_task_cancel_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_add_task_cancel_button_clicked));
    
    if(task_start_time_button) task_start_time_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_task_start_time_button_clicked));
    if(task_duration_spin) task_duration_spin->signal_value_changed().connect(sigc::mem_fun(*this, &SchedulerApp::on_add_task_fields_changed));
    if(task_category_combo) task_category_combo->signal_changed().connect(sigc::mem_fun(*this, &SchedulerApp::on_task_category_combo_changed));
    
    if (task_reminder_entry) {
        task_reminder_entry->signal_focus_in_event().connect(sigc::mem_fun(*this, &SchedulerApp::on_reminder_entry_focus_in));
        task_reminder_entry->signal_focus_out_event().connect(sigc::mem_fun(*this, &SchedulerApp::on_reminder_entry_focus_out));
    }
}

// ... (on_login_button_clicked and other existing functions remain the same)
void SchedulerApp::on_login_button_clicked() {
    if (!login_username_entry || !login_password_entry) return;
    string username = login_username_entry->get_text();
    string password = login_password_entry->get_text();

    if (username.empty() || password.empty()) {
        show_message("登录错误", "用户名和密码不能为空。");
        return;
    }

    auto login_result = m_user_manager.login(username, password);
    switch (login_result) {
        case UserManager::LoginResult::SUCCESS:
            m_current_user = username;
            m_task_manager.setCurrentUser(m_current_user);
            login_password_entry->set_text("");
            if(login_window) login_window->hide();
            if(main_window) main_window->show();
            if(main_statusbar) main_statusbar->push("用户 " + m_current_user + " 已登录", 1);
            update_task_list();
            break;
        case UserManager::LoginResult::USER_NOT_FOUND:
            show_message("登录失败", "用户不存在。");
            break;
        case UserManager::LoginResult::INCORRECT_PASSWORD:
            show_message("登录失败", "密码错误。");
            break;
    }
}

void SchedulerApp::on_show_register_button_clicked() { 
    if(register_window) register_window->show(); 
}

// 新增：显示修改密码窗口
void SchedulerApp::on_show_change_password_button_clicked() {
    if(change_password_window) change_password_window->show();
}

// 新增：处理修改密码逻辑
void SchedulerApp::on_cp_confirm_button_clicked() {
    if (!cp_username_entry || !cp_old_password_entry || !cp_new_password_entry || !cp_confirm_new_password_entry) return;

    string username = cp_username_entry->get_text();
    string old_pass = cp_old_password_entry->get_text();
    string new_pass1 = cp_new_password_entry->get_text();
    string new_pass2 = cp_confirm_new_password_entry->get_text();

    if (username.empty() || old_pass.empty() || new_pass1.empty()) {
        show_message("输入错误", "用户名、原密码和新密码均不能为空。");
        return;
    }

    if (new_pass1 != new_pass2) {
        show_message("输入错误", "两次输入的新密码不一致。");
        return;
    }

    auto result = m_user_manager.changePassword(username, old_pass, new_pass1);
    switch (result) {
        case UserManager::ChangePasswordResult::SUCCESS:
            show_message("成功", "密码修改成功！");
            // 清空输入框并关闭窗口
            cp_username_entry->set_text("");
            cp_old_password_entry->set_text("");
            cp_new_password_entry->set_text("");
            cp_confirm_new_password_entry->set_text("");
            if(change_password_window) change_password_window->hide();
            break;
        case UserManager::ChangePasswordResult::USER_NOT_FOUND:
            show_message("修改失败", "该用户不存在。");
            break;
        case UserManager::ChangePasswordResult::INCORRECT_PASSWORD:
            show_message("修改失败", "原密码错误。");
            break;
    }
}


void SchedulerApp::on_register_button_clicked() {
    if (!register_username_entry || !register_password_entry || !register_confirm_password_entry) return;
    string username = register_username_entry->get_text();
    string pass1 = register_password_entry->get_text();
    string pass2 = register_confirm_password_entry->get_text();


    //modified by wby
    // 新增：密码长度检查

    if (!UserManager::isPasswordValid(pass1)) {
        show_message("注册错误", "密码必须至少6位，且同时包含大小写字母和数字！");
        return; // 验证失败，直接返回
    }
    /*if (pass1.length() < 6) {
        show_message("注册错误", "密码长度必须至少6位。");
        return;
    }

    // 新增：密码复杂度检查（大小写、数字）
    bool hasUpper = false, hasLower = false, hasDigit = false;
    for (char c : pass1) {
        if (isupper(c)) hasUpper = true;
        else if (islower(c)) hasLower = true;
        else if (isdigit(c)) hasDigit = true;
    }
    if (!hasUpper || !hasLower || !hasDigit) {
        show_message("注册错误", "密码必须同时包含大小写字母和数字。");
        return;
    }//modified by wby*/

    // 检查用户名和密码是否为空
    if (username.empty() || pass1.empty()) {
        show_message("注册错误", "用户名和密码不能为空。");
        return;
    }
    if (pass1 != pass2) {
        show_message("注册错误", "两次输入的密码不一致。");
        return;
    }
    if (m_user_manager.registerUser(username, pass1)) {
        show_message("注册成功", "新用户创建成功，请登录。");
        register_username_entry->set_text("");
        register_password_entry->set_text("");
        register_confirm_password_entry->set_text("");
        if(register_window) register_window->hide();
    } else {
        show_message("注册失败", "用户名可能已被占用。");
    }
}

// ... on_logout_button_clicked, on_add_task_button_clicked etc. ...
void SchedulerApp::on_logout_button_clicked() {
    m_current_user.clear();
    if(main_window) main_window->hide();
    if(login_window) login_window->show();
    if(main_statusbar) main_statusbar->pop(1);
}

void SchedulerApp::on_add_task_button_clicked() {
    if (add_task_dialog) {
        reset_add_task_dialog();
        add_task_dialog->show();
    } else {
        show_message("错误", "添加任务对话框未能在XML中找到或加载失败。");
    }
}

void SchedulerApp::on_delete_task_button_clicked() {
    if(!task_tree_view) return;
    auto selection = task_tree_view->get_selection();
    if (auto iter = selection->get_selected()) {
        long long id = (*iter)[m_Columns.m_col_id];
        if(m_task_manager.deleteTask(id)) {
            update_task_list();
            show_message("成功", "任务已删除。");
        } else {
            show_message("失败", "删除任务时发生错误。");
        }
    } else {
        show_message("提示", "请先选择一个要删除的任务。");
    }
}

void SchedulerApp::on_show_help_button_clicked() { 
    if(help_window) help_window->show(); 
}

void SchedulerApp::on_help_close_button_clicked() { 
    if(help_window) help_window->hide(); 
}

void SchedulerApp::on_add_task_ok_button_clicked() {
    if (!task_name_entry || !task_priority_combo || !task_category_combo || !task_custom_category_entry || !task_reminder_combo) {
        show_message("代码错误", "一个或多个对话框控件指针为空。");
        return;
    }
    if (task_reminder_entry && task_reminder_entry->has_focus()) {
        add_task_dialog->grab_focus();
    }

    string name = task_name_entry->get_text();
    if (name.empty()) {
        show_message("输入错误", "任务名称不能为空。"); return;
    }
    if (m_selected_start_time == 0) {
        show_message("输入错误", "请选择一个开始时间。"); return;
    }

    time_t now = time(nullptr);
    if (m_selected_start_time <= now) {
        show_message("时间错误", "任务开始时间必须晚于当前时间。");
        return;
    }

    Task newTask;
    newTask.name = name;
    newTask.startTime = m_selected_start_time;
    
    string priority_str = task_priority_combo->get_active_text();
    if (priority_str == "高") newTask.priority = Priority::HIGH;
    else if (priority_str == "低") newTask.priority = Priority::LOW;
    else newTask.priority = Priority::MEDIUM;
    
    string category_str = task_category_combo->get_active_text();
    if (category_str == "学习") newTask.category = Category::STUDY;
    else if (category_str == "娱乐") newTask.category = Category::ENTERTAINMENT;
    else if (category_str == "生活") newTask.category = Category::LIFE;
    else {
        newTask.category = Category::OTHER;
        newTask.customCategory = task_custom_category_entry->get_text();
        if (newTask.customCategory.empty()) {
            show_message("输入错误", "自定义分类名称不能为空。"); return;
        }
    }

    string reminder_text = task_reminder_combo->get_active_text();
    if (!reminder_text.empty() && reminder_text != "不提醒") {
        try {
            std::smatch match;
            if (std::regex_search(reminder_text, match, std::regex("(\\d+)\\s*分钟前"))) {
                newTask.reminderTime = newTask.startTime - (stoi(match[1].str()) * 60);
            } else if (std::regex_search(reminder_text, match, std::regex("(\\d+)\\s*小时前"))) {
                newTask.reminderTime = newTask.startTime - (stoi(match[1].str()) * 3600);
            } else if (std::regex_search(reminder_text, match, std::regex("(\\d+)\\s*天前"))) {
                 newTask.reminderTime = newTask.startTime - (stoi(match[1].str()) * 86400);
            }
             else { 
                 int minutes = stoi(reminder_text);
                 if (minutes > 0) {
                     newTask.reminderTime = newTask.startTime - (minutes * 60);
                 } else {
                     newTask.reminderTime = 0;
                 }
             }
        } catch (...) { newTask.reminderTime = 0; }

        if (newTask.reminderTime != 0) {
            if (newTask.reminderTime <= now) {
                show_message("时间错误", "提醒时间必须晚于当前时间。");
                return;
            }
            if (newTask.reminderTime >= newTask.startTime) {
                show_message("时间错误", "提醒时间必须早于任务开始时间。");
                return;
            }
        }
    } else { newTask.reminderTime = 0; }
    
    if (m_task_manager.addTask(newTask)) {
        update_task_list();
        show_message("成功", "新任务已添加。");
        if(add_task_dialog) add_task_dialog->hide();
    } else {
        show_message("失败", "添加任务失败，可能存在同名且同开始时间的任务。");
    }
}

void SchedulerApp::on_add_task_cancel_button_clicked() {
    if(add_task_dialog) add_task_dialog->hide();
}

void SchedulerApp::on_task_start_time_button_clicked() {
    if(!add_task_dialog) return;

    time_t initial_time = (m_selected_start_time != 0) ? m_selected_start_time : time(nullptr);
    tm time_info = *localtime(&initial_time);

    Gtk::Dialog calendar_dialog("选择日期", *add_task_dialog, true);
    Gtk::Calendar* calendar = Gtk::manage(new Gtk::Calendar());
    calendar->select_month(time_info.tm_mon, time_info.tm_year + 1900);
    calendar->select_day(time_info.tm_mday);
    calendar_dialog.get_content_area()->pack_start(*calendar);
    calendar_dialog.add_button("确定", Gtk::RESPONSE_OK);
    calendar_dialog.add_button("取消", Gtk::RESPONSE_CANCEL);
    calendar_dialog.show_all();
    
    if (calendar_dialog.run() == Gtk::RESPONSE_OK) {
        unsigned int year, month, day;
        calendar->get_date(year, month, day);
        time_info.tm_year = year - 1900;
        time_info.tm_mon = month;
        time_info.tm_mday = day;
        
        if (get_time_from_user(time_info, calendar_dialog)) {
             m_selected_start_time = mktime(&time_info);
             if(task_start_time_button) task_start_time_button->set_label(time_t_to_string(m_selected_start_time));
             on_add_task_fields_changed();
        }
    }
}

bool SchedulerApp::get_time_from_user(tm& time_struct, Gtk::Window& parent) {
    Gtk::Dialog time_dialog("时间", parent, true);
    Gtk::Grid* grid = Gtk::manage(new Gtk::Grid());
    grid->set_border_width(10);
    grid->set_row_spacing(5);
    grid->set_column_spacing(5);

    Gtk::Label* hour_label = Gtk::manage(new Gtk::Label("小时:"));
    Gtk::SpinButton* hour_spin = Gtk::manage(new Gtk::SpinButton());
    hour_spin->set_adjustment(Gtk::Adjustment::create(time_struct.tm_hour, 0, 23, 1));
    hour_spin->set_digits(0);
    
    Gtk::Label* min_label = Gtk::manage(new Gtk::Label("分钟:"));
    Gtk::SpinButton* min_spin = Gtk::manage(new Gtk::SpinButton());
    min_spin->set_adjustment(Gtk::Adjustment::create(time_struct.tm_min, 0, 59, 1));
    min_spin->set_digits(0);

    grid->attach(*hour_label, 0, 0, 1, 1);
    grid->attach(*hour_spin, 1, 0, 1, 1);
    grid->attach(*min_label, 0, 1, 1, 1);
    grid->attach(*min_spin, 1, 1, 1, 1);

    time_dialog.get_content_area()->pack_start(*grid);
    time_dialog.add_button("确定", Gtk::RESPONSE_OK);
    time_dialog.add_button("取消", Gtk::RESPONSE_CANCEL);
    time_dialog.show_all();

    if (time_dialog.run() == Gtk::RESPONSE_OK) {
        time_struct.tm_hour = hour_spin->get_value_as_int();
        time_struct.tm_min = min_spin->get_value_as_int();
        time_struct.tm_sec = 0;
        return true;
    }
    return false;
}

void SchedulerApp::on_add_task_fields_changed() {
    update_end_time_label();
}

void SchedulerApp::on_task_category_combo_changed() {
    if(task_category_combo && task_custom_category_entry) {
        if (task_category_combo->get_active_id() == "category_other_item") {
            task_custom_category_entry->show();
        } else {
            task_custom_category_entry->hide();
        }
    }
}

bool SchedulerApp::on_reminder_entry_focus_in(GdkEventFocus* /*event*/) {
    if (task_reminder_entry) {
        Glib::signal_timeout().connect_once([this]() {
            task_reminder_entry->select_region(0, -1);
        }, 0);
    }
    return false;
}

bool SchedulerApp::on_reminder_entry_focus_out(GdkEventFocus* /*event*/) {
    if (!task_reminder_entry) {
        return false;
    }

    Glib::ustring text = task_reminder_entry->get_text();
    
    // 去除首尾空格
    const Glib::ustring whitespace = " \t\n\v\f\r";
    const auto first = text.find_first_not_of(whitespace);
    if (first == Glib::ustring::npos) {
        text.clear();
    } else {
        const auto last = text.find_last_not_of(whitespace);
        text = text.substr(first, (last - first + 1));
    }

    if (text.empty()) {
        task_reminder_entry->set_text("不提醒");
        return false;
    }
    
    if (text == "不提醒" || text.find("分钟前") != Glib::ustring::npos || 
        text.find("小时前") != Glib::ustring::npos || text.find("天前") != Glib::ustring::npos) {
        return false;
    }

    try {
        size_t pos = 0;
        long value = std::stol(text.raw(), &pos);

        if (pos != text.bytes()) {
            throw std::invalid_argument("Extra characters found");
        }

        if (value <= 0) {
            task_reminder_entry->set_text("不提醒");
        } else {
            task_reminder_entry->set_text(std::to_string(value) + "分钟前");
        }
    } catch (const std::exception& e) {
        task_reminder_entry->set_text("不提醒");
    }

    return false;
}

void SchedulerApp::update_end_time_label() {
    if (!task_end_time_label || !task_duration_spin) return;
    if (m_selected_start_time == 0) {
        task_end_time_label->set_text("[自动计算]");
        return;
    }
    int duration_minutes = task_duration_spin->get_value_as_int();
    time_t end_time = m_selected_start_time + duration_minutes * 60;
    task_end_time_label->set_text(time_t_to_string(end_time));
}

void SchedulerApp::reset_add_task_dialog() {
    if(task_name_entry) task_name_entry->set_text("");
    m_selected_start_time = 0;
    if(task_start_time_button) task_start_time_button->set_label("选择开始时间...");
    if(task_duration_spin) task_duration_spin->set_value(30);
    if(task_end_time_label) task_end_time_label->set_text("[自动计算]");
    if(task_priority_combo) task_priority_combo->set_active(1);
    if(task_category_combo) task_category_combo->set_active(0);
    if(task_custom_category_entry) {
        task_custom_category_entry->set_text("");
        task_custom_category_entry->hide();
    }
    if(task_reminder_combo) task_reminder_combo->set_active_id("remind_none");
}

void SchedulerApp::update_task_list() {
    if (!m_refTreeModel) return;
    m_refTreeModel->clear(); 
    vector<Task> tasks = m_task_manager.getAllTasks();
    for(const auto& task : tasks) {
        Gtk::TreeModel::Row row = *(m_refTreeModel->append());
        row[m_Columns.m_col_id] = task.id;
        row[m_Columns.m_col_name] = task.name;
        row[m_Columns.m_col_start_time] = time_t_to_string(task.startTime);
        row[m_Columns.m_col_reminder_time] = time_t_to_string(task.reminderTime);
        row[m_Columns.m_col_priority] = priority_to_string(task.priority);
        row[m_Columns.m_col_category] = category_to_string(task);
    }
}

void SchedulerApp::show_message(const string& title, const string& msg) {
    Gtk::Window* parent_window = nullptr;
    if (add_task_dialog && add_task_dialog->is_visible()){
        parent_window = add_task_dialog;
    } else if (main_window && main_window->is_visible()) {
        parent_window = main_window;
    } else if (register_window && register_window->is_visible()) {
        parent_window = register_window;
    } else if (change_password_window && change_password_window->is_visible()) { // 新增
        parent_window = change_password_window;
    } else {
        parent_window = login_window;
    }
    
    if (parent_window) {
        Gtk::MessageDialog dialog(*parent_window, msg, false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
        dialog.set_title(title);
        dialog.run();
    } else {
        cerr << "Could not find a parent window for the message dialog: " << msg << endl;
    }
}