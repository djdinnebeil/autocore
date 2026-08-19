module main_log;

import std;
import auto_core.clock;
import auto_core.logging.config;
import logger_state;
import auto_core.logging.protocol;

namespace fs = std::filesystem;

namespace {

    std::ofstream main_log_stream;
    std::string main_log_date;
    bool main_line_open = false;
    std::mutex main_log_mutex;

    void update_main_log_file_unlocked();
}

namespace {

void update_main_log_file_unlocked() {
    const std::string current_date =
        ac::clock::get_date_iso();

    if (
        main_log_stream.is_open() &&
        main_log_date == current_date
        ) {
        return;
    }

    const bool continuing_session = main_log_stream.is_open();

    if (continuing_session) {
        if (main_line_open) {
            main_log_stream << '\n';
            main_line_open = false;
        }

        main_log_stream
            << "--- Session continues in next log file ---\n";
        main_log_stream.flush();
        main_log_stream.close();
    }

    const std::string main_log_name =
        current_date + "_main.log";

    const fs::path logger_path =
        ac::logging::config::directory() /
        main_log_name;

    main_log_stream.open(
        logger_path,
        std::ios::app
    );

    if (!main_log_stream.is_open()) {
        std::cerr
            << "Failed to open main log file: "
            << logger_path
            << '\n';

        return;
    }

    main_log_date = current_date;

    if (continuing_session) {
        const ac::logging::Event continuation_event {
            .component = "logger",
            .message = std::format(
                "--- Session continues from {} ---",
                ac::clock::format_datetime(logger_component.session_start())
            ),
            .newline = true
        };

        main_log_stream
            << continuation_event.message
            << '\n';
        main_log_stream.flush();
    }
}

}

void write_to_main_log(
    const ac::logging::Event& event
) {
    std::scoped_lock lock(main_log_mutex);
    update_main_log_file_unlocked();

    if (!main_log_stream.is_open()) {
        return;
    }

    if (!main_line_open) {
        main_log_stream
            << '['
            << event.component
            << "] ";
    }

    main_log_stream << event.message;

    if (event.newline) {
        main_log_stream << '\n';
        main_line_open = false;
    }
    else {
        main_line_open = true;
    }

    main_log_stream.flush();
}
