#include "catch_amalgamated.hpp"
#include "../src/console_route_detail.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

import auto_core.core.clock;
import auto_core.core.component;
import auto_core.core.logging.config;

namespace {

    std::string read_text(const std::filesystem::path& path) {
        std::ifstream input(path);
        return std::string {
            std::istreambuf_iterator<char> {input},
            std::istreambuf_iterator<char> {}
        };
    }

    std::string line_with(
        const std::string_view text,
        const std::string_view needle
    ) {
        std::size_t start = 0;
        while (start <= text.size()) {
            const std::size_t end = text.find('\n', start);
            const std::size_t stop =
                end == std::string_view::npos ? text.size() : end;
            const std::string_view line = text.substr(start, stop - start);
            if (line.find(needle) != std::string_view::npos) {
                return std::string {line};
            }
            if (end == std::string_view::npos) {
                break;
            }
            start = end + 1;
        }
        return {};
    }

    bool main_lines_are_in_log(
        const std::string_view comprehensive,
        const std::string_view main_log
    ) {
        std::size_t start = 0;
        while (start <= main_log.size()) {
            const std::size_t end = main_log.find('\n', start);
            const std::size_t stop =
                end == std::string_view::npos ? main_log.size() : end;
            const std::string_view line =
                main_log.substr(start, stop - start);
            if (!line.empty() &&
                comprehensive.find(line) == std::string_view::npos) {
                return false;
            }
            if (end == std::string_view::npos) {
                break;
            }
            start = end + 1;
        }
        return true;
    }

    std::string capture_stdout(const auto& write) {
        std::ostringstream captured;
        auto* previous = std::cout.rdbuf(captured.rdbuf());
        write();
        std::cout.rdbuf(previous);
        return captured.str();
    }

}

TEST_CASE(
    "Component routes local logs and keeps main log as a subset",
    "[component][logging]"
) {
    const std::string name = "log_route_probe";
    const auto directory =
        ac::logging::config::components_directory() / name;
    std::filesystem::remove_all(directory);

    const std::string console = capture_stdout([&] {
        ac::Component component {name};
        component.log("only-comprehensive");
        component.log_main("both-main");
        component.log_print("both-print");
        component.print("both-console");
        component.lognl("open-only");
        component.log(" tail");
        component.lognl_main("Choice: ");
        component.log_main("2");
    });

    const auto date = ac::clock::get_local_datetime().date_iso;
    const auto comprehensive = read_text(
        directory / (date + "_" + name + ".log")
    );
    const auto main_log = read_text(
        directory / (date + "_" + name + ".main.log")
    );

    CHECK(console == "both-print\nboth-console\n");
    CHECK_FALSE(line_with(comprehensive, "only-comprehensive").empty());
    CHECK(line_with(main_log, "only-comprehensive").empty());
    CHECK_FALSE(line_with(comprehensive, "open-only tail").empty());
    CHECK(line_with(main_log, "open-only").empty());

    const auto choice = line_with(comprehensive, "Choice: 2");
    CHECK(choice == line_with(main_log, "Choice: 2"));
    CHECK_FALSE(choice.empty());
    CHECK(line_with(comprehensive, "both-main") ==
        line_with(main_log, "both-main"));
    CHECK(line_with(comprehensive, "both-print") ==
        line_with(main_log, "both-print"));
    CHECK(line_with(comprehensive, "both-console") ==
        line_with(main_log, "both-console"));

    CHECK_FALSE(line_with(comprehensive, "Session started").empty());
    CHECK(line_with(main_log, "Session started").empty());
    CHECK_FALSE(line_with(comprehensive, "***").empty());
    CHECK(line_with(main_log, "***").empty());
    CHECK(main_lines_are_in_log(comprehensive, main_log));

    const std::string nl_print_console = capture_stdout([&] {
        ac::Component component {name + "_nl_print"};
        component.lognl_print("nl-print");
    });
    const std::string printnl_console = capture_stdout([&] {
        ac::Component component {name + "_printnl"};
        component.printnl("nl-console");
    });

    const auto nl_print_dir =
        ac::logging::config::components_directory() / (name + "_nl_print");
    const auto printnl_dir =
        ac::logging::config::components_directory() / (name + "_printnl");
    const auto nl_print_log = read_text(
        nl_print_dir / (date + "_" + name + "_nl_print.log")
    );
    const auto nl_print_main = read_text(
        nl_print_dir / (date + "_" + name + "_nl_print.main.log")
    );
    const auto printnl_log = read_text(
        printnl_dir / (date + "_" + name + "_printnl.log")
    );
    const auto printnl_main = read_text(
        printnl_dir / (date + "_" + name + "_printnl.main.log")
    );

    CHECK(nl_print_console == "nl-print");
    CHECK(printnl_console == "nl-console");
    CHECK(line_with(nl_print_log, "nl-print") ==
        line_with(nl_print_main, "nl-print"));
    CHECK_FALSE(line_with(nl_print_log, "nl-print").empty());
    CHECK(line_with(printnl_log, "nl-console") ==
        line_with(printnl_main, "nl-console"));
    CHECK_FALSE(line_with(printnl_log, "nl-console").empty());
    CHECK(main_lines_are_in_log(nl_print_log, nl_print_main));
    CHECK(main_lines_are_in_log(printnl_log, printnl_main));

    std::filesystem::remove_all(directory);
    std::filesystem::remove_all(nl_print_dir);
    std::filesystem::remove_all(printnl_dir);
}

TEST_CASE(
    "log and log_main use the routes that consult write_logs_to_console",
    "[component][logging]"
) {
    using ac::OutputRoute;
    using ac::component_detail::console_write_count;

    CHECK(OutputRoute::component != OutputRoute::component_main_and_console);
    CHECK(OutputRoute::component_and_main !=
        OutputRoute::component_main_and_console);
    CHECK(console_write_count(
            OutputRoute::component == OutputRoute::component_main_and_console,
            false
        ) == 0);
    CHECK(console_write_count(
            OutputRoute::component_and_main ==
                OutputRoute::component_main_and_console,
            true
        ) == 1);
    CHECK(console_write_count(
            OutputRoute::component_main_and_console ==
                OutputRoute::component_main_and_console,
            false
        ) == 1);
}
