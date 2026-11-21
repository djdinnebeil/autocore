/**
 * \file activation_map.cxx
 * \brief Selects the provider for a shared keymap command.
 *
 * auto_core_keymap.exe --build is the only writer. Startup reads the files.
 * `activate_commands.map` keeps `activate_<program>`. `shared_commands.map`
 * keeps every other duplicated command.
 */
module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cctype>

module auto_core.taskbar;

import std;

namespace {

constexpr std::string_view activation_prefix = "activate_";
constexpr std::string_view choice_prefix = "# Choice:";
constexpr std::string_view choices_prefix = "# Choices:";

struct ProviderOffer {
    std::string provider;
    std::string description;
    std::string application;
};

struct TextLine {
    std::string text;
};

struct MapText {
    bool ok {true};
    bool missing {false};
    std::string text;
    std::string error;
};

struct MapPolicy {
    std::string_view section;
    std::string_view file_label;
    std::string_view alias_kind;
    std::span<const std::string_view> preamble {};
    bool insert_missing_preamble {false};
    bool blank_line_after_section {false};
    bool report_unselected {true};
    /** Drop a saved command when this discovery contains no provider for it. */
    bool prune_unoffered {false};
    bool (*accepts)(std::string_view command) {nullptr};
    std::optional<std::string> (*alias_for)(
        std::string_view provider,
        std::string_view command
    ) {nullptr};
    std::string (*initial_value)(const std::vector<ProviderOffer>& providers) {
        nullptr
    };
};

[[nodiscard]]
bool is_activation_command(const std::string_view command) {
    return command.starts_with(activation_prefix) &&
        command.size() > activation_prefix.size();
}

[[nodiscard]]
bool is_ordinary_command(const std::string_view command) {
    return !command.empty() && !is_activation_command(command);
}

[[nodiscard]]
std::string_view trim(std::string_view text) {
    while (!text.empty() &&
           std::isspace(static_cast<unsigned char>(text.front()))) {
        text.remove_prefix(1);
    }
    while (!text.empty() &&
           std::isspace(static_cast<unsigned char>(text.back()))) {
        text.remove_suffix(1);
    }
    return text;
}

[[nodiscard]]
bool is_comment(const std::string_view text) {
    const auto value = trim(text);
    return value.starts_with('#') || value.starts_with(';');
}

[[nodiscard]]
bool is_generated_choice_comment(const std::string_view text) {
    const auto value = trim(text);
    return value.starts_with(choice_prefix) || value.starts_with(choices_prefix);
}

[[nodiscard]]
bool is_section_header(const std::string_view text) {
    const auto value = trim(text);
    return value.size() >= 2 && value.front() == '[' && value.back() == ']';
}

[[nodiscard]]
bool is_named_section(
    const std::string_view text,
    const std::string_view section
) {
    const auto value = trim(text);
    if (!is_section_header(value)) {
        return false;
    }
    return trim(value.substr(1, value.size() - 2)) == section;
}

[[nodiscard]]
std::optional<std::pair<std::string, std::string>> assignment_of(
    const std::string_view text
) {
    if (is_comment(text) || is_section_header(text)) {
        return std::nullopt;
    }
    const auto value = trim(text);
    const auto equals = value.find('=');
    if (equals == std::string_view::npos) {
        return std::nullopt;
    }
    const auto command = trim(value.substr(0, equals));
    if (command.empty()) {
        return std::nullopt;
    }
    return std::pair {
        std::string {command},
        std::string {trim(value.substr(equals + 1))}
    };
}

[[nodiscard]]
std::vector<TextLine> split_lines(const std::string_view text) {
    std::vector<TextLine> lines;
    std::size_t index = 0;
    while (index < text.size()) {
        const auto newline = text.find('\n', index);
        if (newline == std::string_view::npos) {
            lines.push_back(TextLine {
                std::string {text.substr(index)}
            });
            break;
        }
        auto end = newline;
        if (end > index && text[end - 1] == '\r') {
            --end;
        }
        lines.push_back(TextLine {std::string {text.substr(index, end - index)}});
        index = newline + 1;
    }
    return lines;
}

[[nodiscard]]
std::string join_lines(
    const std::vector<std::string>& lines,
    const std::string_view newline,
    const bool trailing_newline
) {
    std::string text;
    for (std::size_t index = 0; index < lines.size(); ++index) {
        text += lines[index];
        if (index + 1 < lines.size() || trailing_newline) {
            text += newline;
        }
    }
    return text;
}

[[nodiscard]]
std::string choices_comment(const std::vector<ProviderOffer>& providers) {
    std::vector<std::string> ids;
    ids.reserve(providers.size());
    for (const ProviderOffer& provider : providers) {
        ids.push_back(provider.provider);
    }
    std::ranges::sort(ids);
    const std::string_view prefix =
        ids.size() == 1 ? choice_prefix : choices_prefix;
    std::string line {prefix};
    if (!ids.empty()) {
        line += ' ';
    }
    for (std::size_t index = 0; index < ids.size(); ++index) {
        if (index != 0) {
            line += " | ";
        }
        line += ids[index];
    }
    return line;
}

[[nodiscard]]
std::string default_provider(const std::vector<ProviderOffer>& providers) {
    for (const ProviderOffer& provider : providers) {
        if (provider.provider == "taskbar") {
            return "taskbar";
        }
    }
    return {};
}

[[nodiscard]]
std::string blank_value(const std::vector<ProviderOffer>&) {
    return {};
}

[[nodiscard]]
std::string normalize_description(const std::string_view description) {
    std::string normalized;
    normalized.reserve(description.size());
    for (std::size_t index = 0; index < description.size(); ++index) {
        const char character = description[index];
        if (character == '\r' || character == '\n') {
            if (character == '\r' &&
                index + 1 < description.size() &&
                description[index + 1] == '\n') {
                ++index;
            }
            normalized.push_back(' ');
            continue;
        }
        normalized.push_back(character);
    }
    return std::string {trim(normalized)};
}

/** First non-empty description in provider-id order. Catalog text only. */
[[nodiscard]]
std::string unset_catalog_description(
    const std::vector<ProviderOffer>& providers
) {
    std::vector<const ProviderOffer*> ordered;
    ordered.reserve(providers.size());
    for (const ProviderOffer& provider : providers) {
        ordered.push_back(&provider);
    }
    std::ranges::sort(ordered, {}, [](const ProviderOffer* provider) {
        return std::string_view {provider->provider};
    });
    for (const ProviderOffer* provider : ordered) {
        std::string description = normalize_description(provider->description);
        if (!description.empty()) {
            return description;
        }
    }
    return {};
}

[[nodiscard]]
std::optional<std::string> alias_name(
    const std::string_view provider,
    const std::string_view command
) {
    if (provider.empty() || !is_activation_command(command)) {
        return std::nullopt;
    }
    const auto program = command.substr(activation_prefix.size());
    if (provider == "taskbar") {
        return "taskbar_activate_" + std::string {program};
    }
    if (provider == program) {
        return std::string {provider} + "_activate";
    }
    return std::string {provider} + "_activate_" + std::string {program};
}

[[nodiscard]]
std::optional<std::string> ordinary_alias_name(
    const std::string_view provider,
    const std::string_view command
) {
    if (provider.empty() || !is_ordinary_command(command)) {
        return std::nullopt;
    }
    return std::string {provider} + "_" + std::string {command};
}

constexpr std::string_view activation_preamble[] {
    "# Selects the provider for a shared activation command.",
    "# Format: activate_<program> = <provider>",
    "# A blank value leaves the shared name unresolved."
};

constexpr std::string_view shared_preamble[] {
    "# Selects the provider for a shared command.",
    "# Format: <command_name> = <provider>",
    "# A blank value leaves the shared name unresolved."
};

void append_preamble(
    std::vector<std::string>& output,
    const std::span<const std::string_view> preamble
) {
    for (const std::string_view line : preamble) {
        output.emplace_back(line);
    }
    if (!preamble.empty()) {
        output.emplace_back();
    }
}

[[nodiscard]]
bool has_preamble(
    const std::vector<TextLine>& lines,
    const std::size_t section,
    const std::span<const std::string_view> preamble
) {
    if (preamble.empty() || section < preamble.size()) {
        return preamble.empty();
    }
    std::size_t end = section;
    if (trim(lines[end - 1].text).empty()) {
        --end;
    }
    if (end < preamble.size()) {
        return false;
    }
    const std::size_t start = end - preamble.size();
    for (std::size_t index = 0; index < preamble.size(); ++index) {
        if (lines[start + index].text != preamble[index]) {
            return false;
        }
    }
    return true;
}

[[nodiscard]]
MapPolicy activation_policy() {
    return MapPolicy {
        "activation_map",
        "activate_commands.map",
        "activation alias",
        activation_preamble,
        false,
        false,
        true,
        false,
        &is_activation_command,
        &alias_name,
        &default_provider
    };
}

[[nodiscard]]
MapPolicy shared_policy() {
    return MapPolicy {
        "shared_commands",
        "shared_commands.map",
        "shared command alias",
        shared_preamble,
        true,
        true,
        false,
        true,
        &is_ordinary_command,
        &ordinary_alias_name,
        &blank_value
    };
}

[[nodiscard]]
std::string unchanged_note(const std::string_view file_label) {
    return ". " + std::string {file_label} + " was not changed.";
}

[[nodiscard]]
std::map<std::string, std::vector<ProviderOffer>> group_offers(
    const std::vector<ac::taskbar::ActivationOffer>& offers,
    const MapPolicy& policy
) {
    std::map<std::string, std::vector<ProviderOffer>> grouped;
    for (const ac::taskbar::ActivationOffer& offer : offers) {
        if (offer.provider.empty() || !policy.accepts(offer.command)) {
            continue;
        }
        auto& providers = grouped[offer.command];
        const bool seen = std::ranges::any_of(
            providers,
            [&](const ProviderOffer& provider) {
                return provider.provider == offer.provider;
            }
        );
        if (seen) {
            continue;
        }
        providers.push_back(ProviderOffer {
            offer.provider,
            offer.description,
            offer.application
        });
    }
    return grouped;
}

[[nodiscard]]
MapText read_map(
    const std::filesystem::path& path,
    const std::string_view file_label
) {
    const std::string note = unchanged_note(file_label);
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error) {
        return MapText {
            false,
            false,
            {},
            "Failed to inspect " + path.string() + ": " + error.message() + note
        };
    }
    if (!exists) {
        return MapText {true, true, {}, {}};
    }
    error.clear();
    if (std::filesystem::is_directory(path, error) && !error) {
        return MapText {
            false,
            false,
            {},
            path.string() + " is a directory" + note
        };
    }
    if (error || !std::filesystem::is_regular_file(path, error)) {
        return MapText {
            false,
            false,
            {},
            "Failed to inspect " + path.string() + note
        };
    }

