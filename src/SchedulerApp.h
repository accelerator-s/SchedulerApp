#pragma once

#include <gtkmm.h>
#include "UserManager.h"
#include "TaskManager.h"
#include <ctime>
#include <set>
#include <vector>
#include <libayatana-appindicator/app-indicator.h>

class SchedulerApp : public Gtk::Application
{
public:
    // 构造函数
    SchedulerApp();

    // 提醒回调函数，在后台线程触发提醒时被调用
    void on_reminder(const string &title, const string &msg);
    // 登录成功后调用的函数，用于启动后台服务
    void on_login_success();

    // 创建应用程序实例的静态工厂方法
    static Glib::RefPtr<SchedulerApp> create();

    // Gtk::Application 的虚函数，在应用激活时调用
    void on_activate() override;

    // 枚举，定义了三种视图模式
    enum class ViewMode
    {
        MONTH,
        WEEK,
        AGENDA
    };

    // 定义 TreeView 的数据列模型
    class ModelColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:
        ModelColumns()
        {
            add(m_col_id);
            add(m_col_name);
            add(m_col_timespan);
            add(m_col_priority);
            add(m_col_category);
            add(m_col_reminder_option);
            add(m_col_reminder_status);
            add(m_col_task_status);
        }

        Gtk::TreeModelColumn<long long> m_col_id;
        Gtk::TreeModelColumn<Glib::ustring> m_col_name;
        Gtk::TreeModelColumn<Glib::ustring> m_col_timespan;
        Gtk::TreeModelColumn<Glib::ustring> m_col_priority;
        Gtk::TreeModelColumn<Glib::ustring> m_col_category;
        Gtk::TreeModelColumn<Glib::ustring> m_col_reminder_option;
        Gtk::TreeModelColumn<Glib::ustring> m_col_reminder_status;
        Gtk::TreeModelColumn<Glib::ustring> m_col_task_status;
    };

    ModelColumns m_Columns; // TreeView 列模型的实例

    // UI元素指针
    Glib::RefPtr<Gtk::Builder> m_builder;
    Gtk::Window *login_window = nullptr, *register_window = nullptr, *main_window = nullptr, *help_window = nullptr, *change_password_window = nullptr;
    Gtk::Dialog *add_task_dialog = nullptr;
    Gtk::Entry *login_username_entry = nullptr, *login_password_entry = nullptr;
    Gtk::Entry *register_username_entry = nullptr, *register_password_entry = nullptr, *register_confirm_password_entry = nullptr;

    Gtk::Entry *cp_username_entry = nullptr, *cp_old_password_entry = nullptr, *cp_new_password_entry = nullptr, *cp_confirm_new_password_entry = nullptr;
    Gtk::Label *cp_old_password_label = nullptr;

    Gtk::TreeView *task_tree_view = nullptr;
    Glib::RefPtr<Gtk::ListStore> m_refTreeModel;

    // "设置" 菜单相关控件
    Gtk::MenuButton *settings_button = nullptr;
    Gtk::MenuItem *menu_item_logout = nullptr;
    Gtk::MenuItem *menu_item_change_password = nullptr;
    Gtk::MenuItem *menu_item_delete_account = nullptr;
    Gtk::MenuItem *menu_item_help = nullptr;

    // 主界面控件
    Gtk::Paned *m_month_view_pane = nullptr;
    Gtk::Stack *m_main_stack = nullptr;
    Gtk::Grid *m_month_header_grid = nullptr, *m_month_view_grid = nullptr;
    Gtk::Grid *m_week_header_grid = nullptr;
    Gtk::Box *m_date_navigation_box = nullptr;
    Gtk::Label *m_current_date_label = nullptr;
    Gtk::Button *m_prev_button = nullptr, *m_next_button = nullptr;
    Gtk::Button *m_month_view_button = nullptr, *m_week_view_button = nullptr, *m_agenda_view_button = nullptr;
    Gtk::Button *m_agenda_add_task_button = nullptr, *m_agenda_delete_task_button = nullptr;
    Gtk::Box *m_today_indicator_box = nullptr;
    Gtk::Button *m_today_button = nullptr;
    Gtk::Label *m_week_of_year_label = nullptr;
    Gtk::ListBox *m_selected_day_events_listbox = nullptr;
    Gtk::ListBox *m_week_selected_day_events_listbox = nullptr;
    Gtk::RadioButton *m_week_day_buttons[7] = {nullptr};
    Gtk::Label *m_week_day_labels[7] = {nullptr};
    sigc::connection m_week_day_signal_connections[7];

    // 右键上下文菜单
    Gtk::Menu *m_task_context_menu = nullptr, *m_empty_space_context_menu = nullptr;
    Gtk::MenuItem *m_ctx_menu_delete_task = nullptr, *m_ctx_menu_add_task = nullptr;
    long long m_context_menu_task_id = -1; // 用于存储右键点击的任务ID
    time_t m_context_menu_date = 0;        // 用于存储右键点击的日期，以便在添加任务时预填

    // "添加任务" 对话框的控件
    Gtk::Entry *task_name_entry = nullptr;
    Gtk::Button *task_start_time_button = nullptr;
    Gtk::SpinButton *task_duration_spin = nullptr;
    Gtk::Label *task_end_time_label = nullptr;
    Gtk::ComboBoxText *task_priority_combo = nullptr;
    Gtk::ComboBoxText *task_category_combo = nullptr;
    Gtk::Entry *task_custom_category_entry = nullptr;
    Gtk::ComboBoxText *task_reminder_combo = nullptr;
    Gtk::Entry *task_reminder_entry = nullptr;
    time_t m_selected_start_time = 0; // "添加任务"对话框中选择的开始时间

    // 业务逻辑处理器和状态
    UserManager m_user_manager;
    TaskManager m_task_manager;
    string m_current_user;
    bool m_is_changing_password_from_main = false;
    ViewMode m_current_view_mode = ViewMode::WEEK;
    time_t m_displayed_date;
    time_t m_selected_date;

    // 用于管理定时器
    sigc::connection m_timer_connection;

    // 实现系统托盘图标
    void setup_tray_icon();

