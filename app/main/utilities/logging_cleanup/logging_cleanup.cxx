module auto_core.utility.logging_cleanup;

import auto_core.core.component;
import auto_core.core.logging.config;

namespace {
    ac::Component logging_cleanup {"logging_cleanup"};
}

int ac::logging_cleanup::run() {
    ac::logging::config::resolve();

    const auto experimental = ac::logging::config::experimental();
    const auto max_directory_mb =
        ac::logging::config::max_directory_mb();
    const auto retention_days =
        ac::logging::config::retention_days();

    if (experimental) {
        ::logging_cleanup.print(
            "auto_core_logging_cleanup.exe is experimental; automatic log cleanup is not yet implemented. max_directory_mb = {}; retention_days = {}",
            max_directory_mb,
            retention_days
        );
    }
    else {
        ::logging_cleanup.log_debug(
            "auto_core_logging_cleanup.exe is experimental; automatic log cleanup is not yet implemented. max_directory_mb = {}; retention_days = {}",
            max_directory_mb,
            retention_days
        );
    }
    return 0;
}
