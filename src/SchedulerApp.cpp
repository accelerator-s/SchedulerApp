#include "SchedulerApp.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <ctime>
#include <string>
#include <regex>

using namespace std;

// 辅助函数，将 time_t 转换为 Y-m-d H:M 格式的字符串
string time_t_to_datetime_string(time_t time)
{
    if (time == 0)
        return "N/A";
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", localtime(&time));
    return buffer;
}

// 新的智能时间段格式化函数
string SchedulerApp::format_timespan(time_t start_time, time_t end_time)
{
    tm tm_start = *localtime(&start_time);
    tm tm_end = *localtime(&end_time);

    char start_buf[50];
    char end_buf[50];

    bool same_day = (tm_start.tm_year == tm_end.tm_year && tm_start.tm_yday == tm_end.tm_yday);
    bool same_year = (tm_start.tm_year == tm_end.tm_year);

    if (same_day)
    {
        // 同一天: 8:00 - 11:40
        strftime(start_buf, sizeof(start_buf), "%H:%M", &tm_start);
        strftime(end_buf, sizeof(end_buf), "%H:%M", &tm_end);
    }
    else if (same_year)
    {
        // 同年不同天: 7.9 8:00 - 7.10 8:00
        strftime(start_buf, sizeof(start_buf), "%m.%d %H:%M", &tm_start);
        strftime(end_buf, sizeof(end_buf), "%m.%d %H:%M", &tm_end);
    }
    else
    {
        // 不同年: 2025.12.31 8:00 - 2026.1.1 9:00
        strftime(start_buf, sizeof(start_buf), "%Y.%m.%d %H:%M", &tm_start);
        strftime(end_buf, sizeof(end_buf), "%Y.%m.%d %H:%M", &tm_end);
    }

    return string(start_buf) + " - " + string(end_buf);
}

// 辅助函数，将 Priority 枚举转换为字符串
string SchedulerApp::priority_to_string(Priority p)
{
    switch (p)
    {
    case Priority::HIGH:
        return "高";
    case Priority::MEDIUM:
        return "中";
    case Priority::LOW:
        return "低";
    default:
        return "未知";
    }
}

// 辅助函数，将 Category 枚举转换为字符串
string SchedulerApp::category_to_string(const Task &task)
{
    switch (task.category)
    {
    case Category::STUDY:
        return "学习";
    case Category::ENTERTAINMENT:
        return "娱乐";
    case Category::LIFE:
        return "生活";
    case Category::OTHER:
        return task.customCategory;
    default:
        return "未知";
    }
}

// 计算任务状态（未开始、进行中、已结束）
string SchedulerApp::get_task_status(const Task &task, time_t current_time)
{
    if (current_time < task.startTime)
    {
        return "未开始";
    }
    time_t end_time = task.startTime + (task.duration * 60);
    if (current_time < end_time)
    {
        return "进行中";
    }
    else
    {
        return "已结束";
    }
}

// 计算提醒状态（不提醒、已提醒、未提醒）
string SchedulerApp::get_reminder_status(const Task &task)
{
    if (task.reminderTime == 0)
    {
        return "不提醒";
    }
    if (task.reminded)
    {
        return "已提醒";
    }
    return "未提醒";
}

// 构造函数：初始化日期为当前时间
SchedulerApp::SchedulerApp() : Gtk::Application("org.cpp.scheduler.app")
{
    time(&m_displayed_date);
    time(&m_selected_date);
}

// 静态工厂方法，创建应用实例
Glib::RefPtr<SchedulerApp> SchedulerApp::create()
{
    return Glib::RefPtr<SchedulerApp>(new SchedulerApp());
}

