/**
\file taskbar.ixx
\brief Enables quick taskbar access for the first 10 programs in the taskbar using the system shortcut of 'winkey + #'.
*/
export module taskbar;

export import command_registry;
import std;
import <Windows.h>;

export namespace taskbar_runtime_commands {
    void register_with(command_registry::Registry& registry);
}

export {
    void initialize_taskbar();
    std::optional<int> taskbar_position(std::string_view program);
    void activate_auto_core();
    void activate_folder();
    void activate_word();
    void activate_vs_code();
    void activate_iTunes();
    void activate_discord();
    void activate_chrome();
    void activate_spotify();
    void activate_visual();
    void activate_firefox();
    void refresh_firefox();
    void start_reddit_new_tab();
}

export class Taskbar {
public:
    void load_config();
    [[nodiscard]] std::optional<int> position(
        std::string_view program
    ) const;
    int switch_position;
    int switch_keycode;
    bool switch_set;
    void switch_windows(int keycode);
    void activate_position_single(const std::string_view name);
    void activate_position_multiple(
        std::string_view name,
        int& window_count,
        WNDENUMPROC enum_windows
    );
    void activate_auto_core();
    void activate_folder();
    void activate_word();
    void activate_vs_code();
    void activate_iTunes();
    void activate_chrome();
    void activate_visual();
    void activate_discord();
    void activate_firefox();
    void activate_spotify();
    int folder_windows = 0;
    int word_windows = 0;
    int vs_code_windows = 0;
    int chrome_windows = 0;
    int visual_windows = 0;
    int firefox_windows = 0;
    bool winkey_locked;

private:
    std::unordered_map<std::string, int> positions_;
};

export Taskbar taskbar;
