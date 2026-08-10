module component;

import std;

import clipboard;
import clock;
import config;
import encoding;
import error;
import session;
import pipes;
import logger_x;

namespace fs = std::filesystem;

namespace {

    enum class OutputRoute {
        component,
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

}

namespace ac {

    class Component::Impl {
    public:
        explicit Impl(const std::string& component_name)
            : session_start(ac::session::make_start()),
            name(component_name),
            directory(
                fs::path {ac::config::logger_directory()} /
                component_name
            ) {
        }

        ac::session::Start session_start;
        std::string name;
        fs::path directory;
        std::string logger_date;
        std::ofstream log_stream;
        std::mutex mutex;

        void update_log_file_unlocked() {
            const std::string current_date_iso =
                ac::clock::get_date_iso();

            if (logger_date == current_date_iso) {
                return;
            }

            if (log_stream.is_open()) {
                log_stream
                    << "--- Session continues in next log file ---\n";
                log_stream.close();
            }

            const fs::path logger_path =
                directory /
                (current_date_iso + "_" + name + ".log");

            log_stream.open(
                logger_path,
                std::ios::app
            );

            if (!log_stream.is_open()) {
                ac::error::log(
                    "Failed to open component log file: {}",
                    logger_path
                );
                return;
            }

            log_stream
                << "--- Session continues from "
                << session_start.datetime
                << " ---\n";

            logger_date = current_date_iso;
        }

        void open_log_file_unlocked() {
            std::error_code error;

            fs::create_directories(
                directory,
                error
            );

            if (error) {
                ac::error::log(
                    "Failed to create component log directory: {} - {}",
                    directory,
                    error.message()
                );
                return;
            }

            const fs::path logger_path =
                directory /
                (
                    session_start.date_iso
                    + "_"
                    + name
                    + ".log"
                    );

            log_stream.open(
                logger_path,
                std::ios::app
            );

            if (!log_stream.is_open()) {
                ac::error::log(
                    "Failed to open component log file: {}",
                    logger_path
                );
                return;
            }

            logger_date = session_start.date_iso;

            log_stream
                << "Session started for "
                << name
                << " "
                << session_start.datetime
                << '\n';
        }

        void close_log_file_unlocked() {
            if (!log_stream.is_open()) {
                return;
            }

            const auto end = ac::clock::get_local_datetime();

            log_stream
                << "Session ended for "
                << name
                << " "
                << end.date_iso
                << " at "
                << end.timestamp_with_seconds
                << "\n***\n";

            log_stream.close();
        }

        void send_to_main_log(
            const std::string_view message,
            const bool newline
        ) {
            const ac::pipes::LogEvent event {
                .component = name,
                .message = std::string {message},
                .newline = newline
            };

            ac::logger::send_to_logger(event);
        }

        void write_message(
            const std::string_view message,
            const bool newline,
            const OutputRoute route
        ) {
            std::scoped_lock lock(mutex);

            update_log_file_unlocked();

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

            if (
                route == OutputRoute::component_and_main ||
                route == OutputRoute::component_main_and_console
                ) {
                send_to_main_log(message, newline);
            }

            if (route == OutputRoute::component_main_and_console) {
                write_to_stream(
                    std::cout,
                    message,
                    newline
                );

                std::cout.flush();
            }
        }
    };

    Component::Component(const std::string& name)
        : impl(std::make_unique<Impl>(name)) {
        std::scoped_lock lock(impl->mutex);
        impl->open_log_file_unlocked();
    }