// 应用激活时的入口函数
void SchedulerApp::on_activate()
{
    try
    {
        m_builder = Gtk::Builder::create_from_file("GUIDesign.xml");

        get_widgets();

        auto css_provider = Gtk::CssProvider::create();
        css_provider->load_from_data(
            ".today-cell { background-color: alpha(@theme_selected_bg_color, 0.3); border-radius: 5px;}"
            ".other-month-day { color: @theme_unfocused_fg_color; }"
            ".selected-day-cell { border: 2px solid @theme_accent_bg_color; border-radius: 5px; }"
            ".view-button-active { border-bottom-width: 3px; border-bottom-style: solid; border-bottom-color: @theme_accent_bg_color; font-weight: bold; }"
            ".task-label { font-size: small; background-color: alpha(@theme_button_bg_color, 0.7); color: @theme_button_fg_color; border-radius: 10px; padding: 2px 6px; margin-top: 2px; margin-bottom: 2px;}");
        Gtk::StyleContext::add_provider_for_screen(
            Gdk::Screen::get_default(), css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

        if (task_tree_view)
        {
            m_refTreeModel = Gtk::ListStore::create(m_Columns);
            task_tree_view->set_model(m_refTreeModel);
        }

        connect_signals();

        if (login_window)
            add_window(*login_window);
        if (main_window)
            add_window(*main_window);
        if (register_window)
            add_window(*register_window);
        if (change_password_window)
            add_window(*change_password_window);
        if (help_window)
            add_window(*help_window);
        if (add_task_dialog)
            add_window(*add_task_dialog);

        if (login_window)
            login_window->show();
    }
    catch (const Glib::FileError &ex)
    {
        cerr << "文件错误: " << ex.what() << endl;
    }
    catch (const Gtk::BuilderError &ex)
    {
        cerr << "Builder错误: " << ex.what() << endl;
    }
}

// 从 Builder 中获取所有控件指针
void SchedulerApp::get_widgets()
{
    // --- 获取窗口和对话框 ---
    m_builder->get_widget("login_window", login_window);
    m_builder->get_widget("register_window", register_window);
    m_builder->get_widget("main_window", main_window);
    m_builder->get_widget("help_window", help_window);
    m_builder->get_widget("change_password_window", change_password_window);
    m_builder->get_widget("add_task_dialog", add_task_dialog);

    // --- 登录/注册/改密 控件 ---
    m_builder->get_widget("login_username_entry", login_username_entry);
    m_builder->get_widget("login_password_entry", login_password_entry);
    m_builder->get_widget("register_username_entry", register_username_entry);
    m_builder->get_widget("register_password_entry", register_password_entry);
    m_builder->get_widget("register_confirm_password_entry", register_confirm_password_entry);
    m_builder->get_widget("cp_username_entry", cp_username_entry);
    m_builder->get_widget("cp_old_password_entry", cp_old_password_entry);
    m_builder->get_widget("cp_new_password_entry", cp_new_password_entry);
    m_builder->get_widget("cp_confirm_new_password_entry", cp_confirm_new_password_entry);
    m_builder->get_widget("cp_old_password_label", cp_old_password_label);

    // --- "添加任务" 对话框控件 ---
    m_builder->get_widget("task_name_entry", task_name_entry);
    m_builder->get_widget("task_start_time_button", task_start_time_button);
    m_builder->get_widget("task_duration_spin", task_duration_spin);
    m_builder->get_widget("task_end_time_label", task_end_time_label);
    m_builder->get_widget("task_priority_combo", task_priority_combo);
    m_builder->get_widget("task_category_combo", task_category_combo);
    m_builder->get_widget("task_custom_category_entry", task_custom_category_entry);
    m_builder->get_widget("task_reminder_combo", task_reminder_combo);
    m_builder->get_widget("task_reminder_entry", task_reminder_entry);

    // --- 主界面控件 ---
    m_builder->get_widget("settings_button", settings_button);
    m_builder->get_widget("menu_item_logout", menu_item_logout);
    m_builder->get_widget("menu_item_change_password", menu_item_change_password);
    m_builder->get_widget("menu_item_delete_account", menu_item_delete_account);
    m_builder->get_widget("menu_item_help", menu_item_help);

    m_builder->get_widget("main_stack", m_main_stack);
    m_builder->get_widget("month_view_pane", m_month_view_pane);
    m_builder->get_widget("week_view_pane", m_week_view_pane);
    m_builder->get_widget("month_header_grid", m_month_header_grid);
    m_builder->get_widget("month_view_grid", m_month_view_grid);
    m_builder->get_widget("week_header_grid", m_week_header_grid);
    m_builder->get_widget("week_view_grid", m_week_view_grid);
    m_builder->get_widget("date_navigation_box", m_date_navigation_box);
    m_builder->get_widget("current_date_label", m_current_date_label);
    m_builder->get_widget("prev_button", m_prev_button);
    m_builder->get_widget("next_button", m_next_button);
    m_builder->get_widget("month_view_button", m_month_view_button);
    m_builder->get_widget("week_view_button", m_week_view_button);
    m_builder->get_widget("agenda_view_button", m_agenda_view_button);
    m_builder->get_widget("agenda_add_task_button", m_agenda_add_task_button);
    m_builder->get_widget("agenda_delete_task_button", m_agenda_delete_task_button);
    m_builder->get_widget("today_indicator_box", m_today_indicator_box);
    m_builder->get_widget("today_button", m_today_button);
    m_builder->get_widget("week_of_year_label", m_week_of_year_label);
    m_builder->get_widget("selected_day_events_listbox", m_selected_day_events_listbox);
    m_builder->get_widget("week_selected_day_events_listbox", m_week_selected_day_events_listbox);
    m_builder->get_widget("task_tree_view", task_tree_view);

    // --- 右键上下文菜单 ---
    m_builder->get_widget("task_context_menu", m_task_context_menu);
    m_builder->get_widget("empty_space_context_menu", m_empty_space_context_menu);
    m_builder->get_widget("ctx_menu_delete_task", m_ctx_menu_delete_task);
    m_builder->get_widget("ctx_menu_add_task", m_ctx_menu_add_task);

    // 为任务持续时间选择器设置范围和步长
    if (task_duration_spin)
    {
        auto adjustment = Gtk::Adjustment::create(30.0, 1.0, 999999.0, 5.0, 30.0, 0.0);
        task_duration_spin->set_adjustment(adjustment);
        task_duration_spin->set_digits(0);
    }
}

void SchedulerApp::connect_signals()
{
    Gtk::Button *login_button = nullptr, *register_button = nullptr,
                *cp_confirm_button = nullptr, *add_task_ok_button = nullptr, *add_task_cancel_button = nullptr;
    Gtk::LinkButton *show_register_button = nullptr, *show_change_password_button = nullptr;

    // --- 窗口和对话框信号 ---
    m_builder->get_widget("login_button", login_button);
    if (login_button)
        login_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_login_button_clicked));
    m_builder->get_widget("show_register_button", show_register_button);
    if (show_register_button)
        show_register_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_show_register_button_clicked));
    m_builder->get_widget("show_change_password_button", show_change_password_button);
    if (show_change_password_button)
        show_change_password_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_show_change_password_button_clicked));
    m_builder->get_widget("register_button", register_button);
    if (register_button)
        register_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_register_button_clicked));
    if (register_window)
        register_window->signal_delete_event().connect([this](GdkEventAny *)
                                                       { if(register_window) register_window->hide(); return true; });
    m_builder->get_widget("cp_confirm_button", cp_confirm_button);
    if (cp_confirm_button)
        cp_confirm_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_cp_confirm_button_clicked));
    if (change_password_window)
        change_password_window->signal_delete_event().connect([this](GdkEventAny *)
                                                              { if(change_password_window) change_password_window->hide(); return true; });

    // --- "设置"菜单信号 ---
    if (menu_item_logout)
        menu_item_logout->signal_activate().connect(sigc::mem_fun(*this, &SchedulerApp::on_menu_item_logout_activated));
    if (menu_item_change_password)
        menu_item_change_password->signal_activate().connect(sigc::mem_fun(*this, &SchedulerApp::on_menu_item_change_password_activated));
    if (menu_item_delete_account)
        menu_item_delete_account->signal_activate().connect(sigc::mem_fun(*this, &SchedulerApp::on_menu_item_delete_account_activated));
    if (menu_item_help)
        menu_item_help->signal_activate().connect(sigc::mem_fun(*this, &SchedulerApp::on_menu_item_help_activated));

    // --- “添加任务”对话框信号 ---
    m_builder->get_widget("add_task_ok_button", add_task_ok_button);
    if (add_task_ok_button)
        add_task_ok_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_add_task_ok_button_clicked));
    m_builder->get_widget("add_task_cancel_button", add_task_cancel_button);
    if (add_task_cancel_button)
        add_task_cancel_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_add_task_cancel_button_clicked));
    if (task_start_time_button)
        task_start_time_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_task_start_time_button_clicked));
    if (task_duration_spin)
        task_duration_spin->signal_value_changed().connect(sigc::mem_fun(*this, &SchedulerApp::on_add_task_fields_changed));
    if (task_category_combo)
        task_category_combo->signal_changed().connect(sigc::mem_fun(*this, &SchedulerApp::on_task_category_combo_changed));
    if (task_reminder_entry)
    {
        task_reminder_entry->signal_focus_in_event().connect(sigc::mem_fun(*this, &SchedulerApp::on_reminder_entry_focus_in));
        task_reminder_entry->signal_focus_out_event().connect(sigc::mem_fun(*this, &SchedulerApp::on_reminder_entry_focus_out));
    }

    // --- 主界面导航和视图切换信号 ---
    if (m_prev_button)
        m_prev_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_prev_button_clicked));
    if (m_next_button)
        m_next_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_next_button_clicked));
    if (m_today_button)
        m_today_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_today_button_clicked));
    if (m_month_view_button)
        m_month_view_button->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &SchedulerApp::on_view_button_clicked), ViewMode::MONTH));
    if (m_week_view_button)
        m_week_view_button->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &SchedulerApp::on_view_button_clicked), ViewMode::WEEK));
    if (m_agenda_view_button)
        m_agenda_view_button->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &SchedulerApp::on_view_button_clicked), ViewMode::AGENDA));
    if (m_agenda_add_task_button)
        m_agenda_add_task_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_agenda_add_task_button_clicked));
    if (m_agenda_delete_task_button)
        m_agenda_delete_task_button->signal_clicked().connect(sigc::mem_fun(*this, &SchedulerApp::on_agenda_delete_task_button_clicked));

    // --- 右键上下文菜单信号 ---
    if (m_ctx_menu_add_task)
        m_ctx_menu_add_task->signal_activate().connect(sigc::mem_fun(*this, &SchedulerApp::on_ctx_menu_add_task_activated));
    if (m_ctx_menu_delete_task)
        m_ctx_menu_delete_task->signal_activate().connect(sigc::mem_fun(*this, &SchedulerApp::on_ctx_menu_delete_task_activated));

    // 为任务列表和下方任务详情区连接右键点击事件
    if (task_tree_view)
        task_tree_view->signal_button_press_event().connect(sigc::mem_fun(*this, &SchedulerApp::on_tree_view_button_press));
    if (m_selected_day_events_listbox)
        m_selected_day_events_listbox->signal_button_press_event().connect(sigc::bind(sigc::mem_fun(*this, &SchedulerApp::on_list_box_button_press), m_selected_day_events_listbox));
    if (m_week_selected_day_events_listbox)
        m_week_selected_day_events_listbox->signal_button_press_event().connect(sigc::bind(sigc::mem_fun(*this, &SchedulerApp::on_list_box_button_press), m_week_selected_day_events_listbox));

    // 为日程列表视图（Agenda View）设置列
    if (task_tree_view)
    {
        task_tree_view->remove_all_columns();
        task_tree_view->append_column("ID", m_Columns.m_col_id);
        task_tree_view->get_column(0)->set_visible(false);
        int col_index = task_tree_view->append_column("任务名称", m_Columns.m_col_name);
        Gtk::TreeViewColumn *column = task_tree_view->get_column(col_index - 1);
        if (column)
        {
            column->set_expand(true);
        }
        task_tree_view->append_column("任务时段", m_Columns.m_col_timespan);
        task_tree_view->append_column("优先级", m_Columns.m_col_priority);
        task_tree_view->append_column("分类", m_Columns.m_col_category);
        task_tree_view->append_column("提醒设置", m_Columns.m_col_reminder_option);
        task_tree_view->append_column("提醒状态", m_Columns.m_col_reminder_status);
        task_tree_view->append_column("任务状态", m_Columns.m_col_task_status);

        for (auto col : task_tree_view->get_columns())
        {
            if (col != task_tree_view->get_column(1))
            {
                col->set_resizable(true);
            }
        }
    }
}

