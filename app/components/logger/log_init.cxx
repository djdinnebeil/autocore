module log_init;

import std;
import auto_core.clock;
import logger_state;
import main_log;
import auto_core.logging.protocol;

void log_init() {
    const ac::logging::Event session_event {
        .component = "logger",
        .message = std::format(
            "Main log session started at {}",
            ac::clock::format_datetime(logger_component.session_start())
        ),
        .newline = true
    };

    write_to_main_log(session_event);

    logger_component.logg("logger.exe started");

    const ac::logging::Event start_event {
        .component = "logger",
        .message = "logger.exe started",
        .newline = true
    };

    write_to_main_log(start_event);
}
