#pragma once

#include <gtkmm.h>
#include "UserManager.h"
#include "TaskManager.h"

class SchedulerApp : public Gtk::Application {
protected:
    SchedulerApp();

public:
    static Glib::RefPtr<SchedulerApp> create();

protected:
    // 重写Gtk::Application的虚函数
    void on_activate() override;

private:
    class ModelColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:
        ModelColumns()
        { 
            add(m_col_id); 
            add(m_col_name);
            add(m_col_start_time);
            add(m_col_priority);
            add(m_col_category);
            add(m_col_reminder_time);
        }

        Gtk::TreeModelColumn<long long> m_col_id;
        Gtk::TreeModelColumn<Glib::ustring> m_col_name;
        Gtk::TreeModelColumn<Glib::ustring> m_col_start_time;
        Gtk::TreeModelColumn<Glib::ustring> m_col_priority;
        Gtk::TreeModelColumn<Glib::ustring> m_col_category;
        Gtk::TreeModelColumn<Glib::ustring> m_col_reminder_time;
    };

    ModelColumns m_Columns;

    // UI元素指针
    Glib::RefPtr<Gtk::Builder> m_builder;
    Gtk::Window *login_window = nullptr, *register_window = nullptr, *main_window = nullptr, *help_window = nullptr;
    Gtk::Dialog *add_task_dialog = nullptr;
    Gtk::Entry *login_username_entry = nullptr, *login_password_entry = nullptr;
    Gtk::Entry *register_username_entry = nullptr, *register_password_entry = nullptr, *register_confirm_password_entry = nullptr;
    // ... 其他控件指针 ...
    Gtk::TreeView* task_tree_view = nullptr;
    Glib::RefPtr<Gtk::ListStore> m_refTreeModel;
    Gtk::Statusbar* main_statusbar = nullptr;


    // 业务逻辑处理器
    UserManager m_user_manager;
    TaskManager m_task_manager;
    std::string m_current_user;

    // UI初始化和管理
    void get_widgets();
    void connect_signals();
    void show_message(const std::string& title, const std::string& msg);

    // 信号处理函数
    void on_login_button_clicked();
    void on_show_register_button_clicked();
    void on_register_button_clicked();
    void on_logout_button_clicked();
    void on_add_task_button_clicked();
    void on_delete_task_button_clicked();
    void on_show_help_button_clicked();
    void on_help_close_button_clicked();
    void on_add_task_dialog_response(int response_id);
    
    // UI更新
    void update_task_list();
};