// “登录”按钮点击事件处理
void SchedulerApp::on_login_button_clicked()
{
    if (!login_username_entry || !login_password_entry)
        return;
    string username = login_username_entry->get_text();
    string password = login_password_entry->get_text();
    if (username.empty() || password.empty())
    {
        show_message("登录错误", "用户名和密码不能为空。");
        return;
    }
    auto login_result = m_user_manager.login(username, password);
    switch (login_result)
    {
    case UserManager::LoginResult::SUCCESS:
        m_current_user = username;
        m_task_manager.setCurrentUser(m_current_user);
        login_password_entry->set_text("");
        if (login_window)
            login_window->hide();
        if (main_window)
        {
            main_window->maximize(); // 需求1：主窗口全屏（最大化）
            main_window->show();
        }

        time(&m_displayed_date);
        time(&m_selected_date);
        m_current_view_mode = ViewMode::MONTH;
        update_all_views();
        on_login_success();
        break;
    case UserManager::LoginResult::USER_NOT_FOUND:
        show_message("登录失败", "用户不存在。");
        break;
    case UserManager::LoginResult::INCORRECT_PASSWORD:
        show_message("登录失败", "密码错误。");
        break;
    }
}

void SchedulerApp::on_show_register_button_clicked()
{
    if (register_window)
    {
        register_window->show();
    }
}

void SchedulerApp::on_show_change_password_button_clicked()
{
    m_is_changing_password_from_main = false;
    if (change_password_window && cp_username_entry && cp_old_password_entry && cp_old_password_label && cp_new_password_entry && cp_confirm_new_password_entry)
    {
        cp_username_entry->set_text("");
        cp_old_password_entry->set_text("");
        cp_new_password_entry->set_text("");
        cp_confirm_new_password_entry->set_text("");
        cp_username_entry->set_sensitive(true);
        cp_old_password_label->show();
        cp_old_password_entry->show();
        change_password_window->show();
        cp_username_entry->grab_focus();
    }
}

void SchedulerApp::on_cp_confirm_button_clicked()
{
    if (!cp_username_entry || !cp_old_password_entry || !cp_new_password_entry || !cp_confirm_new_password_entry)
        return;
    string username = cp_username_entry->get_text();
    string old_pass = cp_old_password_entry->get_text();
    string new_pass1 = cp_new_password_entry->get_text();
    string new_pass2 = cp_confirm_new_password_entry->get_text();
    if (new_pass1 != new_pass2)
    {
        show_message("输入错误", "两次输入的新密码不一致。");
        return;
    }
    UserManager::ChangePasswordResult result;
    if (m_is_changing_password_from_main)
    {
        if (new_pass1.empty())
        {
            show_message("输入错误", "新密码不能为空。");
            return;
        }
        result = m_user_manager.updatePassword(m_current_user, new_pass1);
    }
    else
    {
        if (username.empty() || old_pass.empty() || new_pass1.empty())
        {
            show_message("输入错误", "用户名、原密码和新密码均不能为空。");
            return;
        }
        result = m_user_manager.changePassword(username, old_pass, new_pass1);
    }
    switch (result)
    {
    case UserManager::ChangePasswordResult::SUCCESS:
        show_message("成功", "密码修改成功！");
        if (change_password_window)
            change_password_window->hide();
        break;
    case UserManager::ChangePasswordResult::USER_NOT_FOUND:
        show_message("修改失败", "该用户不存在。");
        break;
    case UserManager::ChangePasswordResult::INCORRECT_PASSWORD:
        show_message("修改失败", "原密码错误。");
        break;
    case UserManager::ChangePasswordResult::INVALID_PASSWORD:
        show_message("修改失败", "密码必须至少6位，且同时包含大小写字母和数字！");
        break;
    }
}

