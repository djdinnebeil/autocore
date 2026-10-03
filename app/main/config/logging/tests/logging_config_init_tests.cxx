#include "catch_amalgamated.hpp"

#include <sstream>
#include <string>

import auto_core.main.defaults;
import auto_core_initialization;
import logger_init_detail;
import logging_config_detail;

namespace logging = ac::main::logging;
namespace sequence = ac::main::init;
namespace logger_menu = ac::main::logger_init;

namespace {

constexpr std::string_view default_ini =
    "[logging]\n"
    "disable_all = off\n"
    "directory = logs\n"
    "write_logs_to_files = on\n"
    "write_logs_to_console = off\n"
    "log_print_mode = print\n"
    "component_logging_default = on\n";

constexpr std::string_view disabled_ini =
    "[logging]\n"
    "disable_all = on\n"
    "directory = logs\n"
    "write_logs_to_files = on\n"
    "write_logs_to_console = off\n"
    "log_print_mode = print\n"
    "component_logging_default = on\n";

[[nodiscard]] bool before(
    const std::string& text,
    const std::string_view earlier,
    const std::string_view later
) {
    const auto first = text.find(earlier);
    const auto second = text.find(later);
    return first != std::string::npos &&
        second != std::string::npos &&
        first < second;
}

} // namespace

TEST_CASE("Fresh seed writes the exact default file", "[logging][init]") {
    CHECK(logging::select_action(false, false, true) == logging::Action::seed);
    CHECK(std::string {ac::main::defaults::logging_ini} == default_ini);

    std::string written;
    const auto result = logging::commit_seed(false, [&](const std::string_view text) {
        written = std::string {text};
        return true;
    });

    CHECK(result.code == 0);
    CHECK(result.reported_success);
    CHECK(written == default_ini);
}

TEST_CASE("Init with seed behaves as seed", "[logging][init]") {
    CHECK(logging::select_action(false, true, true) == logging::Action::seed);
    CHECK(logging::select_action(true, true, true) == logging::Action::skip_seed);

    std::string written;
    const auto fresh = logging::commit_seed(false, [&](const std::string_view text) {
        written = std::string {text};
        return true;
    });
    CHECK(fresh.code == 0);
    CHECK(written == default_ini);
}

TEST_CASE("Configure prompts all six settings in canonical order", "[logging][init]") {
    std::istringstream input {"on\narchive\noff\non\nlog\noff\n"};
    std::ostringstream output;
    const auto values = logging::prompt_values(
        input,
        output,
        logging::compiled_defaults()
    );

    REQUIRE(values);
    CHECK(logging::ini_text(*values) ==
        "[logging]\n"
        "disable_all = on\n"
        "directory = archive\n"
        "write_logs_to_files = off\n"
        "write_logs_to_console = on\n"
        "log_print_mode = log\n"
        "component_logging_default = off\n");

    const auto shown = output.str();
    CHECK(before(shown, "disable_all [off]: ", "directory [logs]: "));
    CHECK(before(shown, "directory [logs]: ", "write_logs_to_files [on]: "));
    CHECK(before(
        shown,
        "write_logs_to_files [on]: ",
        "write_logs_to_console [off]: "
    ));
    CHECK(before(
        shown,
        "write_logs_to_console [off]: ",
        "log_print_mode [print]: "
    ));
    CHECK(before(
        shown,
        "log_print_mode [print]: ",
        "component_logging_default [on]: "
    ));
}

TEST_CASE("Disable writes the complete file with only disable_all on", "[logging][init]") {
    CHECK(logging::select_action(false, false, false, true) == logging::Action::disable);
    CHECK(logging::select_action(true, false, false, true) ==
        logging::Action::skip_disable);

    std::string written;
    const auto fresh = logging::commit_disable(false, [&](const std::string_view text) {
        written = std::string {text};
        return true;
    });
    CHECK(fresh.code == 0);
    CHECK(fresh.reported_success);
    CHECK(written == disabled_ini);

    int writes = 0;
    const auto existing = logging::commit_disable(true, [&](const std::string_view) {
        ++writes;
        return true;
    });
    CHECK(existing.code == 0);
    CHECK_FALSE(existing.reported_success);
    CHECK(writes == 0);
}

TEST_CASE("Existing logging.ini is preserved", "[logging][init]") {
    CHECK(logging::select_action(true, false, true) == logging::Action::skip_seed);
    CHECK(logging::select_action(true, true, false) ==
        logging::Action::skip_initialization);

    int writes = 0;
    const auto seeded = logging::commit_seed(true, [&](const std::string_view) {
        ++writes;
        return true;
    });
    CHECK(seeded.code == 0);
    CHECK_FALSE(seeded.reported_success);
    CHECK(writes == 0);
}

