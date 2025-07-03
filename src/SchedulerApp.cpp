#include "SchedulerApp.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <ctime>
using namespace std;

// 辅助函数，将 time_t 转换为字符串
string time_t_to_string(time_t time) {
    if (time == 0) return "N/A"; // 处理未设置的时间
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", localtime(&time));
    return buffer;
}


SchedulerApp::SchedulerApp() : Gtk::Application("org.cpp.scheduler.app") {}

Glib::RefPtr<SchedulerApp> SchedulerApp::create() {
    return Glib::RefPtr<SchedulerApp>(new SchedulerApp());
}

void SchedulerApp::on_activate() {
    try {
        m_builder = Gtk::Builder::create_from_file("GUIDesign.xml");
    } catch (const Glib::FileError& ex) {
        cerr << "FileError: " << ex.what() << endl;
        return;
    } catch (const Gtk::BuilderError& ex) {
        cerr << "BuilderError: " << ex.what() << endl;
        return;
    }

    get_widgets();

    m_refTreeModel = Gtk::ListStore::create(m_Columns);
    task_tree_view->set_model(m_refTreeModel);
    // 添加TreeView的列
    task_tree_view->append_column("ID", m_Columns.m_col_id);
    task_tree_view->append_column("名称", m_Columns.m_col_name);
    task_tree_view->append_column("开始时间", m_Columns.m_col_start_time);
    task_tree_view->append_column("优先级", m_Columns.m_col_priority);
    task_tree_view->append_column("分类", m_Columns.m_col_category);
    task_tree_view->append_column("提醒时间", m_Columns.m_col_reminder_time);
    
    connect_signals();

    add_window(*login_window);
    add_window(*main_window);
    add_window(*register_window);
    
    login_window->show();
}

void SchedulerApp::get_widgets() {
    m_builder->get_widget("login_window", login_window);
    m_builder->get_widget("register_window", register_window);
    m_builder->get_widget("main_window", main_window);
    m_builder->get_widget("help_window", help_window);
    m_builder->get_widget("add_task_dialog", add_task_dialog);
    m_builder->get_widget("login_username_entry", login_username_entry);
    m_builder->get_widget("login_password_entry", login_password_entry);
    m_builder->get_widget("register_username_entry", register_username_entry);
    m_builder->get_widget("register_password_entry", register_password_entry);
    m_builder->get_widget("register_confirm_password_entry", register_confirm_password_entry);
    m_builder->get_widget("task_tree_view", task_tree_view);
    m_builder->get_widget("main_statusbar", main_statusbar);
}

void SchedulerApp::connect_signals() {
    Gtk::Button *login_button = nullptr;
    m_builder->get_widget("login_button", login_button);
    login_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_login_button_clicked));

    Gtk::LinkButton *show_register_button = nullptr;
    m_builder->get_widget("show_register_button", show_register_button);
    show_register_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_show_register_button_clicked));
    
    Gtk::Button *register_button = nullptr;
    m_builder->get_widget("register_button", register_button);
    register_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_register_button_clicked));
    register_window->signal_delete_event().connect([](GdkEventAny*){ return false; });

    Gtk::Button *add_task_button = nullptr;
    m_builder->get_widget("add_task_button", add_task_button);
    add_task_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_add_task_button_clicked));

    Gtk::Button *delete_task_button = nullptr;
    m_builder->get_widget("delete_task_button", delete_task_button);
    delete_task_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_delete_task_button_clicked));

    Gtk::Button *logout_button = nullptr;
    m_builder->get_widget("logout_button", logout_button);
    logout_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_logout_button_clicked));
    
    Gtk::Button *show_help_button = nullptr;
    m_builder->get_widget("show_help_button", show_help_button);
    show_help_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_show_help_button_clicked));

    Gtk::Button *help_close_button = nullptr;
    m_builder->get_widget("help_close_button", help_close_button);
    help_close_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_help_close_button_clicked));
    help_window->signal_delete_event().connect([](GdkEventAny*){ return false; });

    add_task_dialog->signal_response().connect(sigc::mem_fun(*this, &SchedulerApp::on_add_task_dialog_response));
}

