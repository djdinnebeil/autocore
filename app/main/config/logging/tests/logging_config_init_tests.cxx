#include "catch_amalgamated.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

import auto_core.main.defaults;
import auto_core_initialization;
import logging_bootstrap;
import logging_config_detail;

namespace logging = ac::main::logging;
namespace sequence = ac::main::init;
namespace boot = ac::main::logging_bootstrap;

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
    CHECK(written == logging::ini_text(logging::compiled_defaults()));
    CHECK(written.find("# disable_all = on | off\n") == 0);
    CHECK(written.find("# directory") == std::string::npos);
    CHECK(written.find(default_ini) != std::string::npos);
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
    CHECK(written == logging::ini_text(logging::compiled_defaults()));
}

TEST_CASE("Logging text keeps canonical keys and closed-value comments", "[logging][init]") {
    logging::Values values;
    values.disable_all = true;
    values.directory = "archive";
    values.write_logs_to_files = false;
    values.write_logs_to_console = true;
    values.log_print_mode = "log";
    values.component_logging_default = false;

    const auto text = logging::ini_text(values);
    CHECK(text.find("# disable_all = on | off\n") != std::string::npos);
    CHECK(text.find("# log_print_mode = log | print\n") != std::string::npos);
    CHECK(text.find("# directory") == std::string::npos);
    CHECK(before(text, "# component_logging_default = on | off\n", "[logging]\n"));
    CHECK(text.find(
        "[logging]\n"
        "disable_all = on\n"
        "directory = archive\n"
        "write_logs_to_files = off\n"
        "write_logs_to_console = on\n"
        "log_print_mode = log\n"
        "component_logging_default = off\n"
    ) != std::string::npos);
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
    CHECK(written == logging::ini_text(logging::disabled_defaults()));
    CHECK(written.find(disabled_ini) != std::string::npos);

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

TEST_CASE("Initialization mode propagates before components_init", "[logging][init]") {
    REQUIRE(sequence::initialization_steps.size() == 8);
    CHECK(sequence::initialization_steps[0].executable == "auto_core_config.exe");
    CHECK(sequence::initialization_steps[1].executable == "components_config.exe");
    CHECK(sequence::initialization_steps[2].executable == "shutdown_config.exe");
    CHECK(sequence::initialization_steps[3].executable == "crash_recovery_config.exe");
    CHECK(sequence::initialization_steps[4].executable == "keymap_config.exe");
    CHECK(sequence::initialization_steps[5].executable == "components_init.exe");
    CHECK(sequence::initialization_steps[6].executable == "components_editor.exe");
    CHECK(sequence::initialization_steps[7].executable == "keymap_editor.exe");

    CHECK(sequence::initialization_steps[0].kind == sequence::OwnerKind::configured);
    CHECK(sequence::initialization_steps[5].kind == sequence::OwnerKind::components);
    CHECK(sequence::initialization_steps[6].kind == sequence::OwnerKind::components_editor);
    CHECK(sequence::initialization_steps[7].kind == sequence::OwnerKind::keymap_editor);

    using sequence::InitializationMode;
    CHECK(sequence::arguments_for(
        sequence::OwnerKind::configured,
        InitializationMode::seed
    ) == L"--seed");
    CHECK(sequence::arguments_for(
        sequence::OwnerKind::configured,
        InitializationMode::interactive
    ) == L"--init");
    CHECK(sequence::arguments_for(
        sequence::OwnerKind::configured,
        InitializationMode::seeded_interactive
    ) == L"--init --seed");
    CHECK(sequence::arguments_for(
        sequence::OwnerKind::components,
        InitializationMode::interactive
    ).empty());
    CHECK(sequence::arguments_for(
        sequence::OwnerKind::components,
        InitializationMode::seed
    ).empty());
    CHECK(sequence::arguments_for(
        sequence::OwnerKind::components,
        InitializationMode::seeded_interactive
    ).empty());
    CHECK(sequence::component_arguments(sequence::ComponentLaunch::seed) == L"--seed");
    CHECK(sequence::component_arguments(
        sequence::ComponentLaunch::configure
    ) == L"--init");
    CHECK(sequence::component_arguments(sequence::ComponentLaunch::skip).empty());
    CHECK(sequence::arguments_for(
        sequence::OwnerKind::components_editor,
        InitializationMode::interactive
    ) == L"--seed");
    CHECK(sequence::arguments_for(
        sequence::OwnerKind::keymap_editor,
        InitializationMode::interactive
    ).empty());

    bool saw_components_init = false;
    for (const auto& step : sequence::initialization_steps) {
        CHECK(step.executable != "logging_config.exe");
        CHECK(step.executable != "logger_config.exe");
        CHECK(step.executable != "logger_init.exe");
        CHECK(sequence::stops_on_failure(step.kind));
        if (step.kind == sequence::OwnerKind::configured) {
            CHECK_FALSE(saw_components_init);
        }
        if (step.executable == "components_init.exe") {
            saw_components_init = true;
        }
        if (step.kind == sequence::OwnerKind::components_editor ||
            step.kind == sequence::OwnerKind::keymap_editor) {
            CHECK(saw_components_init);
        }
    }
}

TEST_CASE("Interactive logging bootstrap selects seed or init", "[logging][init]") {
    CHECK(boot::init_mode(false, false) == boot::InitMode::interactive);
    CHECK(boot::init_mode(true, false) == boot::InitMode::interactive);
    CHECK(boot::init_mode(false, true) == boot::InitMode::seed);
    CHECK(boot::init_mode(true, true) == boot::InitMode::interactive_seed);

    std::istringstream blank_then_blank {"\n\n"};
    std::ostringstream blank_output;
    const auto blank = boot::select_logging_config(
        boot::InitMode::interactive,
        false,
        blank_then_blank,
        blank_output
    );
    CHECK_FALSE(blank.cancelled);
    CHECK(blank.arguments == boot::ConfigArguments::seed);
    CHECK(boot::config_arguments(blank.arguments) == L"--seed");
    CHECK(blank_output.str().find("Enable logging? [Y/n]:") != std::string::npos);
    CHECK(blank_output.str().find("1. Use defaults") != std::string::npos);

    std::istringstream yes_configure {"Y\n2\n"};
    std::ostringstream configure_output;
    const auto configured = boot::select_logging_config(
        boot::InitMode::interactive,
        false,
        yes_configure,
        configure_output
    );
    CHECK_FALSE(configured.cancelled);
    CHECK(configured.arguments == boot::ConfigArguments::initialize);
    CHECK(boot::config_arguments(configured.arguments) == L"--init");

    std::istringstream lower_yes {"y\n1\n"};
    std::ostringstream lower_output;
    const auto lower = boot::select_logging_config(
        boot::InitMode::interactive,
        false,
        lower_yes,
        lower_output
    );
    CHECK(lower.arguments == boot::ConfigArguments::seed);

    std::istringstream invalid {"maybe\nn\n"};
    std::ostringstream invalid_output;
    const auto disabled = boot::select_logging_config(
        boot::InitMode::interactive,
        false,
        invalid,
        invalid_output
    );
    CHECK_FALSE(disabled.cancelled);
    CHECK(disabled.arguments == boot::ConfigArguments::disable);
    CHECK(boot::config_arguments(disabled.arguments) == L"--disable");
    CHECK(invalid_output.str().find("Enter Y or N.") != std::string::npos);
    CHECK(invalid_output.str().find("Use defaults") == std::string::npos);

    std::istringstream menu_invalid {"\n9\n2\n"};
    std::ostringstream menu_output;
    const auto menu = boot::select_logging_config(
        boot::InitMode::interactive,
        false,
        menu_invalid,
        menu_output
    );
    CHECK(menu.arguments == boot::ConfigArguments::initialize);
    CHECK(menu_output.str().find("Enter 1 or 2.") != std::string::npos);
}

TEST_CASE("Seed modes do not show the logging configure menu", "[logging][init]") {
    std::istringstream unread {"2\n"};
    std::ostringstream seed_output;
    const auto seeded = boot::select_logging_config(
        boot::InitMode::seed,
        false,
        unread,
        seed_output
    );
    CHECK_FALSE(seeded.cancelled);
    CHECK(seeded.arguments == boot::ConfigArguments::seed);
    CHECK(seed_output.str().empty());
    CHECK(unread.peek() == '2');

    std::istringstream enabled {"\n"};
    std::ostringstream enabled_output;
    const auto interactive_seed = boot::select_logging_config(
        boot::InitMode::interactive_seed,
        false,
        enabled,
        enabled_output
    );
    CHECK(interactive_seed.arguments == boot::ConfigArguments::seed);
    CHECK(enabled_output.str().find("Enable logging? [Y/n]:") != std::string::npos);
    CHECK(enabled_output.str().find("Use defaults") == std::string::npos);

    std::istringstream declined {"N\n"};
    std::ostringstream declined_output;
    const auto no = boot::select_logging_config(
        boot::InitMode::interactive_seed,
        false,
        declined,
        declined_output
    );
    CHECK(no.arguments == boot::ConfigArguments::disable);
    CHECK(declined_output.str().find("Use defaults") == std::string::npos);
}

TEST_CASE("Existing logging.ini skips the logging bootstrap prompt", "[logging][init]") {
    std::istringstream unread {"n\n"};
    std::ostringstream output;
    const auto preserved = boot::select_logging_config(
        boot::InitMode::interactive,
        true,
        unread,
        output
    );
    CHECK_FALSE(preserved.cancelled);
    CHECK(preserved.arguments == boot::ConfigArguments::preserve);
    CHECK(output.str().empty());
    CHECK(unread.peek() == 'n');
}

TEST_CASE("EOF cancels logging bootstrap without creating auto_core.ini", "[logging][init]") {
    const auto root =
        std::filesystem::temp_directory_path() /
        "auto-core-logging-bootstrap-cancel";
    std::filesystem::remove_all(root);
    const auto ini = root / "config" / "auto_core.ini";
    std::filesystem::create_directories(ini.parent_path());

    std::istringstream eof {};
    std::ostringstream output;
    const auto cancelled = boot::select_logging_config(
        boot::InitMode::interactive,
        false,
        eof,
        output
    );
    CHECK(cancelled.cancelled);
    CHECK_FALSE(std::filesystem::exists(ini));
    std::filesystem::remove_all(root);
}

TEST_CASE("Disable logging writes before normal logging and adds no log files", "[logging][init]") {
    const auto root =
        std::filesystem::temp_directory_path() /
        "auto-core-logging-bootstrap-disable";
    std::filesystem::remove_all(root);
    const auto logs = root / "logs";
    std::filesystem::create_directories(logs);
    {
        std::ofstream unrelated {logs / "unrelated.txt"};
        unrelated << "keep\n";
    }

    std::istringstream input {"n\n"};
    std::ostringstream output;
    const auto selection = boot::select_logging_config(
        boot::InitMode::interactive,
        false,
        input,
        output
    );
    REQUIRE_FALSE(selection.cancelled);
    CHECK(selection.arguments == boot::ConfigArguments::disable);

    bool file_present = false;
    std::string written;
    int logging_calls = 0;
    const auto outcome = boot::run_logging_config(
        selection.arguments,
        [&](const boot::ConfigArguments arguments) {
            CHECK(logging_calls == 0);
            CHECK(arguments == boot::ConfigArguments::disable);
            written = logging::ini_text(logging::disabled_defaults());
            file_present = true;
            return 0;
        },
        [&] { return file_present; },
        [&] { ++logging_calls; }
    );

    CHECK(outcome.code == 0);
    CHECK(outcome.logging_started);
    CHECK(logging_calls == 1);
    CHECK(written.find(disabled_ini) != std::string::npos);

    int files = 0;
    for (const auto& entry : std::filesystem::directory_iterator {logs}) {
        CHECK(entry.path().filename() == "unrelated.txt");
        ++files;
    }
    CHECK(files == 1);
    std::filesystem::remove_all(root);
}

TEST_CASE("Failed logging launch does not start normal logging", "[logging][init]") {
    int logging_calls = 0;
    const auto outcome = boot::run_logging_config(
        boot::ConfigArguments::disable,
        [](const boot::ConfigArguments) { return 1; },
        [] { return true; },
        [&] { ++logging_calls; }
    );
    CHECK(outcome.code == 1);
    CHECK_FALSE(outcome.logging_started);
    CHECK(logging_calls == 0);
}