    std::ifstream input {path, std::ios::binary};
    if (!input) {
        return MapText {
            false,
            false,
            {},
            "Failed to read " + path.string() + note
        };
    }
    std::string text {
        std::istreambuf_iterator<char> {input},
        std::istreambuf_iterator<char> {}
    };
    if (input.bad()) {
        return MapText {
            false,
            false,
            {},
            "Failed to read " + path.string() + note
        };
    }
    if (text.find('\0') != std::string::npos) {
        return MapText {
            false,
            false,
            {},
            path.string() + " contains a NUL byte" + note
        };
    }
    return MapText {true, false, std::move(text), {}};
}

[[nodiscard]]
std::map<std::string, std::string> saved_selections(
    const std::string_view text,
    const MapPolicy& policy
) {
    std::map<std::string, std::string> selections;
    const auto lines = split_lines(text);
    bool in_section = false;
    for (const TextLine& line : lines) {
        if (is_section_header(line.text)) {
            in_section = is_named_section(line.text, policy.section);
            continue;
        }
        if (!in_section) {
            continue;
        }
        const auto assignment = assignment_of(line.text);
        if (!assignment || !policy.accepts(assignment->first)) {
            continue;
        }
        selections.insert_or_assign(assignment->first, assignment->second);
    }
    return selections;
}