void SchedulerApp::on_register_button_clicked()
{
    if (!register_username_entry || !register_password_entry || !register_confirm_password_entry)
        return;
    string username = register_username_entry->get_text();
    string pass1 = register_password_entry->get_text();
    string pass2 = register_confirm_password_entry->get_text();
    if (username.empty() || pass1.empty())
    {
        show_message("注册错误", "用户名和密码不能为空。");
        return;
    }
    if (!UserManager::isPasswordValid(pass1))
    {
        show_message("注册错误", "密码必须至少6位，且同时包含大小写字母和数字！");
        return;
    }
    if (pass1 != pass2)
    {
        show_message("注册错误", "两次输入的密码不一致。");
        return;
    }
    if (m_user_manager.registerUser(username, pass1))
    {
        show_message("注册成功", "新用户创建成功，请登录。");
        register_username_entry->set_text("");
        register_password_entry->set_text("");
        register_confirm_password_entry->set_text("");
        if (register_window)
            register_window->hide();
    }
    else
    {
        show_message("注册失败", "用户名可能已被占用。");
    }
}

void SchedulerApp::on_agenda_add_task_button_clicked()
{
    if (add_task_dialog)
    {
        reset_add_task_dialog();
        time_t now;
        time(&now);
        m_selected_start_time = now;
        if (task_start_time_button)
            task_start_time_button->set_label(time_t_to_datetime_string(m_selected_start_time));
        on_add_task_fields_changed();
        add_task_dialog->show();
    }
}

void SchedulerApp::on_agenda_delete_task_button_clicked()
{
    if (!task_tree_view)
        return;
    auto selection = task_tree_view->get_selection();
    if (auto iter = selection->get_selected())
    {
        long long id = (*iter)[m_Columns.m_col_id];
        if (m_task_manager.deleteTask(id))
        {
            update_all_views();
            show_message("成功", "任务已删除。");
        }
        else
        {
            show_message("失败", "删除任务时发生错误。");
        }
    }
    else
    {
        show_message("提示", "请先在列表中选择一个要删除的任务。");
    }
}

void SchedulerApp::on_menu_item_logout_activated()
{
    m_task_manager.stopReminderThread();
    if (m_timer_connection)
    {
        m_timer_connection.disconnect();
    }
    m_current_user.clear();
    if (main_window)
        main_window->hide();
    if (login_window)
        login_window->show();
    if (m_refTreeModel)
        m_refTreeModel->clear();
}

void SchedulerApp::on_menu_item_change_password_activated()
{
    m_is_changing_password_from_main = true;
    if (change_password_window && cp_username_entry && cp_old_password_entry && cp_old_password_label && cp_new_password_entry && cp_confirm_new_password_entry)
    {
        cp_old_password_entry->set_text("");
        cp_new_password_entry->set_text("");
        cp_confirm_new_password_entry->set_text("");
        cp_username_entry->set_text(m_current_user);
        cp_username_entry->set_sensitive(false);
        cp_old_password_label->hide();
        cp_old_password_entry->hide();
        change_password_window->show();
        cp_new_password_entry->grab_focus();
    }
}

void SchedulerApp::on_menu_item_delete_account_activated()
{
    if (!main_window)
        return;
    Gtk::MessageDialog dialog(*main_window, "删除账户警告", false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO, true);
    dialog.set_secondary_text("您确定要永久删除当前账户 (" + m_current_user + ") 吗？\n此操作不可恢复，所有相关任务数据都将被清除！");
    int result = dialog.run();
    if (result == Gtk::RESPONSE_YES)
    {
        string user_to_delete = m_current_user;
        on_menu_item_logout_activated();
        if (m_user_manager.deleteUser(user_to_delete) == UserManager::DeleteResult::SUCCESS)
        {
            show_message("成功", "账户 " + user_to_delete + " 已被成功删除。");
        }
        else
        {
            show_message("失败", "删除账户时发生未知错误。");
        }
    }
}

void SchedulerApp::on_menu_item_help_activated()
{
    if (help_window)
    {
        help_window->show();
    }
}

// --- 主界面信号处理函数 ---

void SchedulerApp::on_prev_button_clicked()
{
    tm displayed_tm = *localtime(&m_displayed_date);
    if (m_current_view_mode == ViewMode::MONTH)
    {
        if (displayed_tm.tm_mon == 0)
        {
            displayed_tm.tm_mon = 11;
            displayed_tm.tm_year--;
        }
        else
        {
            displayed_tm.tm_mon--;
        }
    }
    else
    { // WEEK
        displayed_tm.tm_mday -= 7;
    }
    m_displayed_date = mktime(&displayed_tm);
    m_selected_date = m_displayed_date; // 翻页时，默认选中第一天
    update_all_views();
}

void SchedulerApp::on_next_button_clicked()
{
    tm displayed_tm = *localtime(&m_displayed_date);
    if (m_current_view_mode == ViewMode::MONTH)
    {
        if (displayed_tm.tm_mon == 11)
        {
            displayed_tm.tm_mon = 0;
            displayed_tm.tm_year++;
        }
        else
        {
            displayed_tm.tm_mon++;
        }
    }
    else
    { // WEEK
        displayed_tm.tm_mday += 7;
    }
    m_displayed_date = mktime(&displayed_tm);
    m_selected_date = m_displayed_date; // 翻页时，默认选中第一天
    update_all_views();
}

void SchedulerApp::on_today_button_clicked()
{
    time(&m_displayed_date);
    time(&m_selected_date);
    update_all_views();
}

void SchedulerApp::on_view_button_clicked(ViewMode new_mode)
{
    if (m_current_view_mode == new_mode)
        return;
    m_current_view_mode = new_mode;
    update_all_views();
}

// --- 右键菜单处理函数 ---
void SchedulerApp::on_ctx_menu_add_task_activated()
{
    if (add_task_dialog)
    {
        reset_add_task_dialog();
        time_t start_t = (m_context_menu_date != 0) ? m_context_menu_date : time(nullptr);
        tm start_tm = *localtime(&start_t);

        if (m_context_menu_date == 0)
        { // 如果不是通过右键菜单（即通过按钮），则将时间设置为当前时间
            m_selected_start_time = start_t;
        }
        else
        { // 如果是通过右键菜单，默认设置到当天的 08:00
            start_tm.tm_hour = 8;
            start_tm.tm_min = 0;
            start_tm.tm_sec = 0;
            m_selected_start_time = mktime(&start_tm);
        }

        if (task_start_time_button)
            task_start_time_button->set_label(time_t_to_datetime_string(m_selected_start_time));
        on_add_task_fields_changed();
        add_task_dialog->show();
    }
}

void SchedulerApp::on_ctx_menu_delete_task_activated()
{
    if (m_context_menu_task_id != -1)
    {
        if (m_task_manager.deleteTask(m_context_menu_task_id))
        {
            update_all_views();
            show_message("成功", "任务已删除。");
        }
        else
        {
            show_message("失败", "删除任务时发生错误。");
        }
    }
    m_context_menu_task_id = -1; // 重置
}