private:
    set<time_t> m_days_with_tasks;       // 缓存有任务的日期
    void update_days_with_tasks_cache(); // 更新缓存的方法

    // UI初始化和管理
    void get_widgets();
    void connect_signals();
    void show_message(const string &title, const string &msg);

    // UI更新和辅助函数
    void update_all_views();
    void update_view_specific_layout();
    void update_task_list();
    void populate_month_view();
    void populate_week_view();
    void update_selected_day_details();
    void update_date_label_and_indicator();
    void update_view_switcher_ui();
    void update_end_time_label();
    void reset_add_task_dialog();
    string priority_to_string(Priority p);
    string category_to_string(const Task &task);
    bool get_time_from_user(tm &time_struct, Gtk::Window &parent);
    string get_task_status(const Task &task, time_t current_time);
    string get_reminder_status(const Task &task);
    string format_timespan_simple(time_t start_time, time_t end_time);
    string format_timespan_complex(time_t start_time, time_t end_time);

    // 登录界面信号处理函数
    void on_login_button_clicked();
    void on_show_register_button_clicked();
    void on_register_button_clicked();
    void on_show_change_password_button_clicked();
    void on_cp_confirm_button_clicked();

    // 主界面信号处理函数
    void on_prev_button_clicked();
    void on_next_button_clicked();
    void on_today_button_clicked();
    void on_view_button_clicked(ViewMode new_mode);
    void on_agenda_add_task_button_clicked();
    void on_agenda_delete_task_button_clicked();
    // 帮助界面信号处理函数
    void on_help_close_button_clicked();

    // 上下文菜单相关事件处理
    bool on_day_cell_button_press(GdkEventButton *event, time_t date);
    bool on_task_label_button_press(GdkEventButton *event, long long task_id, time_t date);
    bool on_tree_view_button_press(GdkEventButton *event);
    bool on_list_box_button_press(GdkEventButton *event, Gtk::ListBox *listbox);
    void on_ctx_menu_add_task_activated();
    void on_ctx_menu_delete_task_activated();

    // "设置" 菜单的信号处理函数
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
    bool on_reminder_entry_focus_in(GdkEventFocus *event);
    bool on_reminder_entry_focus_out(GdkEventFocus *event);
    bool day_has_tasks(time_t day_time, const vector<Task> &all_tasks);

    // 托盘图标相关处理
    AppIndicator *indicator_ = nullptr;
    Gtk::Menu menu_;
    Gtk::MenuItem item_show_{"显示面板"};
    Gtk::MenuItem item_quit_{"退出"};
    GtkMenu *tray_menu_ = nullptr;
    void on_show_window();
    void on_quit_app();
    bool on_main_window_delete_event(GdkEventAny *);
};