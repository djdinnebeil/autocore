module keymap_runtime;

import std;

import auto_core.ini;
import command_registry;
import ac_modules;
import keymap;
import keymap_hardcoded;
import ac_main;
import auto_core.paths;

import <Windows.h>;

static command_registry::Registry create_runtime_command_registry() {
    command_registry::Registry registry;

    ac_actions::runtime_commands::register_with(registry);
    ac_main::runtime_commands::register_with(registry);
    itunes::runtime_commands::register_with(registry);
    journal::runtime_commands::register_with(registry);
    link::runtime_commands::register_with(registry);
    notepad::runtime_commands::register_with(registry);
    notes::runtime_commands::register_with(registry);
    slash::runtime_commands::register_with(registry);
    sp::runtime_commands::register_with(registry);
    taskbar_runtime_commands::register_with(registry);
    taskbar_11::runtime_commands::register_with(registry);
    tasks::runtime_commands::register_with(registry);

    return registry;
}

static const command_registry::Registry& runtime_command_registry() {
    static const command_registry::Registry registry =
        create_runtime_command_registry();
    return registry;
}

static std::string_view trim(std::string_view value);

template<typename ContentsFactory>
static bool write_new_file(
    const std::filesystem::path& path,
    ContentsFactory contents_factory
) {
    std::error_code ec;

    if (std::filesystem::exists(path, ec)) {
        return true;
    }

    if (ec) {
        auto_core.logg_and_logg(
            "Failed to inspect {}: {}",
            path.string(),
            ec.message()
        );
        return false;
    }

    const std::string contents = contents_factory();
    std::ofstream output(path, std::ios::binary);

    if (!output) {
        auto_core.logg_and_logg(
            "Failed to create {}",
            path.string()
        );
        return false;
    }

    output.write(
        contents.data(),
        static_cast<std::streamsize>(contents.size())
    );
    output.close();

    if (!output) {
        auto_core.logg_and_logg(
            "Failed to write {}",
            path.string()
        );
        return false;
    }

    return true;
}

static bool initialize_keymap_workspace() {
    std::error_code ec;
    std::filesystem::create_directories(
        ac::paths::keymap_directory(),
        ec
    );

    if (ec) {
        auto_core.logg_and_logg(
            "Failed to create keymap directory: {}",
            ec.message()
        );
        return false;
    }

    const bool keymap_ready = write_new_file(
        ac::paths::keymap_file(),
        [] {
            return hardcoded_keymap_contents();
        }
    );

    const bool settings_ready = write_new_file(
        ac::paths::keymap_settings_file(),
        [] {
            return std::string {
                "[keymap]\ntrace_enabled = false\n"
            };
        }
    );

    return keymap_ready && settings_ready;
}

static bool keymap_trace_enabled() {
    const auto document =
        ac::ini::read(ac::paths::keymap_settings_file());
    return document &&
        document->find("keymap", "trace_enabled") == "true";
}

static bool use_compiled_keymap() {
    const auto document = ac::ini::read(
        ac::paths::config_directory() / "keymap_mode.ini"
    );
    const auto mode = document
        ? document->find("keymap", "mode")
        : std::nullopt;
    return mode != "runtime";
}

static std::string autocomplete_contents(
    const command_registry::Registry& registry
) {
    const std::vector<std::string> values =
        registry.autocomplete_values();
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

static bool refresh_keymap_commands(
    const command_registry::Registry& registry
) {
    const std::filesystem::path& destination =
        ac::paths::keymap_commands_file();
    const std::string desired = autocomplete_contents(registry);

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
        auto_core.logg_and_logg(
            "Failed to create temporary keymap command list"
        );
        return false;
    }

    output.write(
        desired.data(),
        static_cast<std::streamsize>(desired.size())
    );
    output.close();

    if (!output) {
        auto_core.logg_and_logg(
            "Failed to write temporary keymap command list"
        );
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
        auto_core.logg_and_logg(
            "Failed to replace keymap command list: {}",
            error
        );
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        return false;
    }

    return true;
}

static int get_numkey_vk_code(std::string_view key_name) {
    static const std::unordered_map<std::string_view, int> key_codes = {
        {"numkey_0", numkey_0},
        {"numkey_1", numkey_1},
        {"numkey_2", numkey_2},
        {"numkey_3", numkey_3},
        {"numkey_4", numkey_4},
        {"numkey_5", numkey_5},
        {"numkey_6", numkey_6},
        {"numkey_7", numkey_7},
        {"numkey_8", numkey_8},
        {"numkey_9", numkey_9},
        {"numkey_star", numkey_star},
        {"numkey_plus", numkey_plus},
        {"numkey_dot", numkey_dot},
        {"numkey_enter", numkey_enter},
        {"numkey_dash", numkey_dash},
        {"numkey_slash", numkey_slash},
        {"play_pause_key", play_pause_key},
        {"calculator_key", calculator_key},
        {"mail_key", mail_key},
        {"home_page_key", home_page_key},
    };

    const auto key = key_codes.find(key_name);
    return key != key_codes.end() ? key->second : -1;
}

std::vector<std::string> get_runtime_command_names() {
    return runtime_command_registry().registered_names();
}

std::vector<std::string> get_runtime_command_autocomplete_values() {
    return runtime_command_registry().autocomplete_values();
}

static std::string_view trim(std::string_view value) {
    const std::size_t first = value.find_first_not_of(" \t");

    if (first == std::string_view::npos) {
        return {};
    }

    const std::size_t last = value.find_last_not_of(" \t");

    return value.substr(first, last - first + 1);
}

