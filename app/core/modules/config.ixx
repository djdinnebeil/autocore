/**
 * \file config.ixx
 * \brief Provides a module to handle program configuration.
 *
 * This module defines a Config class to manage global configuration settings
 * for the application. It includes methods to load configuration settings from
 * various configuration files.
 */
module;

#ifdef BUILDING_DLL
    #define DLL_API __declspec(dllexport)
#else
    #define DLL_API __declspec(dllimport)
#endif

export module config;
import std;
import <Windows.h>;

export namespace ac {
    std::wstring get_executable_directory();

    /**
     * \brief Config class to manage global configuration settings.
     */
    class Config {
    public:
        static DLL_API Config& get_instance();
        std::string configuration_log;
        std::wstring current_directory;
        std::wstring program_title;
        std::string logger_directory;
        bool runtime_enabled;
        bool runtime_debugger;
        bool runtime_logger;
        bool start_server;
        int port_number;
        int end_of_day;
        bool send_logg_to_cout;
        std::unordered_map<std::string, int> taskbar_position;
        std::unordered_map<int, std::string> taskbar_program;
    private:
        Config();
        void load_runtime_config();
        void load_server_config();
        void load_clock_config();
        void load_logger_config();
        void load_taskbar_config();
    };

    inline Config& config = Config::get_instance();
}
