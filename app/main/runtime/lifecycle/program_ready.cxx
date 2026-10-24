module;

#include "program_ready_detail.hpp"

module auto_core.main.program_ready;

import std;
import auto_core.main.application;
import auto_core.main.defaults;
import auto_core.core.clock;
import auto_core.core.ini;
import auto_core.core.paths;

namespace {

constexpr ac::main::startup::Settings startup_defaults {
    .show_day_of_week = ac::main::defaults::startup_show_day_of_week,
    .show_task_list = ac::main::defaults::startup_show_task_list
};

struct LoadedStartupSettings {
    ac::main::startup::Settings settings;
    std::string report;
};

void append_report(std::string& report, const std::string_view message) {
    if (!report.empty()) {
        report.push_back(' ');
    }
    report += message;
}

void report_setting_status(
    std::string& report,
    const std::string_view key,
    const ac::main::startup::ValueStatus status
) {
    if (status == ac::main::startup::ValueStatus::missing) {
        append_report(
            report,
            std::string {"config/startup.ini is missing "} +
                std::string {key} +
                ". Using its default; the file will not be rewritten."
        );
    }
    else if (status == ac::main::startup::ValueStatus::invalid) {
        append_report(
            report,
            std::string {"config/startup.ini "} +
                std::string {key} +
                " must be 'on' or 'off'. Using its default; the file "
                "will not be rewritten."
        );
    }
}

LoadedStartupSettings load_startup_settings() {
    const std::filesystem::path path =
        ac::paths::config_directory() / "startup.ini";
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        std::error_code exists_error;
        const bool present = std::filesystem::exists(path, exists_error);
        return {
            .settings = startup_defaults,
            .report = present || exists_error
                ? "config/startup.ini is unreadable; using built-in defaults."
                : "config/startup.ini is missing; using built-in defaults."
        };
    }

    const std::string contents {
        std::istreambuf_iterator<char> {input},
        std::istreambuf_iterator<char> {}
    };
    if (input.bad()) {
        return {
            .settings = startup_defaults,
            .report =
                "config/startup.ini is unreadable; using built-in defaults."
        };
    }

    // startup.ini is deliberately sectionless. Supplying a private section
    // lets it share the project's INI parser without changing other files.
    const auto document = ac::ini::parse("[startup]\n" + contents);
    const auto resolution = ac::main::startup::resolve(
        document.find("startup", "show_day_of_week"),
        document.find("startup", "show_task_list"),
        startup_defaults
    );

    LoadedStartupSettings loaded {.settings = resolution.settings};
    report_setting_status(
        loaded.report,
        "show_day_of_week",
        resolution.day_of_week_status
    );
    report_setting_status(
        loaded.report,
        "show_task_list",
        resolution.task_list_status
    );
    return loaded;
}

std::string read_task_list_summary() {
    const std::filesystem::path task_list_path =
        ac::paths::writer_directory() / "task_list.txt";
    std::ifstream file(task_list_path, std::ios::binary);
    if (!file) {
        auto_core.log_alert(
            "Unable to read task list: {}", task_list_path.string()
        );
        return {};
    }

    std::string task_list;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!task_list.empty()) {
            task_list += '\n';
        }
        task_list += line;
    }
    if (file.bad()) {
        auto_core.log_alert(
            "Unable to read complete task list: {}",
            task_list_path.string()
        );
        return {};
    }
    if (task_list.empty()) {
        return "Nothing pending today.";
    }
    return "Today's task list:\n" + task_list;
}

void print_program_ready() {
    const LoadedStartupSettings startup = load_startup_settings();
    if (!startup.report.empty()) {
        auto_core.log_alert("{}", startup.report);
    }
    auto_core.print(
        ac::main::startup::assemble_message(
            startup.settings,
            [] { return ac::clock::get_day_of_week(); },
            [] { return read_task_list_summary(); }
        )
    );
}

} // namespace

void announce_program_ready() {
    print_program_ready();
}
