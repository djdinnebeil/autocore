module;

#include "keymap_map_detail.hpp"
#include "../taskbar/activate_factory.hpp"
#include "../../shared/provider_display_name.hpp"
#include "../../../core/taskbar/activation_catalog_description.hpp"

module auto_core.main.keymap.runtime;

import std;

import auto_core.core.config;
import auto_core.core.paths;
import auto_core.main.components;
import command_registry;
import auto_core.main.key_codes;
import auto_core.main.keymap;
import auto_core.main.application;
import auto_core.main.taskbar;
import auto_core.main.test_commands;
import taskbar_firefox;

import <Windows.h>;

static bool is_activation_command(const std::string_view command) {
    constexpr std::string_view prefix = "activate_";
    return command.starts_with(prefix) && command.size() > prefix.size();
}

static void add_taskbar_offer(
    std::vector<ac::taskbar::ActivationOffer>& shared_offers,
    std::vector<std::string>& occupied,
    std::string command,
    std::string description,
    std::string application
) {
    occupied.push_back(command);
    shared_offers.push_back(ac::taskbar::ActivationOffer {
        "taskbar",
        std::move(command),
        std::move(description),
        std::move(application)
    });
}

static command_registry::Action provider_action(
    const ac::taskbar::ActivationAlias& alias
) {
    if (alias.provider == "auto_core" &&
        alias.command == "activate_auto_core") {
        return [] {
            ac::main::components::invoke("taskbar", "activate_auto_core");
        };
    }
    if (alias.provider == "taskbar" && !alias.application.empty()) {
        return taskbar_activation_action(alias.application, alias.command);
    }
    return ac::main::components::activation_action(
        alias.provider,
        alias.command
    );
}

struct UnselectedSharedCommand {
    std::string command;
    std::vector<std::string> aliases;
};

static std::vector<UnselectedSharedCommand>& unselected_shared_commands() {
    static std::vector<UnselectedSharedCommand> commands;
    return commands;
}

struct UnsetCatalogRow {
    std::string command;
    std::string description;
};

static std::vector<UnsetCatalogRow>& unset_catalog_rows() {
    static std::vector<UnsetCatalogRow> rows;
    return rows;
}

static void remember_unset_catalog(
    const ac::taskbar::ActivationResolution& resolution
) {
    auto& rows = unset_catalog_rows();
    for (const ac::taskbar::ActivationBinding& binding : resolution.bindings) {
        if (!binding.explicit_blank || binding.offer_count < 2) {
            continue;
        }
        rows.push_back(UnsetCatalogRow {
            binding.command,
            binding.unset_description
        });
    }
}

static std::unordered_set<std::string>& reported_unselected_shared() {
    static std::unordered_set<std::string> reported;
    return reported;
}

struct UnresolvedActivation {
    bool blank {false};
    std::string provider;
};

static std::unordered_map<std::string, UnresolvedActivation>&
unresolved_activations() {
    static std::unordered_map<std::string, UnresolvedActivation> commands;
    return commands;
}

static void remember_unresolved_activations(
    const ac::taskbar::ActivationResolution& resolution,
    const command_registry::Registry& registry
) {
    auto& commands = unresolved_activations();
    commands.clear();
    for (const ac::taskbar::ActivationBinding& binding : resolution.bindings) {
        if (registry.contains(binding.command)) {
            continue;
        }
        commands.insert_or_assign(
            binding.command,
            UnresolvedActivation {
                binding.blank_selection,
                binding.blank_selection ? std::string {} : binding.provider
            }
        );
    }
}

static void remember_unselected_shared(
    const ac::taskbar::ActivationResolution& resolution
) {
    auto& commands = unselected_shared_commands();
    commands.clear();
    for (const ac::taskbar::ActivationBinding& binding : resolution.bindings) {
        if (!binding.blank_selection) {
            continue;
        }
        UnselectedSharedCommand entry;
        entry.command = binding.command;
        entry.aliases.reserve(binding.aliases.size());
        for (const ac::taskbar::ActivationAlias& alias : binding.aliases) {
            entry.aliases.push_back(alias.name);
        }
        commands.push_back(std::move(entry));
    }
}