// --- 核心UI更新函数 ---

void SchedulerApp::update_all_views()
{
    if (!m_main_stack)
        return;

    update_view_specific_layout(); // 首先调整布局

    switch (m_current_view_mode)
    {
    case ViewMode::MONTH:
        m_main_stack->set_visible_child("month_view");
        populate_month_view();
        break;
    case ViewMode::WEEK:
        m_main_stack->set_visible_child("week_view");
        populate_week_view();
        break;
    case ViewMode::AGENDA:
        m_main_stack->set_visible_child("agenda_view");
        update_task_list();
        break;
    }

    update_date_label_and_indicator();
    update_view_switcher_ui();
    update_selected_day_details();
}

void SchedulerApp::update_view_specific_layout()
{
    if (!m_month_view_pane || !m_week_view_pane || !m_date_navigation_box || !main_window)
        return;

    bool is_agenda = (m_current_view_mode == ViewMode::AGENDA);
    m_date_navigation_box->set_visible(!is_agenda);
    m_today_indicator_box->set_visible(!is_agenda);

    if (m_current_view_mode == ViewMode::MONTH)
    {
        m_month_view_pane->set_visible(true);
        m_month_view_pane->set_position(main_window->get_height() / 2);
    }
    if (m_current_view_mode == ViewMode::WEEK)
    {
        m_week_view_pane->set_visible(true);
        m_week_view_pane->set_position(main_window->get_height() / 2);
    }
}

// 填充月视图
void SchedulerApp::populate_month_view()
{
    if (!m_month_view_grid || !m_month_header_grid)
        return;

    for (auto *child : m_month_view_grid->get_children())
    {
        m_month_view_grid->remove(*child);
    }
    for (auto *child : m_month_header_grid->get_children())
    {
        m_month_header_grid->remove(*child);
    }

    const char *days_of_week[] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};
    for (int i = 0; i < 7; ++i)
    {
        auto label = Gtk::make_managed<Gtk::Label>(days_of_week[i]);
        label->get_style_context()->add_class("dim-label");
        m_month_header_grid->attach(*label, i, 0);
    }

    tm displayed_tm = *localtime(&m_displayed_date);
    time_t today_t;
    time(&today_t);
    tm today_tm = *localtime(&today_t);
    tm selected_tm = *localtime(&m_selected_date);

    displayed_tm.tm_mday = 1;
    mktime(&displayed_tm);
    int month_start_wday = displayed_tm.tm_wday;

    tm current_day_tm = displayed_tm;
    current_day_tm.tm_mday -= month_start_wday;
    mktime(&current_day_tm);

    for (int row = 0; row < 6; ++row)
    {
        for (int col = 0; col < 7; ++col)
        {
            time_t current_cell_date_t = mktime(&current_day_tm);

            auto day_label = Gtk::make_managed<Gtk::Label>(to_string(current_day_tm.tm_mday));
            day_label->set_halign(Gtk::ALIGN_CENTER);
            day_label->set_valign(Gtk::ALIGN_CENTER);

            auto event_box = Gtk::make_managed<Gtk::EventBox>();
            event_box->add(*day_label);
            event_box->signal_button_press_event().connect(sigc::bind(sigc::mem_fun(*this, &SchedulerApp::on_day_cell_button_press), current_cell_date_t));

            auto cell_frame = Gtk::make_managed<Gtk::Frame>();
            cell_frame->add(*event_box);
            cell_frame->set_shadow_type(Gtk::SHADOW_NONE);

            if (current_day_tm.tm_mon != displayed_tm.tm_mon)
                cell_frame->get_style_context()->add_class("other-month-day");
            if (current_day_tm.tm_yday == today_tm.tm_yday && current_day_tm.tm_year == today_tm.tm_year)
                cell_frame->get_style_context()->add_class("today-cell");
            if (current_day_tm.tm_yday == selected_tm.tm_yday && current_day_tm.tm_year == selected_tm.tm_year)
                cell_frame->get_style_context()->add_class("selected-day-cell");

            m_month_view_grid->attach(*cell_frame, col, row);

            current_day_tm.tm_mday++;
            mktime(&current_day_tm);
        }
    }
    m_month_view_grid->show_all();
    m_month_header_grid->show_all();
}

// 填充周视图
void SchedulerApp::populate_week_view()
{
    if (!m_week_view_grid || !m_week_header_grid)
        return;

    for (auto *child : m_week_view_grid->get_children())
    {
        m_week_view_grid->remove(*child);
    }
    for (auto *child : m_week_header_grid->get_children())
    {
        m_week_header_grid->remove(*child);
    }

    const char *days_of_week[] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};

    tm displayed_tm = *localtime(&m_displayed_date);
    time_t today_t;
    time(&today_t);
    tm today_tm = *localtime(&today_t);
    tm selected_tm = *localtime(&m_selected_date);

    displayed_tm.tm_mday -= displayed_tm.tm_wday;
    mktime(&displayed_tm);

    vector<Task> all_tasks = m_task_manager.getAllTasks();

    for (int col = 0; col < 7; ++col)
    {
        char day_num_buf[4];
        strftime(day_num_buf, sizeof(day_num_buf), "%d", &displayed_tm);
        auto header_label = Gtk::make_managed<Gtk::Label>(string(days_of_week[col]) + "\n" + day_num_buf);
        header_label->set_justify(Gtk::JUSTIFY_CENTER);
        m_week_header_grid->attach(*header_label, col, 0);

        auto scrolled_window = Gtk::make_managed<Gtk::ScrolledWindow>();
        scrolled_window->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
        auto day_vbox = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 2);
        day_vbox->set_margin_top(5);
        day_vbox->set_margin_start(5);
        day_vbox->set_margin_end(5);
        scrolled_window->add(*day_vbox);

        auto event_box = Gtk::make_managed<Gtk::EventBox>();
        event_box->add(*scrolled_window);
        time_t current_cell_date_t = mktime(&displayed_tm);
        event_box->signal_button_press_event().connect(sigc::bind(sigc::mem_fun(*this, &SchedulerApp::on_day_cell_button_press), current_cell_date_t));

        auto cell_frame = Gtk::make_managed<Gtk::Frame>();
        cell_frame->add(*event_box);
        cell_frame->set_shadow_type(Gtk::SHADOW_NONE);

        if (displayed_tm.tm_yday == today_tm.tm_yday && displayed_tm.tm_year == today_tm.tm_year)
            cell_frame->get_style_context()->add_class("today-cell");
        if (displayed_tm.tm_yday == selected_tm.tm_yday && displayed_tm.tm_year == selected_tm.tm_year)
            cell_frame->get_style_context()->add_class("selected-day-cell");

        tm start_of_day_tm = displayed_tm;
        start_of_day_tm.tm_hour = 0;
        start_of_day_tm.tm_min = 0;
        start_of_day_tm.tm_sec = 0;
        time_t start_of_day = mktime(&start_of_day_tm);
        time_t start_of_next_day = start_of_day + 86400;

        for (const auto &task : all_tasks)
        {
            if (task.startTime >= start_of_day && task.startTime < start_of_next_day)
            {
                char time_buf[10];
                strftime(time_buf, sizeof(time_buf), "%H:%M", localtime(&task.startTime));
                auto task_label = Gtk::make_managed<Gtk::Label>(string(time_buf) + " " + task.name, Gtk::ALIGN_START, Gtk::ALIGN_CENTER);
                task_label->set_ellipsize(Pango::ELLIPSIZE_END);
                task_label->get_style_context()->add_class("task-label");

                auto task_event_box = Gtk::make_managed<Gtk::EventBox>();
                task_event_box->add(*task_label);
                task_event_box->signal_button_press_event().connect(sigc::bind(sigc::mem_fun(*this, &SchedulerApp::on_task_label_button_press), task.id, start_of_day));
                day_vbox->pack_start(*task_event_box, false, false);
            }
        }

        m_week_view_grid->attach(*cell_frame, col, 0);
        displayed_tm.tm_mday++;
        mktime(&displayed_tm);
    }
    m_week_view_grid->show_all();
    m_week_header_grid->show_all();
}