    Component::~Component() noexcept {
        try {
            std::scoped_lock lock(impl->mutex);
            impl->close_log_file_unlocked();
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

    void Component::logg(const std::string& message) {
        impl->write_message(
            message,
            true,
            OutputRoute::component
        );
    }

    void Component::loggnl(const std::string& message) {
        impl->write_message(
            message,
            false,
            OutputRoute::component
        );
    }

    void Component::logg_and_logg(const std::string& message) {
        impl->write_message(
            message,
            true,
            OutputRoute::component_and_main
        );
    }

    void Component::loggnl_and_loggnl(const std::string& message) {
        impl->write_message(
            message,
            false,
            OutputRoute::component_and_main
        );
    }

    void Component::logg_and_print(const std::string& message) {
        impl->write_message(
            message,
            true,
            OutputRoute::component_main_and_console
        );
    }

    void Component::loggnl_and_printnl(const std::string& message) {
        impl->write_message(
            message,
            false,
            OutputRoute::component_main_and_console
        );
    }

    void Component::print(const std::string& message) {
        impl->write_message(
            message,
            true,
            OutputRoute::component_main_and_console
        );
    }

    void Component::printnl(const std::string& message) {
        impl->write_message(
            message,
            false,
            OutputRoute::component_main_and_console
        );
    }

    void Component::print_and_insert(const std::string& message) {
        print(message);

        try {
            std::wstring clipboard_text =
                ac::encoding::to_utf16(message);

            clipboard_text += L"\n\n";

            if (!ac::clipboard::set_clipboard_text(clipboard_text)) {
                logg_and_print("Unable to set clipboard text");
                return;
            }

            if (!ac::clipboard::paste_from_clipboard()) {
                logg_and_print(
                    "Unable to paste from clipboard because "
                    "the Ctrl+V input could not be sent"
                );
            }
        }
        catch (const std::exception& exception) {
            logg_and_print(
                "Unable to convert text for insertion: {}",
                exception.what()
            );
        }
    }

    void Component::logg(const std::wstring& message) {
        logg(ac::encoding::to_utf8(message));
    }

    void Component::loggnl(const std::wstring& message) {
        loggnl(ac::encoding::to_utf8(message));
    }

    void Component::logg_and_logg(const std::wstring& message) {
        logg_and_logg(ac::encoding::to_utf8(message));
    }

    void Component::loggnl_and_loggnl(const std::wstring& message) {
        loggnl_and_loggnl(ac::encoding::to_utf8(message));
    }

    void Component::logg_and_print(const std::wstring& message) {
        logg_and_print(ac::encoding::to_utf8(message));
    }

    void Component::loggnl_and_printnl(const std::wstring& message) {
        loggnl_and_printnl(ac::encoding::to_utf8(message));
    }

    void Component::print(const std::wstring& message) {
        print(ac::encoding::to_utf8(message));
    }

    void Component::printnl(const std::wstring& message) {
        printnl(ac::encoding::to_utf8(message));
    }

    void Component::print_and_insert(const std::wstring& message) {
        print(message);

        std::wstring clipboard_text = message;
        clipboard_text += L"\n\n";

        ac::clipboard::set_clipboard_text(clipboard_text);
        ac::clipboard::paste_from_clipboard();
    }

    void Component::logg(const char message) {
        logg(std::string(1, message));
    }

    void Component::loggnl(const char message) {
        loggnl(std::string(1, message));
    }

    void Component::logg_and_logg(const char message) {
        logg_and_logg(std::string(1, message));
    }

    void Component::loggnl_and_loggnl(const char message) {
        loggnl_and_loggnl(std::string(1, message));
    }

    void Component::logg_and_print(const char message) {
        logg_and_print(std::string(1, message));
    }

    void Component::loggnl_and_printnl(const char message) {
        loggnl_and_printnl(std::string(1, message));
    }

    void Component::print(const char message) {
        print(std::string(1, message));
    }

    void Component::printnl(const char message) {
        printnl(std::string(1, message));
    }

    void Component::logg(const wchar_t message) {
        logg(ac::encoding::to_utf8(message));
    }

    void Component::loggnl(const wchar_t message) {
        loggnl(ac::encoding::to_utf8(message));
    }

    void Component::logg_and_logg(const wchar_t message) {
        logg_and_logg(ac::encoding::to_utf8(message));
    }

    void Component::loggnl_and_loggnl(const wchar_t message) {
        loggnl_and_loggnl(ac::encoding::to_utf8(message));
    }

    void Component::logg_and_print(const wchar_t message) {
        logg_and_print(ac::encoding::to_utf8(message));
    }

    void Component::loggnl_and_printnl(const wchar_t message) {
        loggnl_and_printnl(ac::encoding::to_utf8(message));
    }

    void Component::print(const wchar_t message) {
        print(ac::encoding::to_utf8(message));
    }

    void Component::printnl(const wchar_t message) {
        printnl(ac::encoding::to_utf8(message));
    }

    void Component::update_log_file() {
        std::scoped_lock lock(impl->mutex);
        impl->update_log_file_unlocked();
    }

    void Component::flush() {
        std::scoped_lock lock(impl->mutex);

        if (impl->log_stream.is_open()) {
            impl->log_stream.flush();
        }
    }
}
