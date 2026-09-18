module log_init;

import std;
import auto_core.core.clock;
import logger_state;
import main_log;
import auto_core.core.logging.protocol;

void log_init() {
    const ac::logging::Event session_event {
        .timestamp = ac::clock::get_log_timestamp(),
        .component = "logger",
        .message = std::format(
            "Main log session started at {}",
            ac::clock::format_datetime(logger_component.session_start())
        ),
        .newline = true
    };

    write_to_main_log(session_event);

    logger_component.log("logger_ac.exe started");

    const ac::logging::Event start_event {
        .timestamp = ac::clock::get_log_timestamp(),
        .component = "logger",
        .message = "logger_ac.exe started",
        .newline = true
    };

    write_to_main_log(start_event);
}
