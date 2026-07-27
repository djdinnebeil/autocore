module logger_c;
import std;
import config;
import clock;
import logger;
import print;
import <Windows.h>;

namespace fs = std::filesystem;

namespace ac {

    Logger::Logger(const std::string& log_name) {
        name = log_name;
        directory = log_directory + name + "\\";
        open_log_file();
    }

    Logger::~Logger() {
        close_log_file();
    }

    void Logger::logg(const std::string& msg) {
        log_stream << msg << std::endl;
        if (ac::config.send_logg_to_cout) {
            std::cout << msg << std::endl;
        }
    }

    void Logger::loggnl(const std::string& msg) {
        log_stream << msg;
        if (ac::config.send_logg_to_cout) {
            std::cout << msg;
        }
    }

    void Logger::logg_and_logg(const std::string& msg) {
        log_stream << msg << std::endl;
        ac::logger::logg(msg);
    }

    void Logger::loggnl_and_loggnl(const std::string& msg) {
        log_stream << msg;
        ac::logger::loggnl(msg);
    }

    void Logger::logg_and_print(const std::string& msg) {
        log_stream << msg << std::endl;
        ac::print(msg);
    }

    void Logger::loggnl_and_printnl(const std::string& msg) {
        log_stream << msg;
        ac::printnl(msg);
    }

    void Logger::logg(const std::wstring& msg) {
        log_stream << ac::encoding::to_utf8(msg) << std::endl;
        if (ac::config.send_logg_to_cout) {
            std::cout << ac::encoding::to_utf8(msg) << std::endl;
        }
    }

    void Logger::loggnl(const std::wstring& msg) {
        log_stream << ac::encoding::to_utf8(msg);
        if (ac::config.send_logg_to_cout) {
            std::cout << ac::encoding::to_utf8(msg);
        }
    }

    void Logger::logg_and_logg(const std::wstring& msg) {
        log_stream << ac::encoding::to_utf8(msg) << std::endl;
        ac::logger::logg(ac::encoding::to_utf8(msg));
    }

    void Logger::loggnl_and_loggnl(const std::wstring& msg) {
        log_stream << ac::encoding::to_utf8(msg);
        ac::logger::loggnl(ac::encoding::to_utf8(msg));
    }

    void Logger::logg_and_print(const std::wstring& msg) {
        log_stream << ac::encoding::to_utf8(msg) << std::endl;
        ac::print(ac::encoding::to_utf8(msg));
    }

    void Logger::loggnl_and_printnl(const std::wstring& msg) {
        log_stream << ac::encoding::to_utf8(msg);
        ac::printnl(ac::encoding::to_utf8(msg));
    }

    void Logger::logg(char msg) {
        log_stream << ac::encoding::to_utf8(msg) << std::endl;
        if (ac::config.send_logg_to_cout) {
            std::cout << ac::encoding::to_utf8(msg) << std::endl;
        }
    }

    void Logger::loggnl(char msg) {
        log_stream << ac::encoding::to_utf8(msg);
        if (ac::config.send_logg_to_cout) {
            std::cout << ac::encoding::to_utf8(msg);
        }
    }

    void Logger::logg_and_logg(char msg) {
        log_stream << ac::encoding::to_utf8(msg) << std::endl;
        ac::logger::logg(ac::encoding::to_utf8(msg));
    }

    void Logger::loggnl_and_loggnl(char msg) {
        log_stream << ac::encoding::to_utf8(msg);
        ac::logger::loggnl(ac::encoding::to_utf8(msg));
    }

    void Logger::logg_and_print(char msg) {
        log_stream << ac::encoding::to_utf8(msg) << std::endl;
        ac::print(ac::encoding::to_utf8(msg));
    }

    void Logger::loggnl_and_printnl(char msg) {
        log_stream << ac::encoding::to_utf8(msg);
        ac::printnl(ac::encoding::to_utf8(msg));
    }

    void Logger::logg(wchar_t msg) {
        log_stream << ac::encoding::to_utf8(msg) << std::endl;
        if (ac::config.send_logg_to_cout) {
            std::cout << ac::encoding::to_utf8(msg) << std::endl;
        }
    }

    void Logger::loggnl(wchar_t msg) {
        log_stream << ac::encoding::to_utf8(msg);
        if (ac::config.send_logg_to_cout) {
            std::cout << ac::encoding::to_utf8(msg);
        }
    }

    void Logger::logg_and_logg(wchar_t msg) {
        log_stream << ac::encoding::to_utf8(msg) << std::endl;
        ac::logger::logg(ac::encoding::to_utf8(msg));
    }

    void Logger::loggnl_and_loggnl(wchar_t msg) {
        log_stream << ac::encoding::to_utf8(msg);
        ac::logger::loggnl(ac::encoding::to_utf8(msg));
    }

    void Logger::logg_and_print(wchar_t msg) {
        log_stream << ac::encoding::to_utf8(msg) << std::endl;
        ac::print(ac::encoding::to_utf8(msg));
    }

    void Logger::loggnl_and_printnl(wchar_t msg) {
        log_stream << ac::encoding::to_utf8(msg);
        ac::printnl(ac::encoding::to_utf8(msg));
    }

    /**
     * \brief Updates the log file.
     *
     * Closes the current log file (if open), updates the log file name based on the current date,
     * and opens a new log file.
     */
    void Logger::update_log_file() {
        std::string session_status = "";
        if (log_stream.is_open()) {
            session_status = "---\n";
            log_stream << session_status;
            log_stream.flush();
            log_stream.close();
        }
        session_status += session_started;
        std::string datestamp = ac::clock::get_datestamp();
        std::string filename = name + "_" + datestamp + ".log";
        std::string logger_path = directory + filename;
        log_stream.open(logger_path, std::ios::app);
        log_stream << session_status << std::endl;
    }

    /**
     * \brief Opens the log file.
     *
     * Creates the necessary directories and opens the log file for writing.
     */
    void Logger::open_log_file() {
        fs::create_directories(directory);
        session_started = "Session started at " + ac::clock::get_datetime_stamp_for_logger();
        update_log_file();
    }

    /**
     * \brief Closes the log file.
     *
     * Flushes and closes the log file if it is open, logging the session end.
     */
    void Logger::close_log_file() {
        if (log_stream.is_open()) {
            std::string session_ended = "Session ended at " + ac::clock::get_datetime_stamp_for_logger();
            log_stream << session_ended << "\n***\n";
            log_stream.flush();
            log_stream.close();
        }
    }
}