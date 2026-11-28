module;

#include "merge_detail.hpp"

module auto_core.utility.log_merge;

import std;
import auto_core.core.component;
import auto_core.core.error;
import auto_core.core.ini;
import auto_core.core.logging.config;
import auto_core.core.paths;

namespace {

    struct Policy {
        std::uint64_t merge_interval_seconds = 5;
        bool merge_logs_on_shutdown = true;

        Policy() {
            const auto ini_path =
                ac::paths::config_directory() / "logging_policy.ini";
            const auto document = ac::ini::read(ini_path);
            if (!document) {
                return;
            }
            const auto interval = document->find(
                "log_merge", "merge_interval_seconds"
            );
            if (const auto parsed =
                    ac::logger::detail::parse_interval(interval)) {
                merge_interval_seconds = *parsed;
            }
            const auto shutdown_merge = document->find(
                "log_merge", "merge_logs_on_shutdown"
            );
            if (const auto parsed =
                    ac::logger::detail::parse_on_off(shutdown_merge)) {
                merge_logs_on_shutdown = *parsed;
            }
        }
    };

    const Policy& policy() {
        static const Policy value;
        return value;
    }

    ac::Component& reporter() {
        static ac::Component component {
            "auto_core_logger",
            ac::logging::config::LoggingScope {"auto_core"},
            ac::SharedLog {"auto_core"}
        };
        return component;
    }

    ac::log_merge::Result from_detail(
        const ac::logger::detail::MergeResult& result
    ) {
        return {
            .ok = result.ok,
            .message = result.message,
            .stopped = result.stopped
        };
    }

} // namespace

std::uint64_t ac::log_merge::merge_interval_seconds() {
    return policy().merge_interval_seconds;
}

bool ac::log_merge::merge_logs_on_shutdown() {
    return policy().merge_logs_on_shutdown;
}

ac::log_merge::Result ac::log_merge::merge_once(
    const std::filesystem::path& log_directory,
    const std::string_view only_date
) {
    return from_detail(ac::logger::detail::merge_once(
        log_directory,
        only_date
    ));
}

ac::log_merge::Result ac::log_merge::merge_once_interruptible(
    const std::filesystem::path& log_directory,
    void* stop_event,
    const std::string_view only_date
) {
    return from_detail(ac::logger::detail::merge_once_interruptible(
        log_directory,
        static_cast<HANDLE>(stop_event),
        only_date
    ));
}

void ac::log_merge::report(const Result& result) {
    if (result.ok || result.stopped) {
        return;
    }
    const auto message = result.message.empty()
        ? std::string {"Unable to merge logs."}
        : result.message;
    ac::error::log(message);
    reporter().log_alert("{}", message);
}

void ac::log_merge::log_refresh_success() {
    reporter().log_alert("Main log file updated.");
}

void ac::log_merge::log_periodic(const std::uint64_t interval_seconds) {
    reporter().log_debug("Periodic merge ({}s)", interval_seconds);
}

void ac::log_merge::log_oneshot(const bool shutdown_launch) {
    if (shutdown_launch) {
        reporter().log_info("Shutdown one-shot merge");
        return;
    }
    reporter().log_debug("One-shot merge");
}
