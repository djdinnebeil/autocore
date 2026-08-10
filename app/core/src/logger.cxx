module logger;

import std;

import clock;
import config;
import encoding;
import print;
import session;
import error;
import logger_x;
import pipes;

namespace fs = std::filesystem;

namespace {

    enum class LogRoute {
        component_only,
        component_and_main,
        component_main_and_console
    };

    void write_to_stream(
        std::ostream& stream,
        const std::string_view message,
        const bool newline
    ) {
        stream << message;

        if (newline) {
            stream << '\n';
        }
    }

    void write_message(
        std::ofstream& log_stream,
        std::string_view component_name,
        std::string_view message,
        bool newline,
        LogRoute destination
    ) {
        if (log_stream.is_open()) {
            write_to_stream(
                log_stream,
                message,
                newline
            );

            if (newline) {
                log_stream.flush();
            }
        }

        switch (destination) {
        case LogRoute::component_only:
            break;

        case LogRoute::component_and_main: {
            const ac::pipes::LogEvent event {
                .component = std::string {component_name},
                .message = std::string {message},
                .newline = newline
            };

            ac::logger::send_to_logger(event);
            break;
        }

        case LogRoute::component_main_and_console: {
            const ac::pipes::LogEvent event {
                .component = std::string {component_name},
                .message = std::string {message},
                .newline = newline
            };

            ac::logger::send_to_logger(event);
            write_to_stream(std::cout, message, newline);
            std::cout.flush();
            break;
        }
        }
    }

}

namespace ac {

    class Logger::Impl {
    public:
        explicit Impl(const std::string& log_name)
            : session_start(ac::session::make_start()),
            name(log_name),
            directory(
                fs::path {
                    ac::config::logger_directory()
                } /
                log_name
            ) {
        }

        ac::session::Start session_start;
        std::string name;
        fs::path directory;
        std::string logger_date;
        std::ofstream log_stream;
    };

    Logger::Logger(const std::string& log_name)
        : impl(std::make_unique<Impl>(log_name)) {
        open_log_file();
    }

    Logger::~Logger() noexcept {
        try {
            close_log_file();
        }
        catch (const std::exception& exception) {
            ac::error::log(
                "Unable to close component log: {}",
                exception.what()
            );
        }
        catch (...) {
            ac::error::log(
                "Unable to close component log: unknown error."
            );
        }
    }

    void Logger::logg(const std::string& msg) {
        write_message(
            impl->log_stream,
            impl->name,
            msg,
            true,
            LogRoute::component_only
        );
    }

    void Logger::loggnl(const std::string& msg) {
        write_message(
            impl->log_stream,
            impl->name,
            msg,
            false,
            LogRoute::component_only
        );
    }

    void Logger::logg_and_logg(const std::string& msg) {
        write_message(
            impl->log_stream,
            impl->name,
            msg,
            true,
            LogRoute::component_and_main
        );
    }

    void Logger::loggnl_and_loggnl(const std::string& msg) {
        write_message(
            impl->log_stream,
            impl->name,
            msg,
            false,
            LogRoute::component_and_main
        );
    }

    void Logger::logg_and_print(const std::string& msg) {
        write_message(
            impl->log_stream,
            impl->name,
            msg,
            true,
            LogRoute::component_main_and_console
        );
    }

    void Logger::loggnl_and_printnl(const std::string& msg) {
        write_message(
            impl->log_stream,
            impl->name,
            msg,
            false,
            LogRoute::component_main_and_console
        );
    }

    void Logger::logg(const std::wstring& msg) {
        logg(ac::encoding::to_utf8(msg));
    }

    void Logger::loggnl(const std::wstring& msg) {
        loggnl(ac::encoding::to_utf8(msg));
    }

    void Logger::logg_and_logg(const std::wstring& msg) {
        logg_and_logg(ac::encoding::to_utf8(msg));
    }

    void Logger::loggnl_and_loggnl(const std::wstring& msg) {
        loggnl_and_loggnl(ac::encoding::to_utf8(msg));
    }

