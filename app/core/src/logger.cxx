module logger;
import std;
import config;
import clock;
import <Windows.h>;

namespace ac::logger {

    void logg(const std::string& msg) {
        main_log_stream << msg << std::endl;
        if (ac::config.send_logg_to_cout) {
            std::cout << msg << std::endl;
        }
    }

    void loggnl(const std::string& msg) {
        main_log_stream << msg;
        if (ac::config.send_logg_to_cout) {
            std::cout << msg;
        }
    }

    void logg(const std::wstring& msg) {
        main_log_stream << ac::encoding::to_utf8(msg) << std::endl;
        if (ac::config.send_logg_to_cout) {
            std::cout << ac::encoding::to_utf8(msg) << std::endl;
        }
    }

    void loggnl(const std::wstring& msg) {
        main_log_stream << ac::encoding::to_utf8(msg);
        if (ac::config.send_logg_to_cout) {
            std::cout << ac::encoding::to_utf8(msg);
        }
    }

    void logg(char msg) {
        main_log_stream << ac::encoding::to_utf8(msg) << std::endl;
        if (ac::config.send_logg_to_cout) {
            std::cout << ac::encoding::to_utf8(msg) << std::endl;
        }
    }

    void loggnl(char msg) {
        main_log_stream << ac::encoding::to_utf8(msg);
        if (ac::config.send_logg_to_cout) {
            std::cout << ac::encoding::to_utf8(msg);
        }
    }

    void logg(wchar_t msg) {
        main_log_stream << ac::encoding::to_utf8(msg) << std::endl;
        if (ac::config.send_logg_to_cout) {
            std::cout << ac::encoding::to_utf8(msg) << std::endl;
        }
    }

    void loggnl(wchar_t msg) {
        main_log_stream << ac::encoding::to_utf8(msg);
        if (ac::config.send_logg_to_cout) {
            std::cout << ac::encoding::to_utf8(msg);
        }
    }

    void update_main_log_file() {
        close_main_log_file();
        logger_datestamp = ac::clock::get_datestamp();
        main_log_name = "log_" + logger_datestamp + ".log";
        std::string logger_path = log_directory + main_log_name;
        main_log_stream.open(logger_path, std::ios::app);
    }

    /**
     * \brief Closes the main log file.
     *
     * Flushes and closes the main log file if it is open.
     */
    void close_main_log_file() {
        if (main_log_stream.is_open()) {
            main_log_stream.flush();
            main_log_stream.close();
        }
    }

    /**
     * \brief Ends the logging session.
     *
     * Logs the session end message and closes the main log file.
     */
    void log_end() {
        ac::logger::logg("Session ended {}", ac::clock::get_datetime_stamp_for_logger());
        ac::logger::logg("***");
        close_main_log_file();
    }
}