/**
 * \file component_star.ixx
 * \brief Shared enablement for `<name>_star.exe`.
 *
 * `get_state`, `set_state`, and `toggle_state` are the reusable operations.
 * `run` is the initial two-option menu. Later component menus call the
 * operations directly and do not parse `components.list` or launch
 * `components_editor.exe` themselves.
 */
module;

#include "../core/src/components_list_detail.hpp"

export module component_star;

import std;
import auto_core.core.component;
import auto_core.core.paths;
import components_editor_request;

import <iostream>;

export namespace ac::component_star {

    namespace list = ac::config::components_list;
    namespace request = ac::config::components_request;

    struct ComponentState {
        list::ListedEnablement enablement {list::ListedEnablement::unavailable};
        std::string error;
    };

    [[nodiscard]]
    inline ComponentState get_state(const std::string_view name) {
        ComponentState state;
        if (!request::is_valid_component_name(name)) {
            state.error = "Invalid component name.";
            return state;
        }

        const auto parsed = list::load(ac::paths::components_list_file());
        state.enablement = list::listed_enablement(parsed, name);
        if (state.enablement == list::ListedEnablement::unavailable) {
            state.error = parsed.error.empty()
                ? "Unable to read components.list. Component state was not determined."
                : parsed.error;
        }
        return state;
    }

    [[nodiscard]]
    inline int set_state(const std::string_view name, const bool enabled) {
        return request::run_component_update(
            name,
            enabled ? request::ListedState::on : request::ListedState::off
        );
    }

    [[nodiscard]]
    inline int toggle_state(const std::string_view name) {
        const auto state = get_state(name);
        if (state.enablement == list::ListedEnablement::unavailable) {
            return 1;
        }
        return set_state(
            name,
            state.enablement != list::ListedEnablement::enabled
        );
    }

    [[nodiscard]]
    inline int run(const std::string_view name) {
        if (!request::is_valid_component_name(name)) {
            std::cerr << "Invalid component name: " << name << '\n';
            return 1;
        }

        ac::Component star {std::string {name} + "_star"};

        while (true) {
            const auto state = get_state(name);
            if (state.enablement == list::ListedEnablement::unavailable) {
                star.log_print(std::string_view {state.error});
                return 1;
            }

            const bool enabled =
                state.enablement == list::ListedEnablement::enabled;
            star.log_print(
                "1. {} component",
                enabled ? "Disable" : "Enable"
            );
            star.log_print("2. Exit");
            star.lognl_print("Choice: ");

            std::string line;
            if (!std::getline(std::cin, line)) {
                star.log("Input ended.");
                return 1;
            }
            star.log(std::string_view {line});

            const auto choice = list::trim_list_line(line);
            if (choice == "1") {
                const int exit_code = toggle_state(name);
                if (exit_code != 0) {
                    star.log_print(
                        "Failed to update components.list ({}).",
                        exit_code
                    );
                }
            }
            else if (choice == "2") {
                return 0;
            }
            else {
                star.log_print("Enter 1 or 2.");
            }
        }
    }

} // namespace ac::component_star