    void Logger::logg_and_print(const std::wstring& msg) {
        logg_and_print(ac::encoding::to_utf8(msg));
    }

    void Logger::loggnl_and_printnl(const std::wstring& msg) {
        loggnl_and_printnl(ac::encoding::to_utf8(msg));
    }

    void Logger::logg(const char msg) {
        logg(std::string(1, msg));
    }

    void Logger::loggnl(const char msg) {
        loggnl(std::string(1, msg));
    }

    void Logger::logg_and_logg(const char msg) {
        logg_and_logg(std::string(1, msg));
    }

    void Logger::loggnl_and_loggnl(const char msg) {
        loggnl_and_loggnl(std::string(1, msg));
    }

    void Logger::logg_and_print(const char msg) {
        logg_and_print(std::string(1, msg));
    }

    void Logger::loggnl_and_printnl(const char msg) {
        loggnl_and_printnl(std::string(1, msg));
    }

    void Logger::logg(const wchar_t msg) {
        logg(ac::encoding::to_utf8(msg));
    }

    void Logger::loggnl(const wchar_t msg) {
        loggnl(ac::encoding::to_utf8(msg));
    }

    void Logger::logg_and_logg(const wchar_t msg) {
        logg_and_logg(ac::encoding::to_utf8(msg));
    }

    void Logger::loggnl_and_loggnl(const wchar_t msg) {
        loggnl_and_loggnl(ac::encoding::to_utf8(msg));
    }

    void Logger::logg_and_print(const wchar_t msg) {
        logg_and_print(ac::encoding::to_utf8(msg));
    }

    void Logger::loggnl_and_printnl(const wchar_t msg) {
        loggnl_and_printnl(ac::encoding::to_utf8(msg));
    }

    void Logger::update_log_file() {
        const std::string current_date_iso =
            ac::clock::get_date_iso();

        if (impl->logger_date == current_date_iso) {
            return;
        }

        if (impl->log_stream.is_open()) {
            impl->log_stream
                << "--- Session continues for "
                << impl->session_start.datetime
                << " ---\n";

            impl->log_stream.close();
        }

        const fs::path logger_path =
            fs::path(impl->directory) /
            (current_date_iso + "_" + impl->name + ".log");

        impl->log_stream.open(
            logger_path,
            std::ios::app
        );

        if (!impl->log_stream.is_open()) {
            ac::error::log(
                "Failed to open component log file: {}",
                logger_path
            );

            return;
        }

        impl->log_stream
            << "--- Session continues from "
            << impl->session_start.datetime
            << " ---\n";

        impl->logger_date = current_date_iso;
    }

    void Logger::open_log_file() {
        std::error_code error;

        fs::create_directories(
            impl->directory,
            error
        );

        if (error) {
            ac::error::log(
                "Failed to create component log directory: {} - {}",
                impl->directory,
                error.message()
            );

            return;
        }

        const fs::path logger_path =
            fs::path(impl->directory) /
            (
                impl->session_start.date_iso
                + "_"
                + impl->name
                + ".log"
            );

        impl->log_stream.open(
            logger_path,
            std::ios::app
        );

        if (!impl->log_stream.is_open()) {
            ac::error::log(
                "Failed to open component log file: {}",
                logger_path
            );

            return;
        }

        impl->logger_date =
            impl->session_start.date_iso;

        impl->log_stream
            << "Session started for "
            << impl->name
            << " "
            << impl->session_start.datetime
            << '\n';
    }

    void Logger::close_log_file() {
        if (!impl->log_stream.is_open()) {
            return;
        }

        const auto end =
            ac::clock::get_local_datetime();

        const std::string end_datetime = std::format(
            "{} at {}",
            end.date_iso,
            end.timestamp_with_seconds
        );

        impl->log_stream
            << "Session ended for "
            << impl->name
            << " "
            << end_datetime
            << "\n***\n";

        impl->log_stream.close();
    }

    void Logger::flush() {
        if (impl->log_stream.is_open()) {
            impl->log_stream.flush();
        }
    }
}
