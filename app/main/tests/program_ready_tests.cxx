#include "catch_amalgamated.hpp"
#include "program_ready_detail.hpp"

import auto_core.main.defaults;

using ac::main::startup::Settings;

namespace {

constexpr Settings startup_defaults {
    .show_day_of_week = ac::main::defaults::startup_show_day_of_week,
    .show_task_list = ac::main::defaults::startup_show_task_list
};

std::string message_for(
    const Settings settings,
    int& day_reads,
    int& task_reads,
    const std::string& task_summary = "Today's task list:\nShip it"
) {
    return ac::main::startup::assemble_message(
        settings,
        [&] {
            ++day_reads;
            return std::string {"Thursday"};
        },
        [&] {
            ++task_reads;
            return task_summary;
        }
    );
}

} // namespace

TEST_CASE("Startup banner supports every display combination", "[startup][unit]") {
    int day_reads = 0;
    int task_reads = 0;

    CHECK(message_for({true, true}, day_reads, task_reads) ==
        "Program ready\nToday is Thursday\nToday's task list:\nShip it");
    CHECK(day_reads == 1);
    CHECK(task_reads == 1);

    day_reads = 0;
    task_reads = 0;
    CHECK(message_for({true, false}, day_reads, task_reads) ==
        "Program ready\nToday is Thursday");
    CHECK(day_reads == 1);
    CHECK(task_reads == 0);

    day_reads = 0;
    task_reads = 0;
    CHECK(message_for({false, true}, day_reads, task_reads) ==
        "Program ready\nToday's task list:\nShip it");
    CHECK(day_reads == 0);
    CHECK(task_reads == 1);

    day_reads = 0;
    task_reads = 0;
    CHECK(message_for({false, false}, day_reads, task_reads) ==
        "Program ready");
    CHECK(day_reads == 0);
    CHECK(task_reads == 0);
}

TEST_CASE("Missing startup settings preserve current behavior", "[startup][unit]") {
    const auto resolution = ac::main::startup::resolve(
        std::nullopt,
        std::nullopt,
        startup_defaults
    );
    CHECK(resolution.settings.show_day_of_week);
    CHECK(resolution.settings.show_task_list);

    int day_reads = 0;
    int task_reads = 0;
    CHECK(message_for(
        resolution.settings,
        day_reads,
        task_reads,
        "Nothing pending today."
    ) == "Program ready\nToday is Thursday\nNothing pending today.");
}

TEST_CASE("Invalid startup values independently use on defaults", "[startup][unit]") {
    const auto resolution = ac::main::startup::resolve(
        "yes",
        "OFF",
        startup_defaults
    );
    CHECK(resolution.settings.show_day_of_week);
    CHECK(resolution.settings.show_task_list);
    CHECK(resolution.day_of_week_status ==
        ac::main::startup::ValueStatus::invalid);
    CHECK(resolution.task_list_status ==
        ac::main::startup::ValueStatus::invalid);
}