[[nodiscard]]
std::string render_map(
    const std::string& existing,
    const std::map<std::string, std::vector<ProviderOffer>>& grouped,
    const MapPolicy& policy
) {
    const std::string newline =
        existing.find("\r\n") == std::string::npos ? "\n" : "\r\n";
    const bool trailing_newline =
        existing.empty() || existing.back() == '\n';
    const auto lines = split_lines(existing);

    std::optional<std::size_t> section;
    for (std::size_t index = 0; index < lines.size(); ++index) {
        if (is_named_section(lines[index].text, policy.section)) {
            section = index;
            break;
        }
    }

    std::size_t section_end = lines.size();
    std::vector<std::string> order;
    std::map<std::string, std::string> values;
    if (section) {
        for (std::size_t index = *section + 1; index < lines.size(); ++index) {
            if (is_section_header(lines[index].text)) {
                section_end = index;
                break;
            }
        }
        for (std::size_t index = *section + 1; index < section_end; ++index) {
            if (is_generated_choice_comment(lines[index].text)) {
                continue;
            }
            const auto assignment = assignment_of(lines[index].text);
            if (!assignment || !policy.accepts(assignment->first)) {
                continue;
            }
            if (!values.contains(assignment->first)) {
                order.push_back(assignment->first);
            }
            values.insert_or_assign(assignment->first, assignment->second);
        }
    }

    std::vector<std::string> added;
    for (const auto& [command, providers] : grouped) {
        if (providers.size() < 2 || values.contains(command)) {
            continue;
        }
        added.push_back(command);
    }

    std::vector<std::string> retained;
    retained.reserve(order.size());
    for (const std::string& command : order) {
        if (!policy.prune_unoffered) {
            retained.push_back(command);
            continue;
        }
        const auto found = grouped.find(command);
        if (found == grouped.end() || found->second.empty()) {
            continue;
        }
        retained.push_back(command);
    }

    const bool create_section = !section && !added.empty();
    if (!section && added.empty()) {
        return existing;
    }

    std::vector<std::string> output;
    const auto write_preamble = [&] {
        if (!output.empty() && !trim(output.back()).empty()) {
            output.emplace_back();
        }
        append_preamble(output, policy.preamble);
    };
    if (section) {
        const bool present = has_preamble(lines, *section, policy.preamble);
        if (policy.insert_missing_preamble && !present) {
            for (std::size_t index = 0; index < *section; ++index) {
                output.push_back(lines[index].text);
            }
            write_preamble();
            output.push_back(lines[*section].text);
        }
        else {
            for (std::size_t index = 0; index <= *section; ++index) {
                output.push_back(lines[index].text);
            }
        }
    }
    else {
        for (const TextLine& line : lines) {
            output.push_back(line.text);
        }
        if (existing.empty() || policy.insert_missing_preamble) {
            write_preamble();
        }
        else if (!output.empty() && !trim(output.back()).empty()) {
            output.emplace_back();
        }
        output.emplace_back("[" + std::string {policy.section} + "]");
    }
    if (policy.blank_line_after_section &&
        (!retained.empty() || !added.empty())) {
        output.emplace_back();
    }

    bool first_block = true;
    const auto emit_assignment = [&](
        const std::string& command,
        const std::string& value
    ) {
        if (!first_block) {
            output.emplace_back();
        }
        first_block = false;
        const auto found = grouped.find(command);
        const std::vector<ProviderOffer> empty;
        output.push_back(choices_comment(
            found == grouped.end() ? empty : found->second
        ));
        output.push_back(command + " = " + value);
    };

    for (const std::string& command : retained) {
        emit_assignment(command, values.at(command));
    }
    for (const std::string& command : added) {
        emit_assignment(command, policy.initial_value(grouped.at(command)));
    }
    if (section) {
        for (std::size_t index = section_end; index < lines.size(); ++index) {
            output.push_back(lines[index].text);
        }
    }

    if (create_section && output.empty()) {
        return existing;
    }
    return join_lines(
        output,
        newline,
        trailing_newline || existing.empty() || create_section
    );
}