// 信号处理函数实现 (已更新)
void SchedulerApp::on_login_button_clicked() {
    string username = login_username_entry->get_text();
    string password = login_password_entry->get_text();

    if (username.empty() || password.empty()) {
        show_message("登录错误", "用户名和密码不能为空。");
        return;
    }

    // 调用更新后的 login 方法
    auto login_result = m_user_manager.login(username, password);
    
    // 使用 switch 处理不同的登录结果
    switch (login_result) {
        case UserManager::LoginResult::SUCCESS:
            m_current_user = username;
            m_task_manager.setCurrentUser(m_current_user);
            login_password_entry->set_text("");
            login_window->hide();
            main_window->show();
            main_statusbar->push("用户 " + m_current_user + " 已登录", 1);
            update_task_list();
            // TODO: m_task_manager.startReminderThread();
            break;
        case UserManager::LoginResult::USER_NOT_FOUND:
            show_message("登录失败", "用户不存在。");
            break;
        case UserManager::LoginResult::INCORRECT_PASSWORD:
            show_message("登录失败", "密码错误。");
            break;
    }
}

void SchedulerApp::on_show_register_button_clicked() { register_window->show(); }

void SchedulerApp::on_register_button_clicked() {
    string username = register_username_entry->get_text();
    string pass1 = register_password_entry->get_text();
    string pass2 = register_confirm_password_entry->get_text();
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
        register_window->hide();
    } else {
        show_message("注册失败", "用户名可能已被占用。");
    }
}

void SchedulerApp::on_logout_button_clicked() {
    // TODO: m_task_manager.stopReminderThread();
    m_current_user.clear();
    main_window->hide();
    login_window->show();
    main_statusbar->pop(1); // 清除状态栏消息
}

void SchedulerApp::on_add_task_button_clicked() {
    add_task_dialog->run();
}

void SchedulerApp::on_add_task_dialog_response(int response_id) {
    add_task_dialog->hide();
    if(response_id == Gtk::RESPONSE_OK) {
        cout << "OK button clicked on Add Task dialog." << endl;
        // TODO: 从对话框获取数据并调用 m_task_manager.addTask()
        // TODO: 调用 update_task_list() 刷新
    } else {
        cout << "Cancel button clicked on Add Task dialog." << endl;
    }
}

void SchedulerApp::on_delete_task_button_clicked() {
    auto selection = task_tree_view->get_selection();
    if (auto iter = selection->get_selected()) {
        long long id = (*iter)[m_Columns.m_col_id];

        if(m_task_manager.deleteTask(id)) {
            m_refTreeModel->erase(iter);
            show_message("成功", "任务已删除。");
        } else {
            show_message("失败", "删除任务时发生错误。");
        }
    } else {
        show_message("提示", "请先选择一个要删除的任务。");
    }
}

void SchedulerApp::on_show_help_button_clicked() { help_window->show(); }
void SchedulerApp::on_help_close_button_clicked() { help_window->hide(); }

void SchedulerApp::update_task_list() {
    cout << "Updating task list view..." << endl;
    m_refTreeModel->clear(); 

    vector<Task> tasks = m_task_manager.getAllTasks();

    for(const auto& task : tasks) {
        Gtk::TreeModel::Row row = *(m_refTreeModel->append());
        row[m_Columns.m_col_id] = task.id;
        row[m_Columns.m_col_name] = task.name;
        
        row[m_Columns.m_col_start_time] = time_t_to_string(task.startTime);
        row[m_Columns.m_col_reminder_time] = time_t_to_string(task.reminderTime);

        // TODO: 将 Priority 和 Category 枚举转换为字符串
        row[m_Columns.m_col_priority] = "中"; // 占位符
        row[m_Columns.m_col_category] = "学习"; // 占位符
    }
}

void SchedulerApp::show_message(const string& title, const string& msg) {
    // 确保弹窗在正确的父窗口上
    Gtk::Window* parent_window = login_window;
    if (main_window && main_window->is_visible()) {
        parent_window = main_window;
    } else if (register_window && register_window->is_visible()) {
        parent_window = register_window;
    }
    Gtk::MessageDialog dialog(*parent_window, msg, false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
    dialog.set_title(title);
    dialog.run();
}