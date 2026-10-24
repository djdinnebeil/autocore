/**
 * \file program_ready_detail.hpp
 * \brief Pure startup configuration and ready-message helpers.
 */
#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace ac::main::startup {

    struct Settings {
        bool show_day_of_week;
        bool show_task_list;
    };

    enum class ValueStatus {
        valid,
        missing,
        invalid
    };

    struct Resolution {
        Settings settings;
        ValueStatus day_of_week_status;
        ValueStatus task_list_status;
    };

    [[nodiscard]]
    inline Resolution resolve(
        const std::optional<std::string_view> show_day_of_week,
        const std::optional<std::string_view> show_task_list,
        const Settings& defaults
    ) noexcept {
        const auto resolve_value = [](
            const std::optional<std::string_view> value,
            const bool default_value
        ) -> std::pair<bool, ValueStatus> {
            if (!value) {
                return {default_value, ValueStatus::missing};
            }
            if (*value == "on") {
                return {true, ValueStatus::valid};
            }
            if (*value == "off") {
                return {false, ValueStatus::valid};
            }
            return {default_value, ValueStatus::invalid};
        };
        const auto [day_enabled, day_status] =
            resolve_value(show_day_of_week, defaults.show_day_of_week);
        const auto [tasks_enabled, tasks_status] =
            resolve_value(show_task_list, defaults.show_task_list);
        return {
            .settings = {
                .show_day_of_week = day_enabled,
                .show_task_list = tasks_enabled
            },
            .day_of_week_status = day_status,
            .task_list_status = tasks_status
        };
    }

    template<typename DayProvider, typename TaskListProvider>
    [[nodiscard]]
    std::string assemble_message(
        const Settings& settings,
        DayProvider&& day_provider,
        TaskListProvider&& task_list_provider
    ) {
        std::string message = "Program ready";
        if (settings.show_day_of_week) {
            message += "\nToday is ";
            message += std::invoke(std::forward<DayProvider>(day_provider));
        }
        if (settings.show_task_list) {
            const std::string task_list = std::invoke(
                std::forward<TaskListProvider>(task_list_provider)
            );
            if (!task_list.empty()) {
                message += '\n';
                message += task_list;
            }
        }
        return message;
    }

} // namespace ac::main::startup