[[nodiscard]]
const ProviderOffer* find_provider(
    const std::vector<ProviderOffer>& providers,
    const std::string_view provider
) {
    const auto found = std::ranges::find(providers, provider, &ProviderOffer::provider);
    return found == providers.end() ? nullptr : &*found;
}

[[nodiscard]]
ac::taskbar::ActivationResolution resolve_selections(
    const std::map<std::string, std::string>& selections,
    const std::map<std::string, std::vector<ProviderOffer>>& grouped,
    const std::vector<std::string>& occupied_names,
    const MapPolicy& policy
) {
    std::unordered_set<std::string> occupied {
        occupied_names.begin(),
        occupied_names.end()
    };
    std::unordered_set<std::string> used_aliases;
    ac::taskbar::ActivationResolution resolution;

    std::set<std::string> commands;
    for (const auto& [command, providers] : grouped) {
        (void)providers;
        commands.insert(command);
    }
    for (const auto& [command, value] : selections) {
        (void)value;
        if (policy.accepts(command)) {
            commands.insert(command);
        }
    }

    for (const std::string& command : commands) {
        const auto grouped_providers = grouped.find(command);
        const std::vector<ProviderOffer> empty;
        const std::vector<ProviderOffer>& providers =
            grouped_providers == grouped.end()
                ? empty
                : grouped_providers->second;
        const auto saved = selections.find(command);
        const bool has_saved = saved != selections.end();
        if (providers.size() == 1 && !has_saved) {
            continue;
        }
        if (providers.empty() && !has_saved) {
            continue;
        }

        ac::taskbar::ActivationBinding binding;
        binding.command = command;
        for (const ProviderOffer& provider : providers) {
            const auto name = policy.alias_for(provider.provider, command);
            if (!name) {
                continue;
            }
            if (*name == command ||
                occupied.contains(*name) ||
                used_aliases.contains(*name)) {
                resolution.reports.push_back(
                    std::string {policy.alias_kind} + " '" + *name +
                    "' was not published because that command already exists."
                );
                continue;
            }
            used_aliases.insert(*name);
            binding.aliases.push_back(ac::taskbar::ActivationAlias {
                *name,
                provider.provider,
                command,
                provider.description,
                provider.application
            });
        }
        std::ranges::sort(binding.aliases, {}, &ac::taskbar::ActivationAlias::name);
        binding.offer_count = providers.size();

        if (!has_saved || saved->second.empty()) {
            binding.blank_selection = true;
            binding.explicit_blank = has_saved && saved->second.empty();
            if (binding.explicit_blank && binding.offer_count >= 2) {
                binding.unset_description = unset_catalog_description(providers);
            }
            if (policy.report_unselected) {
                resolution.reports.push_back(
                    std::string {policy.file_label} +
                    " has no provider selected for " + command + "."
                );
            }
        }
        else if (const ProviderOffer* provider =
                     find_provider(providers, saved->second)) {
            binding.resolved = true;
            binding.provider = provider->provider;
            binding.description = provider->description;
            binding.application = provider->application;
        }
        else {
            resolution.reports.push_back(
                std::string {policy.file_label} + " selects '" + saved->second +
                "' for " + command + ", but that provider is unavailable."
            );
        }
        resolution.bindings.push_back(std::move(binding));
    }
    return resolution;
}

