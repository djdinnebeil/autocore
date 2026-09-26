module;

#include "keymap_map_detail.hpp"

module auto_core.main.keymap.runtime;

import std;

import auto_core.core.ini;
import auto_core.core.paths;
import auto_core.main.components;
import auto_core.main.components.dash;
import auto_core.main.components.slash;
import auto_core.main.components.taskbar;
import command_registry;
import auto_core.main.key_codes;
import auto_core.main.keymap;
import auto_core.main.application;
import auto_core.main.taskbar;
import auto_core.main.test_commands;

import <Windows.h>;

static command_registry::Registry create_runtime_command_registry() {
    command_registry::Registry registry;

    test_commands::runtime_commands::register_with(registry);
    main_component::runtime_commands::register_with(registry);
    if (ac::main::components::special_enabled("dash")) {
        dash::runtime_commands::register_with(registry);
    }
    if (ac::main::components::special_enabled("slash")) {
        slash_component::runtime_commands::register_with(registry);
    }
    taskbar_component::runtime_commands::register_with(registry);
    ac::main::components::register_with(registry);
    // Configuration-defined taskbar commands are registered last so a
    // manual INI file can never override a reserved Main command.
    taskbar_runtime_commands::register_with(registry);

    return registry;
}

static const command_registry::Registry& runtime_command_registry() {
    static const command_registry::Registry registry =
        create_runtime_command_registry();
    return registry;
}

static bool initialize_keymap_workspace() {
    std::error_code ec;
    std::filesystem::create_directories(
        ac::paths::keymap_components_directory(),
        ec
    );

    if (ec) {
        auto_core.log_print(
            "Failed to use keymap/components: {}. Run keymap_config.exe. "
            "Using the emergency keymap; keymap.map will not be created.",
            ec.message()
        );
        return false;
    }

    return true;
}

struct KeymapSettings {
    bool silence_nonset_warning = false;
};

static KeymapSettings keymap_settings() {
    const auto path = ac::paths::keymap_settings_file();
    const auto document = ac::ini::read(path);
    if (!document) {
        std::error_code exists_error;
        const bool present = std::filesystem::exists(path, exists_error);
        if (present && !exists_error) {
            auto_core.log_print(
                "config/keymap.ini is malformed. Run keymap_config.exe to "
                "generate a valid file. Using built-in defaults; the file "
                "will not be created."
            );
        }
        else {
            auto_core.log_print(
                "config/keymap.ini is missing. Run keymap_config.exe to "
                "generate it. Using built-in defaults; the file will not "
                "be created."
            );
        }
        return {};
    }

    const auto silence = document->find("keymap", "silence_nonset_warning");
    if (!silence) {
        auto_core.log_print(
            "config/keymap.ini is missing an expected [keymap] key. Run "
            "keymap_config.exe to repair it. Using built-in defaults for "
            "missing keys; the file will not be rewritten."
        );
    }
    else if (*silence != "true" && *silence != "false") {
        auto_core.log_print(
            "config/keymap.ini contains an invalid value. Run "
            "keymap_config.exe to repair it. Using built-in defaults for "
            "invalid keys; the file will not be rewritten."
        );
    }

    return { silence == "true" };
}

static std::string autocomplete_contents(
    const command_registry::Registry& registry
) {
    std::vector<std::string> values = registry.autocomplete_values();
    std::ranges::sort(values);

    std::size_t required_size = values.size();

    for (const std::string& value : values) {
        required_size += value.size();
    }

    std::string contents;
    contents.reserve(required_size);

    for (const std::string& value : values) {
        contents += value;
        contents += '\n';
    }

    return contents;
}

static bool replace_text_file(
    const std::filesystem::path& destination,
    const std::string& desired,
    std::string_view failed_create,
    std::string_view failed_write,
    std::string_view failed_replace
) {
    {
        std::ifstream existing_file(destination, std::ios::binary);

        if (existing_file) {
            const std::string existing {
                std::istreambuf_iterator<char> {existing_file},
                std::istreambuf_iterator<char> {}
            };

            if (existing == desired) {
                return true;
            }
        }
    }

    std::filesystem::path temporary = destination;
    temporary += ".tmp";
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);

    if (!output) {
        auto_core.log_main("{}", failed_create);
        return false;
    }

    output.write(
        desired.data(),
        static_cast<std::streamsize>(desired.size())
    );
    output.close();

    if (!output) {
        auto_core.log_main("{}", failed_write);
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        return false;
    }

    if (!MoveFileExW(
        temporary.c_str(),
        destination.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
    )) {
        const DWORD error = GetLastError();
        auto_core.log_main("{}: {}", failed_replace, error);
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        return false;
    }

    return true;
}

static bool refresh_keymap_commands(
    const command_registry::Registry& registry
) {
    return replace_text_file(
        ac::paths::keymap_commands_file(),
        autocomplete_contents(registry),
        "Failed to create temporary keymap command list",
        "Failed to write temporary keymap command list",
        "Failed to replace keymap command list"
    );
}

std::vector<std::string> get_runtime_command_names() {
    return runtime_command_registry().registered_names();
}

std::vector<std::string> get_runtime_command_autocomplete_values() {
    return runtime_command_registry().autocomplete_values();
}