static std::size_t find_action_delimiter(
    std::string_view value
) {
    int parenthesis_depth = 0;
    bool inside_quotes = false;

    for (std::size_t index = 0; index < value.size(); ++index) {
        const char character = value[index];

        if (character == '"' &&
            (index == 0 || value[index - 1] != '\\')) {
            inside_quotes = !inside_quotes;
            continue;
        }

        if (inside_quotes) {
            continue;
        }

        if (character == '(') {
            ++parenthesis_depth;
        }
        else if (character == ')') {
            if (parenthesis_depth == 0) {
                return std::string_view::npos;
            }

            --parenthesis_depth;
        }
        else if (character == ',' &&
            parenthesis_depth == 0) {
            return index;
        }
    }

    return std::string_view::npos;
}

/**
 * \brief Parses one keymap.ini entry.
 * \param line The complete keymap configuration line.
 * \param key_string Receives the configured key name.
 * \param primary Receives the primary command expression.
 * \param secondary Receives the secondary command expression.
 * \return true if the line contains a valid key and two actions;
 *         otherwise, false.
 */
static bool parse_line(
    std::string_view line,
    std::string_view& key_string,
    std::string_view& primary,
    std::string_view& secondary
) {
    const std::size_t equals = line.find('=');
    if (equals == std::string_view::npos) {
        return false;
    }

    const std::size_t opening_brace =
        line.find('{', equals);

    const std::size_t closing_brace =
        line.rfind('}');

    if (opening_brace == std::string_view::npos ||
        closing_brace == std::string_view::npos ||
        closing_brace <= opening_brace + 1) {
        return false;
    }

    const std::string_view actions = line.substr(
        opening_brace + 1,
        closing_brace - opening_brace - 1
    );

    const std::size_t delimiter =
        find_action_delimiter(actions);

    if (delimiter == std::string_view::npos) {
        return false;
    }

    key_string = trim(line.substr(0, equals));

    primary = trim(actions.substr(0, delimiter));
    secondary = trim(actions.substr(delimiter + 1));

    return !key_string.empty() &&
        !primary.empty() &&
        !secondary.empty();
}

/**
 * \brief Parses the keymap configuration file and sets the keymap.
 *
 * This function reads keymap.ini, parses each configured key binding,
 * and populates the keymap with primary and secondary actions.
 */
static bool set_keymap_from_stream(
    std::istream& config_file,
    const command_registry::Registry& registry,
    bool trace_enabled
) {
    ac_numkey_event.clear();
    std::string line;
    std::size_t configured_entries = 0;

    if (trace_enabled) {
        auto_core.logg_and_logg("keymap trace logging enabled");
    }

    while (std::getline(config_file, line)) {
        const std::string_view trimmed_line = trim(line);

        if (trimmed_line.empty() ||
            trimmed_line == "[keymap]" ||
            trimmed_line.starts_with(';') ||
            trimmed_line.starts_with('#')) {
            continue;
        }

        std::string_view key_string;
        std::string_view primary;
        std::string_view secondary;

        if (!parse_line(line, key_string, primary, secondary)) {
            auto_core.logg_and_logg(
                "Invalid keymap line format: {}",
                line
            );
            continue;
        }

        const int key = get_numkey_vk_code(key_string);

        if (key == -1) {
            auto_core.logg_and_logg(
                "Unknown keymap key: {}",
                key_string
            );
            continue;
        }

        if (trace_enabled) {
            auto_core.logg_and_logg(
                "creating keymap actions: {} = {}, {}",
                key_string,
                primary,
                secondary
            );
        }

        command_registry::Action primary_action =
            registry.resolve(primary);
        command_registry::Action secondary_action =
            registry.resolve(secondary);

        if (!primary_action || !secondary_action) {
            auto_core.logg_and_logg(
                "Unknown keymap command for {}: {}, {}",
                key_string,
                primary,
                secondary
            );
            continue;
        }

        ac_numkey_event[key] = {
            std::move(primary_action),
            std::move(secondary_action)
        };
        ++configured_entries;
    }

    if (configured_entries == 0) {
        auto_core.logg_and_logg(
            "Runtime keymap contained no usable entries"
        );
        return false;
    }

    auto_core.logg_and_logg("keymap configured from file");
    return true;
}

static bool set_keymap_from_file(
    const command_registry::Registry& registry,
    bool trace_enabled
) {
    std::ifstream config_file(ac::paths::keymap_file());

    if (!config_file) {
        auto_core.logg_and_logg(
            "Failed to open runtime keymap: {}",
            ac::paths::keymap_file().string()
        );
        return false;
    }

    return set_keymap_from_stream(config_file, registry, trace_enabled);
}

void set_keymap_from_file() {
    const command_registry::Registry& registry =
        runtime_command_registry();

    if (!set_keymap_from_file(registry, keymap_trace_enabled())) {
        set_hardcoded_keymap();
    }
}

void initialize_keymap() {
    if (!initialize_keymap_workspace()) {
        auto_core.logg_and_logg(
            "Using compiled keymap because workspace initialization failed"
        );
        set_hardcoded_keymap();
        return;
    }

    if (use_compiled_keymap()) {
        set_hardcoded_keymap();
        return;
    }

    const command_registry::Registry& registry =
        runtime_command_registry();

    refresh_keymap_commands(registry);

    if (!set_keymap_from_file(registry, keymap_trace_enabled())) {
        auto_core.logg_and_logg(
            "Using compiled keymap because runtime loading failed"
        );
        set_hardcoded_keymap();
    }
}