void SchedulerApp::update_selected_day_details()
{
    Gtk::ListBox *current_list_box = nullptr;
    if (m_current_view_mode == ViewMode::MONTH && m_selected_day_events_listbox)
    {
        current_list_box = m_selected_day_events_listbox;
    }
    else if (m_current_view_mode == ViewMode::WEEK && m_week_selected_day_events_listbox)
    {
        current_list_box = m_week_selected_day_events_listbox;
    }

    if (!current_list_box)
        return;

    for (auto *child : current_list_box->get_children())
    {
        current_list_box->remove(*child);
    }

    tm start_of_day_tm = *localtime(&m_selected_date);
    start_of_day_tm.tm_hour = 0;
    start_of_day_tm.tm_min = 0;
    start_of_day_tm.tm_sec = 0;
    time_t start_of_day = mktime(&start_of_day_tm);
    time_t start_of_next_day = start_of_day + 86400;

    vector<Task> tasks = m_task_manager.getAllTasks();

    for (const auto &task : tasks)
    {
        if (task.startTime >= start_of_day && task.startTime < start_of_next_day)
        {
            auto row = Gtk::make_managed<Gtk::ListBoxRow>();
            auto box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 10);
            box->set_margin_top(5);
            box->set_margin_bottom(5);
            box->set_margin_start(10);
            box->set_margin_end(10);

            time_t end_time = task.startTime + task.duration * 60;
            auto time_label = Gtk::make_managed<Gtk::Label>(format_timespan(task.startTime, end_time));
            auto name_label = Gtk::make_managed<Gtk::Label>(task.name);
            name_label->set_hexpand(true);
            name_label->set_halign(Gtk::ALIGN_START);

            box->pack_start(*time_label, false, false);
            box->pack_start(*name_label);
            row->add(*box);
            row->set_data("task_id", new long long(task.id));
            current_list_box->add(*row);
        }
    }
    current_list_box->show_all();
}

void SchedulerApp::update_date_label_and_indicator()
{
    if (!m_current_date_label || !m_week_of_year_label || !m_today_indicator_box)
        return;

    tm displayed_tm = *localtime(&m_displayed_date);
    char buffer[80];
    if (m_current_view_mode == ViewMode::MONTH)
    {
        strftime(buffer, sizeof(buffer), "%Y年%m月", &displayed_tm);
    }
    else
    { // 周视图
        tm end_of_week_tm = displayed_tm;
        end_of_week_tm.tm_mday += 6;
        mktime(&end_of_week_tm);
        if (displayed_tm.tm_mon != end_of_week_tm.tm_mon)
        {
            strftime(buffer, sizeof(buffer), "%Y年%m月%d日", &displayed_tm);
        }
        else
        {
            strftime(buffer, sizeof(buffer), "%Y年%m月", &displayed_tm);
        }
    }
    m_current_date_label->set_text(buffer);

    time_t today_t;
    time(&today_t);
    tm today_tm = *localtime(&today_t);
    tm selected_tm = *localtime(&m_selected_date);

    if (today_tm.tm_year == selected_tm.tm_year && today_tm.tm_yday == selected_tm.tm_yday)
    {
        m_today_button->get_style_context()->add_class("view-button-active");
    }
    else
    {
        m_today_button->get_style_context()->remove_class("view-button-active");
    }

    char week_buf[10];
    strftime(week_buf, sizeof(week_buf), "第%V周", &selected_tm);
    m_week_of_year_label->set_text(week_buf);
}

void SchedulerApp::update_view_switcher_ui()
{
    if (!m_month_view_button || !m_week_view_button || !m_agenda_view_button)
        return;

    m_month_view_button->get_style_context()->remove_class("view-button-active");
    m_week_view_button->get_style_context()->remove_class("view-button-active");
    m_agenda_view_button->get_style_context()->remove_class("view-button-active");

    switch (m_current_view_mode)
    {
    case ViewMode::MONTH:
        m_month_view_button->get_style_context()->add_class("view-button-active");
        break;
    case ViewMode::WEEK:
        m_week_view_button->get_style_context()->add_class("view-button-active");
        break;
    case ViewMode::AGENDA:
        m_agenda_view_button->get_style_context()->add_class("view-button-active");
        break;
    }
}

// --- 日历交互事件处理函数 ---

bool SchedulerApp::on_day_cell_button_press(GdkEventButton *event, time_t date)
{
    m_selected_date = date;
    update_all_views();

    if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_SECONDARY)
    {
        if (m_empty_space_context_menu)
        {
            m_context_menu_date = date;
            m_context_menu_task_id = -1;
            m_empty_space_context_menu->popup_at_pointer((GdkEvent *)event);
        }
    }
    return true;
}

bool SchedulerApp::on_task_label_button_press(GdkEventButton *event, long long task_id, time_t date)
{
    m_selected_date = date;
    update_all_views();

    if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_SECONDARY)
    {
        if (m_task_context_menu)
        {
            m_context_menu_task_id = task_id;
            m_context_menu_date = 0;
            m_task_context_menu->popup_at_pointer((GdkEvent *)event);
        }
    }
    return true;
}