static bool parse_line(
    std::string_view line,
    std::string_view& key_string,
    std::string_view& primary,
    std::string_view& secondary
) {
    const auto mapping = ac::keymap::map_file::parse_line(line);
    if (!mapping) {
        return false;
    }
    key_string = mapping->key;
    primary = mapping->primary;
    secondary = mapping->secondary;
    return true;
}

static std::string display_key_name(std::string_view key_name) {
    std::string display {key_name};
    std::ranges::replace(display, '_', ' ');
    return display;
}

static void report_incorrect_value(
    std::string_view display_name,
    std::string_view expression
) {
    auto_core.log_print(
        "{} is set to an incorrect value: {}",
        display_name,
        expression
    );
}

static command_registry::Action no_op_action() {
    return [] {};
}

static command_registry::Action incorrect_value_action(
    std::string display_name,
    std::string expression
) {
    report_incorrect_value(display_name, expression);
    return [display_name = std::move(display_name),
            expression = std::move(expression)] {
        report_incorrect_value(display_name, expression);
    };
}

struct SideBind {
    command_registry::Action action;
    bool resolved = false;
};

static SideBind bind_side(
    std::string_view expression,
    const command_registry::Registry& registry,
    std::string_view display_name
) {
    if (expression.empty()) {
        return { no_op_action(), false };
    }

    command_registry::Action action = registry.resolve(expression);
    if (action) {
        return { std::move(action), true };
    }

    return {
        incorrect_value_action(
            std::string {display_name},
            std::string {expression}
        ),
        false
    };
}

/**
 * \brief Parses the keymap configuration file and sets the keymap.
 *
 * This function reads keymap.map, parses each configured key binding,
 * and populates the keymap with primary and secondary actions.
 */
static bool set_keymap_from_stream(
    std::istream& config_file,
    const command_registry::Registry& registry,
    const KeymapSettings& settings
) {
    active_keymap.clear();
    std::string line;
    std::size_t configured_entries = 0;

    while (std::getline(config_file, line)) {
        if (ac::keymap::map_file::is_ignored_line(
                ac::keymap::map_file::trim(line))) {
            continue;
        }

        std::string_view key_string;
        std::string_view primary;
        std::string_view secondary;

        if (!parse_line(line, key_string, primary, secondary)) {
            auto_core.log_print(
                "Invalid keymap line format: {}",
                line
            );
            continue;
        }

        if (primary.empty() && secondary.empty()) {
            continue;
        }

        const std::optional<key_codes::Code> key =
            key_codes::resolve(key_string);

        if (!key) {
            auto_core.log_print(
                "Unknown keymap key: {}",
                key_string
            );
            continue;
        }

        const std::string display_name = display_key_name(key_string);
        SideBind primary_bind =
            bind_side(primary, registry, display_name);
        SideBind secondary_bind =
            bind_side(secondary, registry, display_name);

        active_keymap[*key] = {
            std::move(primary_bind.action),
            std::move(secondary_bind.action)
        };

        if (primary_bind.resolved || secondary_bind.resolved) {
            ++configured_entries;
        }
    }

    if (configured_entries == 0) {
        auto_core.log_print(
            "Runtime keymap contained no usable entries"
        );
        return false;
    }

    if (!settings.silence_nonset_warning) {
        for (const auto& key : key_codes::keys) {
            if (!active_keymap.contains(key.code)) {
                auto_core.log_print(
                    "{} hasn't been set",
                    display_key_name(key.name)
                );
            }
        }
    }

    auto_core.log_main("keymap configured from file");
    return true;
}

static bool set_keymap_from_file(
    const command_registry::Registry& registry,
    const KeymapSettings& settings
) {
    std::ifstream config_file(ac::paths::keymap_file());

    if (!config_file) {
        auto_core.log_print(
            "{} is missing. Run keymap_editor.exe to generate it. "
            "Using the emergency keymap; the file will not be created.",
            ac::paths::keymap_file().string()
        );
        return false;
    }

    return set_keymap_from_stream(config_file, registry, settings);
}

static void set_emergency_keymap() {
    active_keymap.clear();
    active_keymap[key_codes::numpad_0] = {
        activate_function_key,
        deactivate_function_key
    };
    active_keymap[key_codes::numpad_1] = {
        taskbar_pipe_command(
            ac::protocol::taskbar::commands::activate_auto_core
        ),
        close_program
    };
}

void set_keymap_from_file() {
    const command_registry::Registry& registry =
        runtime_command_registry();

    if (!set_keymap_from_file(registry, keymap_settings())) {
        set_emergency_keymap();
    }
}

void initialize_keymap() {
    if (!initialize_keymap_workspace()) {
        set_emergency_keymap();
        return;
    }

    const command_registry::Registry& registry =
        runtime_command_registry();
    refresh_keymap_commands(registry);

    if (!set_keymap_from_file(registry, keymap_settings())) {
        auto_core.log_print(
            "Using the emergency keymap because keymap.map could not be "
            "loaded. Run keymap_editor.exe if keymap.map is missing. "
            "The file will not be created."
        );
        set_emergency_keymap();
    }
}
