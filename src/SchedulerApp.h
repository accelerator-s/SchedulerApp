#pragma once

#include <gtkmm.h>
#include "UserManager.h"
#include "TaskManager.h"

class SchedulerApp : public Gtk::Application {
public:
    SchedulerApp();

    void on_reminder(const std::string& title, const std::string& msg);
    void on_login_success();

    static Glib::RefPtr<SchedulerApp> create();

    void on_activate() override;

    // 更新列定义以匹配新的显示需求
    class ModelColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:
        ModelColumns()
        { 
            add(m_col_id); 
            add(m_col_name);
            add(m_col_timespan);           // 新增：任务时段列
            add(m_col_priority);
            add(m_col_category);
            add(m_col_reminder_option);    // 原 m_col_reminder_time，改名以反映内容
            add(m_col_reminder_status);
            add(m_col_task_status);
        }

        Gtk::TreeModelColumn<long long> m_col_id;
        Gtk::TreeModelColumn<Glib::ustring> m_col_name;
        Gtk::TreeModelColumn<Glib::ustring> m_col_timespan;          // 新增
        Gtk::TreeModelColumn<Glib::ustring> m_col_priority;
        Gtk::TreeModelColumn<Glib::ustring> m_col_category;
        Gtk::TreeModelColumn<Glib::ustring> m_col_reminder_option;   // 改名
        Gtk::TreeModelColumn<Glib::ustring> m_col_reminder_status;
        Gtk::TreeModelColumn<Glib::ustring> m_col_task_status;
    };

    ModelColumns m_Columns;

    // UI元素指针
    Glib::RefPtr<Gtk::Builder> m_builder;
    Gtk::Window *login_window = nullptr, *register_window = nullptr, *main_window = nullptr, *help_window = nullptr, *change_password_window = nullptr;
    Gtk::Dialog *add_task_dialog = nullptr;
    Gtk::Entry *login_username_entry = nullptr, *login_password_entry = nullptr;
    Gtk::Entry *register_username_entry = nullptr, *register_password_entry = nullptr, *register_confirm_password_entry = nullptr;
    
    Gtk::Entry *cp_username_entry = nullptr, *cp_old_password_entry = nullptr, *cp_new_password_entry = nullptr, *cp_confirm_new_password_entry = nullptr;
    Gtk::Label *cp_old_password_label = nullptr;

    Gtk::TreeView* task_tree_view = nullptr;
    Glib::RefPtr<Gtk::ListStore> m_refTreeModel;
    Gtk::Statusbar* main_statusbar = nullptr;

    // "更多" 菜单相关控件
    Gtk::MenuButton* more_button = nullptr;
    Gtk::MenuItem* menu_item_logout = nullptr;
    Gtk::MenuItem* menu_item_change_password = nullptr;
    Gtk::MenuItem* menu_item_delete_account = nullptr;
    Gtk::MenuItem* menu_item_help = nullptr;

    // "添加任务" 对话框的控件
    Gtk::Entry* task_name_entry = nullptr;
    Gtk::Button* task_start_time_button = nullptr;
    Gtk::SpinButton* task_duration_spin = nullptr;
    Gtk::Label* task_end_time_label = nullptr;
    Gtk::ComboBoxText* task_priority_combo = nullptr;
    Gtk::ComboBoxText* task_category_combo = nullptr;
    Gtk::Entry* task_custom_category_entry = nullptr;
    Gtk::ComboBoxText* task_reminder_combo = nullptr;
    Gtk::Entry* task_reminder_entry = nullptr;
    time_t m_selected_start_time = 0;

    // 业务逻辑处理器
    UserManager m_user_manager;
    TaskManager m_task_manager;
    std::string m_current_user;
    bool m_is_changing_password_from_main = false;

    // 用于管理定时器
    sigc::connection m_timer_connection; 

    // UI初始化和管理
    void get_widgets();
    void connect_signals();
    void show_message(const std::string& title, const std::string& msg);
    
    // UI更新和辅助函数
    void update_task_list();
    void update_end_time_label();
    void reset_add_task_dialog();
    std::string priority_to_string(Priority p);
    std::string category_to_string(const Task& task);
    bool get_time_from_user(tm& time_struct, Gtk::Window& parent);
    std::string get_task_status(const Task& task, time_t current_time);
    std::string get_reminder_status(const Task& task);
    // 新增：智能时间格式化函数
    std::string format_timespan(time_t start_time, time_t end_time);

    // 信号处理函数
    void on_login_button_clicked();
    void on_show_register_button_clicked();
    void on_register_button_clicked();
    void on_add_task_button_clicked();
    void on_delete_task_button_clicked();
    void on_show_change_password_button_clicked();
    void on_cp_confirm_button_clicked();

    // "更多" 菜单的信号处理函数
    void on_menu_item_logout_activated();
    void on_menu_item_change_password_activated();
    void on_menu_item_delete_account_activated();
    void on_menu_item_help_activated();
    
    // "添加任务" 对话框的信号处理
    void on_add_task_ok_button_clicked();
    void on_add_task_cancel_button_clicked();
    void on_task_start_time_button_clicked();
    void on_add_task_fields_changed();
    void on_task_category_combo_changed();

    // 提醒时间输入框的焦点事件处理
    bool on_reminder_entry_focus_in(GdkEventFocus* event);
    bool on_reminder_entry_focus_out(GdkEventFocus* event);
};