[[nodiscard]]
bool publish_text(
    const std::filesystem::path& destination,
    const std::string& contents,
    std::string& error,
    const std::string_view file_label
) {
    const std::string note = unchanged_note(file_label);
    std::error_code directory_error;
    std::filesystem::create_directories(
        destination.parent_path(),
        directory_error
    );
    if (directory_error) {
        error = "Failed to create " + destination.parent_path().string() +
            ": " + directory_error.message() + note;
        return false;
    }

    const auto temporary = destination.parent_path() /
        (destination.filename().wstring() + L"." +
         std::to_wstring(GetCurrentProcessId()) + L"." +
         std::to_wstring(GetTickCount64()) + L".tmp");
    HANDLE handle = CreateFileW(
        temporary.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    if (handle == INVALID_HANDLE_VALUE) {
        error = "Failed to write " + temporary.string() + note;
        return false;
    }

    std::size_t written = 0;
    while (written < contents.size()) {
        DWORD chunk = 0;
        const DWORD request = static_cast<DWORD>(std::min<std::size_t>(
            contents.size() - written,
            1u << 20
        ));
        if (!WriteFile(
                handle,
                contents.data() + written,
                request,
                &chunk,
                nullptr
            ) || chunk == 0) {
            CloseHandle(handle);
            DeleteFileW(temporary.c_str());
            error = "Failed to write " + temporary.string() + note;
            return false;
        }
        written += chunk;
    }
    if (!CloseHandle(handle)) {
        DeleteFileW(temporary.c_str());
        error = "Failed to close " + temporary.string() + note;
        return false;
    }
    if (!MoveFileExW(
            temporary.c_str(),
            destination.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
        )) {
        DeleteFileW(temporary.c_str());
        error = "Failed to replace " + destination.string() + note;
        return false;
    }
    return true;
}

[[nodiscard]]
ac::taskbar::ActivationResolution resolve_map(
    const std::filesystem::path& map_path,
    const std::vector<ac::taskbar::ActivationOffer>& offers,
    const std::vector<std::string>& occupied_names,
    const MapPolicy& policy
) {
    const MapText map = read_map(map_path, policy.file_label);
    if (!map.ok) {
        ac::taskbar::ActivationResolution resolution;
        resolution.reports.push_back(map.error);
        return resolution;
    }
    return resolve_selections(
        saved_selections(map.text, policy),
        group_offers(offers, policy),
        occupied_names,
        policy
    );
}

[[nodiscard]]
ac::taskbar::ActivationMapUpdate update_map(
    const std::filesystem::path& map_path,
    const std::vector<ac::taskbar::ActivationOffer>& offers,
    const std::vector<std::string>& occupied_names,
    const MapPolicy& policy
) {
    ac::taskbar::ActivationMapUpdate update;
    const MapText map = read_map(map_path, policy.file_label);
    if (!map.ok) {
        update.error = map.error;
        return update;
    }

    const auto grouped = group_offers(offers, policy);
    const std::string rendered = render_map(map.text, grouped, policy);
    if (rendered != map.text) {
        if (!publish_text(map_path, rendered, update.error, policy.file_label)) {
            return update;
        }
    }
    update.ok = true;
    update.resolution = resolve_selections(
        saved_selections(rendered, policy),
        grouped,
        occupied_names,
        policy
    );
    return update;
}

void claim_aliases(
    ac::taskbar::ActivationResolution& resolution,
    std::unordered_set<std::string>& claimed,
    const std::string_view alias_kind
) {
    for (ac::taskbar::ActivationBinding& binding : resolution.bindings) {
        std::vector<ac::taskbar::ActivationAlias> kept;
        kept.reserve(binding.aliases.size());
        for (ac::taskbar::ActivationAlias& alias : binding.aliases) {
            if (!claimed.insert(alias.name).second) {
                resolution.reports.push_back(
                    std::string {alias_kind} + " '" + alias.name +
                    "' was not published because that command already exists."
                );
                continue;
            }
            kept.push_back(std::move(alias));
        }
        binding.aliases = std::move(kept);
    }
}

} // namespace