TEST_CASE("Failed write does not report initialization success", "[logging][init]") {
    const auto seeded = logging::commit_seed(false, [](const std::string_view) {
        return false;
    });
    CHECK(seeded.code == 1);
    CHECK_FALSE(seeded.reported_success);

    const auto initialized = logging::commit_text(
        default_ini,
        [](const std::string_view) {
            return false;
        }
    );
    CHECK(initialized.code == 1);
    CHECK_FALSE(initialized.reported_success);

    const auto disabled = logging::commit_disable(false, [](const std::string_view) {
        return false;
    });
    CHECK(disabled.code == 1);
    CHECK_FALSE(disabled.reported_success);
}

TEST_CASE("Configure cancellation returns no values", "[logging][init]") {
    std::istringstream input {};
    std::ostringstream output;
    CHECK_FALSE(logging::prompt_values(
        input,
        output,
        logging::compiled_defaults()
    ));
}

TEST_CASE("Auto Core choice selects only auto_core_config", "[logging][init]") {
    REQUIRE(sequence::initialization_steps.size() >= 3);
    CHECK(sequence::initialization_steps[0].executable == "auto_core_config.exe");
    CHECK(sequence::initialization_steps[0].kind == sequence::OwnerKind::auto_core);
    CHECK(sequence::arguments_for(sequence::OwnerKind::auto_core, true) == L"--seed");
    CHECK(sequence::arguments_for(sequence::OwnerKind::auto_core, false) == L"--init");

    CHECK(sequence::initialization_steps[1].executable == "logger_init.exe");
    CHECK(sequence::arguments_for(sequence::OwnerKind::logger, true).empty());
    CHECK(sequence::arguments_for(sequence::OwnerKind::logger, false).empty());

    CHECK(sequence::initialization_steps[2].executable == "components_init.exe");
    CHECK(sequence::arguments_for(sequence::OwnerKind::components, true).empty());
    CHECK(sequence::arguments_for(sequence::OwnerKind::components, false).empty());

    for (const auto& step : sequence::initialization_steps) {
        CHECK(step.executable != "logging_config.exe");
        CHECK(step.executable != "logger_config.exe");
        CHECK(sequence::stops_on_failure(step.kind));
    }
}

TEST_CASE("Logger menu offers three independent choices", "[logging][init]") {
    std::istringstream invalid {"9\n1\n"};
    std::ostringstream output;
    const auto defaults = logger_menu::prompt_menu(invalid, output);
    REQUIRE(defaults);
    CHECK(*defaults == logger_menu::Choice::use_defaults);
    CHECK(output.str().find("Enter 1, 2, or 3.") != std::string::npos);
    CHECK(output.str().find("1. Use defaults") != std::string::npos);
    CHECK(output.str().find("2. Configure") != std::string::npos);
    CHECK(output.str().find("3. Disable logging") != std::string::npos);

    std::istringstream configure {"2\n"};
    std::ostringstream configure_output;
    const auto configured = logger_menu::prompt_menu(configure, configure_output);
    REQUIRE(configured);
    CHECK(*configured == logger_menu::Choice::configure);

    std::istringstream disable {"3\n"};
    std::ostringstream disable_output;
    const auto disabled = logger_menu::prompt_menu(disable, disable_output);
    REQUIRE(disabled);
    CHECK(*disabled == logger_menu::Choice::disable_logging);

    std::istringstream eof {};
    std::ostringstream eof_output;
    CHECK_FALSE(logger_menu::prompt_menu(eof, eof_output));
}

TEST_CASE("Logger choices launch owners and do not write INI text", "[logging][init]") {
    const auto defaults = logger_menu::launches_for(logger_menu::Choice::use_defaults);
    REQUIRE(defaults.size() == 2);
    CHECK(defaults[0].executable == "logging_config.exe");
    CHECK(defaults[0].arguments == L"--seed");
    CHECK(defaults[1].executable == "logger_config.exe");
    CHECK(defaults[1].arguments == L"--seed");

    const auto configure = logger_menu::launches_for(logger_menu::Choice::configure);
    CHECK(configure[0].arguments == L"--init");
    CHECK(configure[1].arguments == L"--init");

    const auto disable = logger_menu::launches_for(logger_menu::Choice::disable_logging);
    CHECK(disable[0].executable == "logging_config.exe");
    CHECK(disable[0].arguments == L"--disable");
    CHECK(disable[1].executable == "logger_config.exe");
    CHECK(disable[1].arguments == L"--seed");
}