static void register_resolved(
    command_registry::Registry& registry,
    const ac::taskbar::ActivationResolution& resolution,
    const std::string_view alias_label,
    const std::string_view command_label,
    const std::string_view map_name
) {
    for (const std::string& report : resolution.reports) {
        auto_core.log_alert("{}", report);
    }

    for (const ac::taskbar::ActivationBinding& binding : resolution.bindings) {
        for (const ac::taskbar::ActivationAlias& alias : binding.aliases) {
            if (registry.contains(alias.name)) {
                auto_core.log_alert(
                    "{} '{}' was not registered because that command already "
                    "exists.",
                    alias_label,
                    alias.name
                );
                continue;
            }
            command_registry::Action action = provider_action(alias);
            if (!action) {
                auto_core.log_alert(
                    "{} '{}' was not registered because {} is unavailable.",
                    alias_label,
                    alias.name,
                    alias.provider
                );
                continue;
            }
            registry.add(
                alias.name,
                std::move(action),
                alias.description,
                alias.provider
            );
        }

        if (!binding.resolved || registry.contains(binding.command)) {
            if (binding.resolved && registry.contains(binding.command)) {
                auto_core.log_alert(
                    "{} '{}' was not registered because that command already "
                    "exists.",
                    command_label,
                    binding.command
                );
            }
            continue;
        }
        const ac::taskbar::ActivationAlias selected {
            binding.command,
            binding.provider,
            binding.command,
            binding.description,
            binding.application
        };
        command_registry::Action action = provider_action(selected);
        if (!action) {
            auto_core.log_alert(
                "{} selects '{}' for {}, but that provider is unavailable.",
                map_name,
                binding.provider,
                binding.command
            );
            continue;
        }
        registry.add(
            binding.command,
            std::move(action),
            binding.description,
            binding.provider
        );
    }
}

static command_registry::Registry create_runtime_command_registry() {
    command_registry::Registry registry;
    unset_catalog_rows().clear();

    std::vector<ac::taskbar::ActivationOffer> offers;
    std::vector<ac::taskbar::ActivationOffer> shared_offers;
    std::vector<std::string> occupied;
    occupied.emplace_back("activate_firefox_or_open_reddit");
    occupied.emplace_back(ac::main::activate_factory_name);

    for (const ac::taskbar::ConfiguredActivationBinding& binding :
         ac::taskbar::configured_activation_bindings_from_disk()) {
        add_taskbar_offer(
            shared_offers,
            occupied,
            binding.command,
            binding.description,
            binding.application
        );
    }
    occupied.emplace_back("activate_auto_core");
    offers.push_back(ac::taskbar::ActivationOffer {
        "auto_core",
        "activate_auto_core",
        ac::taskbar::activation_catalog_description(
            "activate_auto_core",
            "auto_core",
            {}
        ),
        {}
    });
    for (const ac::main::components::AdvertisedCommand& command :
         ac::main::components::advertised_commands()) {
        occupied.push_back(command.name);
        ac::taskbar::ActivationOffer offer {
            command.provider,
            command.name,
            command.description,
            command.application
        };
        if (is_activation_command(command.name)) {
            offers.push_back(std::move(offer));
        }
        else {
            shared_offers.push_back(std::move(offer));
        }
    }

    ac::taskbar::ActivationResolution resolution =
        ac::taskbar::resolve_activation_commands(
            ac::paths::activation_map_file(),
            offers,
            occupied
        );
    ac::taskbar::ActivationResolution shared =
        ac::taskbar::resolve_shared_commands(
            ac::paths::shared_commands_map_file(),
            shared_offers,
            occupied
        );
    ac::taskbar::reconcile_provider_aliases(resolution, shared, occupied);
    remember_unselected_shared(shared);
    remember_unset_catalog(resolution);
    remember_unset_catalog(shared);
    std::unordered_set<std::string> shared_names;
    for (const ac::taskbar::ActivationBinding& binding : resolution.bindings) {
        shared_names.insert(binding.command);
    }
    for (const ac::taskbar::ActivationBinding& binding : shared.bindings) {
        shared_names.insert(binding.command);
    }

    test_commands::runtime_commands::register_with(registry);
    main_component::runtime_commands::register_with(registry);
    taskbar_runtime_commands::register_configured_with(registry, shared_names);
    ac::main::components::register_with(registry, shared_names);
    register_resolved(
        registry,
        resolution,
        "activation alias",
        "shared activation command",
        "activate_commands.map"
    );
    remember_unresolved_activations(resolution, registry);
    register_resolved(
        registry,
        shared,
        "shared command alias",
        "shared command",
        "shared_commands.map"
    );
    taskbar_runtime_commands::register_with(registry);
    registry.add(
        "activate_firefox_or_open_reddit",
        &activate_firefox_or_open_reddit,
        "Opens Reddit when Firefox is in front; otherwise activates Firefox.",
        "taskbar"
    );

    return registry;
}

static const command_registry::Registry& runtime_command_registry() {
    static const command_registry::Registry registry =
        create_runtime_command_registry();
    return registry;
}