ac::taskbar::ActivationResolution ac::taskbar::resolve_activation_commands(
    const std::filesystem::path& map_path,
    const std::vector<ActivationOffer>& offers,
    const std::vector<std::string>& occupied_names
) {
    return resolve_map(map_path, offers, occupied_names, activation_policy());
}

ac::taskbar::ActivationMapUpdate ac::taskbar::update_activation_map(
    const std::filesystem::path& map_path,
    const std::vector<ActivationOffer>& offers,
    const std::vector<std::string>& occupied_names
) {
    return update_map(map_path, offers, occupied_names, activation_policy());
}

ac::taskbar::ActivationResolution ac::taskbar::resolve_shared_commands(
    const std::filesystem::path& map_path,
    const std::vector<ActivationOffer>& offers,
    const std::vector<std::string>& occupied_names
) {
    return resolve_map(map_path, offers, occupied_names, shared_policy());
}

ac::taskbar::ActivationMapUpdate ac::taskbar::update_shared_command_map(
    const std::filesystem::path& map_path,
    const std::vector<ActivationOffer>& offers,
    const std::vector<std::string>& occupied_names
) {
    return update_map(map_path, offers, occupied_names, shared_policy());
}

void ac::taskbar::reconcile_provider_aliases(
    ActivationResolution& activation,
    ActivationResolution& shared,
    const std::vector<std::string>& occupied_names
) {
    std::unordered_set<std::string> claimed {
        occupied_names.begin(),
        occupied_names.end()
    };
    for (const ActivationBinding& binding : activation.bindings) {
        claimed.insert(binding.command);
    }
    for (const ActivationBinding& binding : shared.bindings) {
        claimed.insert(binding.command);
    }
    claim_aliases(activation, claimed, "activation alias");
    claim_aliases(shared, claimed, "shared command alias");
}
