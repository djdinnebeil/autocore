module main_log;

import std;
import clock;
import config;
import pipes;

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

    if (main_log_stream.is_open()) {
        main_log_stream.close();
    }

    const std::string main_log_name =
        current_date + "_auto_core.log";

    const fs::path logger_path =
        fs::path {
            std::string {
                ac::config::logger_directory()
            }
    } /
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
}

}

void update_main_log_file() {
    std::scoped_lock lock(main_log_mutex);
    update_main_log_file_unlocked();
}

void write_to_main_log(
    const ac::pipes::LogEvent& event
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
