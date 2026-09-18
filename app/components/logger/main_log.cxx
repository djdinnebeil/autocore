module main_log;

import std;
import auto_core.core.clock;
import auto_core.core.logging.config;
import logger_state;
import auto_core.core.logging.protocol;

namespace fs = std::filesystem;

namespace {

    std::ofstream main_log_stream;
    std::string main_log_date;
    std::string main_line_component;
    bool main_line_open = false;
    bool main_log_online = true;
    std::mutex main_log_mutex;

    std::string event_date(const ac::logging::Event& event) {
        if (
            event.timestamp.size() >= 10 &&
            event.timestamp[4] == '-' &&
            event.timestamp[7] == '-'
        ) {
            return event.timestamp.substr(0, 10);
        }
        return ac::clock::get_date_iso();
    }

    void close_open_line_unlocked() {
        if (main_log_stream.is_open() && main_line_open) {
            main_log_stream << '\n';
            main_line_open = false;
            main_line_component.clear();
        }
    }

    void write_event_unlocked(const ac::logging::Event& event) {
        if (!main_log_stream.is_open()) {
            return;
        }

        if (
            main_line_open &&
            main_line_component != event.component
        ) {
            close_open_line_unlocked();
        }

        std::size_t offset = 0;
        while (offset < event.message.size()) {
            if (!main_line_open) {
                main_log_stream
                    << '[' << event.timestamp << "] ["
                    << event.component << "] ";
                main_line_open = true;
                main_line_component = event.component;
            }

            const std::size_t end = event.message.find('\n', offset);
            if (end == std::string::npos) {
                main_log_stream << event.message.substr(offset);
                offset = event.message.size();
            }
            else {
                main_log_stream
                    << event.message.substr(offset, end - offset)
                    << '\n';
                main_line_open = false;
                main_line_component.clear();
                offset = end + 1;
            }
        }

        if (event.message.empty() && !main_line_open) {
            main_log_stream
                << '[' << event.timestamp << "] ["
                << event.component << "] ";
            main_line_open = true;
            main_line_component = event.component;
        }

        if (event.newline && main_line_open) {
            main_log_stream << '\n';
            main_line_open = false;
            main_line_component.clear();
        }

        main_log_stream.flush();
    }

    void update_main_log_file_unlocked(
        const ac::logging::Event& event
    ) {
        const std::string target_date = event_date(event);

        if (
            main_log_stream.is_open() &&
            main_log_date == target_date
        ) {
            return;
        }

        const bool continuing_session = main_log_stream.is_open();

        if (continuing_session) {
            close_open_line_unlocked();
            const ac::logging::Event marker {
                .timestamp = event.timestamp,
                .component = "logger",
                .message = "--- Session continues in next log file ---",
                .newline = true
            };
            write_event_unlocked(marker);
            main_log_stream.close();
        }

        const fs::path logger_path =
            ac::logging::config::directory() /
            (target_date + "_main.log");

        main_log_stream.open(logger_path, std::ios::app);

        if (!main_log_stream.is_open()) {
            std::cerr
                << "Failed to open main log file: "
                << logger_path
                << '\n';
            return;
        }

        main_log_date = target_date;

        if (continuing_session) {
            const ac::logging::Event marker {
                .timestamp = event.timestamp,
                .component = "logger",
                .message = std::format(
                    "--- Session continues from {} ---",
                    ac::clock::format_datetime(
                        logger_component.session_start()
                    )
                ),
                .newline = true
            };
            write_event_unlocked(marker);
        }
    }

    void write_to_main_log_unlocked(
        const ac::logging::Event& event
    ) {
        update_main_log_file_unlocked(event);
        write_event_unlocked(event);
    }

} // namespace

void write_to_main_log(const ac::logging::Event& event) {
    std::scoped_lock lock(main_log_mutex);
    if (!main_log_online) {
        return;
    }
    write_to_main_log_unlocked(event);
}

void shutdown_main_log(const ac::logging::Event& request) {
    std::scoped_lock lock(main_log_mutex);
    if (!main_log_online) {
        return;
    }

    const ac::logging::Event shutdown_event {
        .timestamp = request.timestamp,
        .component = request.component,
        .message = "logger_ac.exe is shutting down. Centralized logging is "
            "offline. See component log files for further shutdown details.",
        .newline = true
    };
    write_to_main_log_unlocked(shutdown_event);
    main_log_online = false;
}