bool SchedulerApp::on_tree_view_button_press(GdkEventButton *event)
{
    if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_SECONDARY)
    {
        Gtk::TreeModel::Path path;
        if (task_tree_view->get_path_at_pos(event->x, event->y, path))
        {
            auto iter = m_refTreeModel->get_iter(path);
            if (iter)
            {
                m_context_menu_task_id = (*iter)[m_Columns.m_col_id];
                m_context_menu_date = 0;
                m_task_context_menu->popup_at_pointer((GdkEvent *)event);
            }
        }
        else
        {
            m_context_menu_date = 0;
            m_context_menu_task_id = -1;
            m_empty_space_context_menu->popup_at_pointer((GdkEvent *)event);
        }
        return true;
    }
    return false;
}

// ** THE FIX IS HERE **
// This function definition now matches the declaration in SchedulerApp.h
bool SchedulerApp::on_list_box_button_press(GdkEventButton *event, Gtk::ListBox *listbox)
{
    if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_SECONDARY)
    {
        auto clicked_row = listbox->get_row_at_y(event->y);
        if (clicked_row)
        {
            listbox->select_row(*clicked_row);
            long long *p_id = static_cast<long long *>(clicked_row->get_data("task_id"));
            if (p_id && m_task_context_menu)
            {
                m_context_menu_task_id = *p_id;
                m_context_menu_date = 0;
                m_task_context_menu->popup_at_pointer((GdkEvent *)event);
            }
        }
        else
        {
            if (m_empty_space_context_menu)
            {
                m_context_menu_date = m_selected_date;
                m_context_menu_task_id = -1;
                m_empty_space_context_menu->popup_at_pointer((GdkEvent *)event);
            }
        }
        return true;
    }
    return false;
}

// --- 其余函数 ---

void SchedulerApp::on_add_task_ok_button_clicked()
{
    if (!task_name_entry || !task_duration_spin || !task_priority_combo || !task_category_combo || !task_custom_category_entry || !task_reminder_combo)
    {
        show_message("代码错误", "一个或多个对话框控件指针为空。");
        return;
    }
    string name = task_name_entry->get_text();
    if (name.empty())
    {
        show_message("输入错误", "任务名称不能为空。");
        return;
    }
    if (m_selected_start_time == 0)
    {
        show_message("输入错误", "请选择一个开始时间。");
        return;
    }
    time_t now = time(nullptr);
    if (m_selected_start_time <= now)
    {
        show_message("时间错误", "任务开始时间必须晚于当前时间。");
        return;
    }
    Task newTask;
    newTask.name = name;
    newTask.startTime = m_selected_start_time;
    newTask.duration = task_duration_spin->get_value_as_int();
    string priority_str = task_priority_combo->get_active_text();
    if (priority_str == "高")
        newTask.priority = Priority::HIGH;
    else if (priority_str == "低")
        newTask.priority = Priority::LOW;
    else
        newTask.priority = Priority::MEDIUM;
    string category_str = task_category_combo->get_active_text();
    if (category_str == "学习")
        newTask.category = Category::STUDY;
    else if (category_str == "娱乐")
        newTask.category = Category::ENTERTAINMENT;
    else if (category_str == "生活")
        newTask.category = Category::LIFE;
    else
    {
        newTask.category = Category::OTHER;
        newTask.customCategory = task_custom_category_entry->get_text();
        if (newTask.customCategory.empty())
        {
            show_message("输入错误", "自定义分类名称不能为空。");
            return;
        }
    }
    string reminder_text = task_reminder_combo->get_active_text();
    if (task_reminder_combo->get_active_id() == "remind_custom")
    {
        reminder_text = task_reminder_entry->get_text();
    }
    newTask.reminderOption = reminder_text;
    if (!reminder_text.empty() && reminder_text != "不提醒")
    {
        try
        {
            std::smatch match;
            if (std::regex_search(reminder_text, match, std::regex("(\\d+)\\s*分钟前")))
            {
                newTask.reminderTime = newTask.startTime - (stoi(match[1].str()) * 60);
            }
            else if (std::regex_search(reminder_text, match, std::regex("(\\d+)\\s*小时前")))
            {
                newTask.reminderTime = newTask.startTime - (stoi(match[1].str()) * 3600);
            }
            else if (std::regex_search(reminder_text, match, std::regex("(\\d+)\\s*天前")))
            {
                newTask.reminderTime = newTask.startTime - (stoi(match[1].str()) * 86400);
            }
            else
            {
                int minutes = stoi(reminder_text);
                if (minutes > 0)
                {
                    newTask.reminderTime = newTask.startTime - (minutes * 60);
                }
                else
                {
                    newTask.reminderTime = 0;
                }
            }
        }
        catch (...)
        {
            newTask.reminderTime = 0;
            newTask.reminderOption = "不提醒";
        }
        if (newTask.reminderTime != 0)
        {
            if (newTask.reminderTime <= now)
            {
                show_message("时间错误", "提醒时间必须晚于当前时间。");
                return;
            }
            if (newTask.reminderTime >= newTask.startTime)
            {
                show_message("时间错误", "提醒时间必须早于任务开始时间。");
                return;
            }
        }
    }
    else
    {
        newTask.reminderTime = 0;
        newTask.reminderOption = "不提醒";
    }
    if (m_task_manager.addTask(newTask))
    {
        update_all_views();
        show_message("成功", "新任务已添加。");
        if (add_task_dialog)
            add_task_dialog->hide();
    }
    else
    {
        show_message("失败", "添加任务失败，可能存在同名且同开始时间的任务。");
    }
}