struct KeymapSettings {
    bool unmapped_key = false;
};

static KeymapSettings keymap_settings() {
    return { ac::config::core_settings().unmapped_key };
}

static std::string catalog_contents(
    const command_registry::Registry& registry
) {
    std::map<std::string, std::string> lines;
    for (const command_registry::CatalogEntry& entry : registry.catalog_entries()) {
        lines.emplace(
            entry.name,
            command_registry::format_merged_catalog_line(
                entry.name,
                ac::main::provider_display_name(entry.provider),
                entry.description
            )
        );
    }
    for (const UnsetCatalogRow& row : unset_catalog_rows()) {
        if (lines.contains(row.command)) {
            continue;
        }
        lines.emplace(
            row.command,
            command_registry::format_merged_catalog_line(
                row.command,
                "Unset",
                row.description
            )
        );
    }

    std::size_t required_size = lines.size();
    for (const auto& [name, line] : lines) {
        (void)name;
        required_size += line.size();
    }

    std::string contents;
    contents.reserve(required_size);
    for (const auto& [name, line] : lines) {
        (void)name;
        contents += line;
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

    const auto directory = destination.parent_path();
    const auto filename = destination.filename().wstring();
    const DWORD pid = GetCurrentProcessId();
    HANDLE handle = INVALID_HANDLE_VALUE;
    std::filesystem::path temporary;
    for (unsigned attempt = 0; attempt < 16; ++attempt) {
        temporary =
            directory /
            (filename + L"." + std::to_wstring(pid) + L"." +
                std::to_wstring(attempt) + L".tmp");
        handle = CreateFileW(
            temporary.c_str(),
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );
        if (handle != INVALID_HANDLE_VALUE) {
            break;
        }
        const DWORD error = GetLastError();
        if (error != ERROR_FILE_EXISTS) {
            auto_core.log_info(
                "{}: {} (error {})",
                failed_create,
                temporary.string(),
                error
            );
            return false;
        }
    }
    if (handle == INVALID_HANDLE_VALUE) {
        auto_core.log_info(
            "{}: {}",
            failed_create,
            destination.string()
        );
        return false;
    }

    auto remove_temporary = [&] {
        if (handle != INVALID_HANDLE_VALUE) {
            CloseHandle(handle);
            handle = INVALID_HANDLE_VALUE;
        }
        DeleteFileW(temporary.c_str());
    };

    std::size_t written_total = 0;
    while (written_total < desired.size()) {
        const auto chunk = static_cast<DWORD>(std::min<std::size_t>(
            desired.size() - written_total,
            static_cast<std::size_t>(1u << 20)
        ));
        DWORD written = 0;
        if (!WriteFile(
                handle,
                desired.data() + written_total,
                chunk,
                &written,
                nullptr
            ) ||
            written != chunk) {
            const DWORD error = GetLastError();
            auto_core.log_info(
                "{}: {} (error {})",
                failed_write,
                temporary.string(),
                error == 0 ? ERROR_WRITE_FAULT : error
            );
            remove_temporary();
            return false;
        }
        written_total += written;
    }
    if (!FlushFileBuffers(handle)) {
        const DWORD error = GetLastError();
        auto_core.log_info(
            "{}: {} (error {})",
            failed_write,
            temporary.string(),
            error == 0 ? ERROR_WRITE_FAULT : error
        );
        remove_temporary();
        return false;
    }
    if (!CloseHandle(handle)) {
        handle = INVALID_HANDLE_VALUE;
        auto_core.log_info(
            "{}: {} (error {})",
            failed_write,
            temporary.string(),
            GetLastError()
        );
        remove_temporary();
        return false;
    }
    handle = INVALID_HANDLE_VALUE;

    if (!MoveFileExW(
        temporary.c_str(),
        destination.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
    )) {
        const DWORD error = GetLastError();
        auto_core.log_info("{}: {}", failed_replace, error);
        DeleteFileW(temporary.c_str());
        return false;
    }

    return true;
}

static bool refresh_keymap_commands(
    const command_registry::Registry& registry
) {
    const auto destination = ac::paths::keymap_commands_file();
    std::error_code directory_error;
    std::filesystem::create_directories(
        destination.parent_path(),
        directory_error
    );
    if (directory_error) {
        auto_core.log_info(
            "Failed to use {}: {}",
            destination.parent_path().string(),
            directory_error.message()
        );
        return false;
    }

    return replace_text_file(
        destination,
        catalog_contents(registry),
        "Failed to create temporary keymap command list",
        "Failed to write temporary keymap command list",
        "Failed to replace keymap command list"
    );
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
    auto_core.log_alert(
        "{} is set to an incorrect value: {}",
        display_name,
        expression
    );
}

static command_registry::Action no_op_action() {
    return [] {};
}

static std::string_view command_token(std::string_view expression) {
    const auto arguments = expression.find('(');
    if (arguments != std::string_view::npos) {
        expression = expression.substr(0, arguments);
    }
    while (!expression.empty() &&
           std::isspace(static_cast<unsigned char>(expression.front()))) {
        expression.remove_prefix(1);
    }
    while (!expression.empty() &&
           std::isspace(static_cast<unsigned char>(expression.back()))) {
        expression.remove_suffix(1);
    }
    return expression;
}

static void report_unselected_shared(const UnselectedSharedCommand& entry) {
    if (!reported_unselected_shared().insert(entry.command).second) {
        return;
    }
    std::string aliases;
    if (entry.aliases.empty()) {
        aliases = "none";
    }
    else {
        for (std::size_t index = 0; index < entry.aliases.size(); ++index) {
            if (index != 0) {
                aliases += ", ";
            }
            aliases += entry.aliases[index];
        }
    }
    auto_core.log_alert(
        "shared_commands.map has no provider selected for {}. "
        "Available aliases: {}.",
        entry.command,
        aliases
    );
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

static void report_blank_activation(const std::string_view command) {
    auto_core.log_alert(
        "No provider selected for {}. Set a provider in "
        "keymap\\activate_commands.map, then restart Auto Core.",
        command
    );
}

static void report_unavailable_activation(
    const std::string_view command,
    const std::string_view provider
) {
    auto_core.log_alert(
        "The provider \"{}\" selected for {} is unavailable. "
        "Check the provider choices in keymap\\activate_commands.map, "
        "then restart Auto Core.",
        provider,
        command
    );
}

static command_registry::Action blank_activation_action(std::string command) {
    return [command = std::move(command)] {
        report_blank_activation(command);
    };
}

static command_registry::Action unavailable_activation_action(
    std::string command,
    std::string provider
) {
    return [command = std::move(command), provider = std::move(provider)] {
        report_unavailable_activation(command, provider);
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

    const std::string_view token = command_token(expression);
    for (const UnselectedSharedCommand& entry : unselected_shared_commands()) {
        if (entry.command != token) {
            continue;
        }
        report_unselected_shared(entry);
        return { no_op_action(), false };
    }

    const auto& activations = unresolved_activations();
    const auto activation = activations.find(std::string {token});
    if (activation != activations.end()) {
        command_registry::Action action = activation->second.blank
            ? blank_activation_action(activation->first)
            : unavailable_activation_action(
                activation->first,
                activation->second.provider
            );
        return { std::move(action), false };
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

    bool in_commands = false;
    while (std::getline(config_file, line)) {
        const auto trimmed = ac::keymap::map_file::trim(line);
        if (ac::keymap::map_file::is_section_header(trimmed)) {
            in_commands = ac::keymap::map_file::is_commands_header(trimmed);
            continue;
        }
        if (in_commands || ac::keymap::map_file::is_ignored_line(trimmed)) {
            continue;
        }

        std::string_view key_string;
        std::string_view primary;
        std::string_view secondary;

        if (!parse_line(line, key_string, primary, secondary)) {
            auto_core.log_alert(
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
            auto_core.log_alert(
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
        auto_core.log_alert(
            "Runtime keymap contained no usable entries"
        );
        return false;
    }

    if (settings.unmapped_key) {
        for (const auto& key : key_codes::keys) {
            if (!active_keymap.contains(key.code)) {
                auto_core.log_alert(
                    "{} hasn't been set",
                    display_key_name(key.name)
                );
            }
        }
    }

    auto_core.log_info("keymap configured from file");
    return true;
}

static bool set_keymap_from_file(
    const command_registry::Registry& registry,
    const KeymapSettings& settings
) {
    std::ifstream config_file(ac::paths::keymap_file());

    if (!config_file) {
        auto_core.log_alert(
            "{} is missing. Run auto_core_keymap.exe --seed to generate it. "
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
        [] {
            ac::main::components::invoke("taskbar", "activate_auto_core");
        },
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
    const command_registry::Registry& registry =
        runtime_command_registry();
    refresh_keymap_commands(registry);

    if (!set_keymap_from_file(registry, keymap_settings())) {
        auto_core.log_alert(
            "Using the emergency keymap because keymap.map could not be "
            "loaded. Run auto_core_keymap.exe --seed if keymap.map is missing. "
            "The file will not be created."
        );
        set_emergency_keymap();
    }
}