void SchedulerApp::on_add_task_cancel_button_clicked()
{
    if (add_task_dialog)
        add_task_dialog->hide();
}
void SchedulerApp::on_task_start_time_button_clicked()
{
    if (!add_task_dialog)
        return;
    time_t initial_time = (m_selected_start_time != 0) ? m_selected_start_time : time(nullptr);
    tm time_info = *localtime(&initial_time);
    Gtk::Dialog calendar_dialog("选择日期", *add_task_dialog, true);
    Gtk::Calendar *calendar = Gtk::manage(new Gtk::Calendar());
    calendar->select_month(time_info.tm_mon, time_info.tm_year + 1900);
    calendar->select_day(time_info.tm_mday);
    calendar_dialog.get_content_area()->pack_start(*calendar);
    calendar_dialog.add_button("确定", Gtk::RESPONSE_OK);
    calendar_dialog.add_button("取消", Gtk::RESPONSE_CANCEL);
    calendar_dialog.show_all();
    if (calendar_dialog.run() == Gtk::RESPONSE_OK)
    {
        unsigned int year, month, day;
        calendar->get_date(year, month, day);
        time_info.tm_year = year - 1900;
        time_info.tm_mon = month;
        time_info.tm_mday = day;
        if (get_time_from_user(time_info, calendar_dialog))
        {
            m_selected_start_time = mktime(&time_info);
            if (task_start_time_button)
                task_start_time_button->set_label(time_t_to_datetime_string(m_selected_start_time));
            on_add_task_fields_changed();
        }
    }
}
bool SchedulerApp::get_time_from_user(tm &time_struct, Gtk::Window &parent)
{
    Gtk::Dialog time_dialog("选择时间", parent, true);
    Gtk::Grid *grid = Gtk::manage(new Gtk::Grid());
    grid->set_border_width(10);
    grid->set_row_spacing(5);
    grid->set_column_spacing(5);
    Gtk::Label *hour_label = Gtk::manage(new Gtk::Label("小时:"));
    Gtk::SpinButton *hour_spin = Gtk::manage(new Gtk::SpinButton());
    hour_spin->set_adjustment(Gtk::Adjustment::create(time_struct.tm_hour, 0, 23, 1));
    hour_spin->set_digits(0);
    Gtk::Label *min_label = Gtk::manage(new Gtk::Label("分钟:"));
    Gtk::SpinButton *min_spin = Gtk::manage(new Gtk::SpinButton());
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
    if (time_dialog.run() == Gtk::RESPONSE_OK)
    {
        time_struct.tm_hour = hour_spin->get_value_as_int();
        time_struct.tm_min = min_spin->get_value_as_int();
        time_struct.tm_sec = 0;
        return true;
    }
    return false;
}
void SchedulerApp::on_add_task_fields_changed() { update_end_time_label(); }
void SchedulerApp::on_task_category_combo_changed()
{
    if (task_category_combo && task_custom_category_entry)
    {
        if (task_category_combo->get_active_id() == "category_other_item")
        {
            task_custom_category_entry->show();
        }
        else
        {
            task_custom_category_entry->hide();
        }
    }
}
bool SchedulerApp::on_reminder_entry_focus_in(GdkEventFocus * /*event*/)
{
    if (task_reminder_entry)
    {
        Glib::signal_timeout().connect_once([this]()
                                            { task_reminder_entry->select_region(0, -1); }, 0);
    }
    return false;
}
bool SchedulerApp::on_reminder_entry_focus_out(GdkEventFocus * /*event*/)
{
    if (!task_reminder_entry)
    {
        return false;
    }
    Glib::ustring text = task_reminder_entry->get_text();
    const Glib::ustring whitespace = " \t\n\v\f\r";
    const auto first = text.find_first_not_of(whitespace);
    if (first == Glib::ustring::npos)
    {
        text.clear();
    }
    else
    {
        const auto last = text.find_last_not_of(whitespace);
        text = text.substr(first, (last - first + 1));
    }
    if (text.empty())
    {
        task_reminder_entry->set_text("不提醒");
        return false;
    }
    if (text == "不提醒" || text.find("分钟前") != Glib::ustring::npos || text.find("小时前") != Glib::ustring::npos || text.find("天前") != Glib::ustring::npos)
    {
        return false;
    }
    try
    {
        size_t pos = 0;
        long value = std::stol(text.raw(), &pos);
        if (pos != text.bytes())
        {
            throw std::invalid_argument("Extra characters found");
        }
        if (value <= 0)
        {
            task_reminder_entry->set_text("不提醒");
        }
        else
        {
            task_reminder_entry->set_text(std::to_string(value) + "分钟前");
        }
    }
    catch (const std::exception &e)
    {
        task_reminder_entry->set_text("不提醒");
    }
    return false;
}
void SchedulerApp::update_end_time_label()
{
    if (!task_end_time_label || !task_duration_spin)
        return;
    if (m_selected_start_time == 0)
    {
        task_end_time_label->set_text("[自动计算]");
        return;
    }
    int duration_minutes = task_duration_spin->get_value_as_int();
    time_t end_time = m_selected_start_time + duration_minutes * 60;
    task_end_time_label->set_text(time_t_to_datetime_string(end_time));
}
void SchedulerApp::reset_add_task_dialog()
{
    if (task_name_entry)
        task_name_entry->set_text("");
    m_selected_start_time = 0;
    m_context_menu_date = 0;
    if (task_start_time_button)
        task_start_time_button->set_label("选择开始时间...");
    if (task_duration_spin)
        task_duration_spin->set_value(30);
    if (task_end_time_label)
        task_end_time_label->set_text("[自动计算]");
    if (task_priority_combo)
        task_priority_combo->set_active(1);
    if (task_category_combo)
        task_category_combo->set_active(0);
    if (task_custom_category_entry)
    {
        task_custom_category_entry->set_text("");
        task_custom_category_entry->hide();
    }
    if (task_reminder_combo)
        task_reminder_combo->set_active_id("remind_none");
}
void SchedulerApp::update_task_list()
{
    if (!m_refTreeModel)
        return;
    m_refTreeModel->clear();
    vector<Task> tasks = m_task_manager.getAllTasks();
    time_t now = time(nullptr);
    for (const auto &task : tasks)
    {
        Gtk::TreeModel::Row row = *(m_refTreeModel->append());
        row[m_Columns.m_col_id] = task.id;
        row[m_Columns.m_col_name] = task.name;
        row[m_Columns.m_col_timespan] = format_timespan(task.startTime, task.startTime + task.duration * 60);
        row[m_Columns.m_col_priority] = priority_to_string(task.priority);
        row[m_Columns.m_col_category] = category_to_string(task);
        row[m_Columns.m_col_reminder_option] = task.reminderOption;
        row[m_Columns.m_col_reminder_status] = get_reminder_status(task);
        row[m_Columns.m_col_task_status] = get_task_status(task, now);
    }
}
void SchedulerApp::show_message(const string &title, const string &msg)
{
    Gtk::Window *parent_window = nullptr;
    if (add_task_dialog && add_task_dialog->is_visible())
    {
        parent_window = add_task_dialog;
    }
    else if (change_password_window && change_password_window->is_visible())
    {
        parent_window = change_password_window;
    }
    else if (register_window && register_window->is_visible())
    {
        parent_window = register_window;
    }
    else if (main_window && main_window->is_visible())
    {
        parent_window = main_window;
    }
    else
    {
        parent_window = login_window;
    }
    if (parent_window)
    {
        Gtk::MessageDialog dialog(*parent_window, msg, false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
        dialog.set_title(title);
        dialog.set_keep_above(true);
        dialog.run();
    }
    else
    {
        cerr << "无法为消息对话框找到父窗口: " << msg << endl;
    }
}
void SchedulerApp::on_login_success()
{
    m_task_manager.setReminderCallback([this](const std::string &title, const std::string &msg)
                                       { Glib::signal_idle().connect_once([this, title, msg]()
                                                                          { this->show_message(title, msg); this->update_all_views(); }); });
    m_task_manager.startReminderThread();
    m_timer_connection = Glib::signal_timeout().connect([this]()
                                                        {
        this->update_all_views();
        return true; }, 60000);
}

void SchedulerApp::on_reminder(const std::string &title, const std::string &msg) { show_message(title, msg); }