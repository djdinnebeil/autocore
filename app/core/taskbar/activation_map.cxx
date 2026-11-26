/**
 * \file activation_map.cxx
 * \brief Selects the provider for a shared keymap command.
 *
 * auto_core_keymap.exe --build rewrites both maps after a successful
 * discovery. Startup reads activate_commands.map and does not write it.
 * One activation provider resolves in memory even when the saved row is
 * missing or stale. `activate_commands.map` keeps `activate_<program>`.
 * `shared_commands.map` keeps every other duplicated command.
 */
module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cctype>

module auto_core.taskbar;

import std;
import auto_core.core.paths;

namespace {

constexpr std::string_view activation_prefix = "activate_";
constexpr std::string_view choice_prefix = "# Choice:";
constexpr std::string_view choices_prefix = "# Choices:";
constexpr std::string_view needs_initialization_comment =
    "# Needs initialization";

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
    /** Write a row for one provider and select that provider. */
    bool record_single_provider {false};
    /** Activation rows use `# Choices:` for every provider count. */
    bool plural_choices {false};
    /** Write activation rows by smallest provider id, then command name. */
    bool order_by_provider {false};
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
std::string plural_choices_comment(
    const std::vector<ProviderOffer>& providers
) {
    std::vector<std::string> ids;
    ids.reserve(providers.size());
    for (const ProviderOffer& provider : providers) {
        ids.push_back(provider.provider);
    }
    std::ranges::sort(ids);
    std::string line {choices_prefix};
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
std::string activation_initial_value(
    const std::vector<ProviderOffer>& providers
) {
    if (providers.size() == 1) {
        return providers.front().provider;
    }
    return {};
}

[[nodiscard]]
bool offers_provider(
    const std::vector<ProviderOffer>& providers,
    const std::string_view provider
) {
    return std::ranges::any_of(providers, [&](const ProviderOffer& offer) {
        return offer.provider == provider;
    });
}

[[nodiscard]]
std::string activation_selection_value(
    const std::vector<ProviderOffer>& providers,
    const std::string& saved
) {
    if (providers.size() == 1) {
        return providers.front().provider;
    }
    if (offers_provider(providers, saved)) {
        return saved;
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
    if (provider.empty() || provider == "taskbar" ||
        !is_activation_command(command)) {
        return std::nullopt;
    }
    const auto program = command.substr(activation_prefix.size());
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
        true,
        true,
        true,
        true,
        &is_activation_command,
        &alias_name,
        &activation_initial_value
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
        false,
        false,
        false,
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
std::string ascii_lower_copy(const std::string_view value) {
    std::string result {value};
    std::ranges::transform(result, result.begin(), [](const unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return result;
}

enum class DeclaredKeyKind {
    none,
    agreed,
    conflict
};

struct DeclaredKey {
    DeclaredKeyKind kind {DeclaredKeyKind::none};
    std::string key;
    std::vector<std::string> keys;
};

[[nodiscard]]
DeclaredKey declared_component_key(const std::vector<ProviderOffer>& providers) {
    std::vector<std::string> keys;
    for (const ProviderOffer& provider : providers) {
        if (provider.provider == "taskbar" || provider.application.empty()) {
            continue;
        }
        const std::string key = ascii_lower_copy(provider.application);
        if (std::ranges::find(keys, key) == keys.end()) {
            keys.push_back(key);
        }
    }
    std::ranges::sort(keys);
    if (keys.empty()) {
        return {};
    }
    if (keys.size() > 1) {
        return DeclaredKey {DeclaredKeyKind::conflict, {}, std::move(keys)};
    }
    return DeclaredKey {DeclaredKeyKind::agreed, keys.front(), std::move(keys)};
}

[[nodiscard]]
std::string pending_value_for(const std::string_view key) {
    std::string value {key};
    value += ".map";
    return value;
}

[[nodiscard]]
bool application_map_missing(const std::string_view key) {
    const auto path = ac::paths::taskbar_applications_directory() /
        (std::string {key} + ".map");
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error) {
        return false;
    }
    return !present;
}

[[nodiscard]]
std::string dependency_comment(
    const std::vector<ProviderOffer>& providers,
    const std::string& value,
    const bool plural_choices
) {
    if (!plural_choices) {
        const DeclaredKey declared = declared_component_key(providers);
        if (declared.kind == DeclaredKeyKind::agreed &&
            value == pending_value_for(declared.key)) {
            return std::string {needs_initialization_comment};
        }
    }
    if (plural_choices) {
        return plural_choices_comment(providers);
    }
    return choices_comment(providers);
}

[[nodiscard]]
std::string conflict_report(
    const std::string& command,
    const std::vector<std::string>& keys
) {
    std::string report = command;
    report += " declares conflicting application keys";
    for (std::size_t index = 0; index < keys.size(); ++index) {
        report += index == 0 ? " '" : " and '";
        report += keys[index];
        report += "'";
    }
    report += ".";
    return report;
}

[[nodiscard]]
std::string dependency_value(
    const std::vector<ProviderOffer>& providers,
    const std::string& saved
) {
    const DeclaredKey declared = declared_component_key(providers);
    if (declared.kind != DeclaredKeyKind::agreed) {
        return saved;
    }
    const std::string pending = pending_value_for(declared.key);
    if (application_map_missing(declared.key)) {
        return saved.empty() ? saved : pending;
    }
    return saved;
}

[[nodiscard]]
std::string smallest_provider_id(const std::vector<ProviderOffer>& providers) {
    std::string key;
    for (const ProviderOffer& provider : providers) {
        if (key.empty() || provider.provider < key) {
            key = provider.provider;
        }
    }
    return key;
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
    std::map<std::string, std::string> added_values;
    for (const auto& [command, providers] : grouped) {
        if (!policy.record_single_provider) {
            const DeclaredKey declared = declared_component_key(providers);
            if (declared.kind == DeclaredKeyKind::agreed &&
                application_map_missing(declared.key) &&
                !values.contains(command)) {
                added.push_back(command);
                added_values.emplace(command, pending_value_for(declared.key));
                continue;
            }
        }
        const bool single =
            policy.record_single_provider && providers.size() == 1;
        if ((!single && providers.size() < 2) || values.contains(command)) {
            continue;
        }
        added.push_back(command);
        added_values.emplace(command, policy.initial_value(providers));
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
        const std::vector<ProviderOffer>& providers =
            found == grouped.end() ? empty : found->second;
        output.push_back(dependency_comment(
            providers,
            value,
            policy.plural_choices
        ));
        output.push_back(command + " = " + value);
    };

    struct WrittenCommand {
        std::string command;
        bool added {false};
    };
    std::vector<WrittenCommand> commands;
    commands.reserve(retained.size() + added.size());
    for (const std::string& command : retained) {
        commands.push_back(WrittenCommand {command, false});
    }
    for (const std::string& command : added) {
        commands.push_back(WrittenCommand {command, true});
    }
    if (policy.order_by_provider) {
        const auto provider_key = [&](const std::string& command) {
            const auto found = grouped.find(command);
            if (found == grouped.end()) {
                return std::string {};
            }
            return smallest_provider_id(found->second);
        };
        std::ranges::sort(
            commands,
            [&](const WrittenCommand& left, const WrittenCommand& right) {
                const std::string left_key = provider_key(left.command);
                const std::string right_key = provider_key(right.command);
                if (left_key != right_key) {
                    return left_key < right_key;
                }
                return left.command < right.command;
            }
        );
    }

    for (const WrittenCommand& row : commands) {
        if (row.added) {
            emit_assignment(row.command, added_values.at(row.command));
            continue;
        }
        const auto found = grouped.find(row.command);
        const std::vector<ProviderOffer> empty;
        const std::vector<ProviderOffer>& providers =
            found == grouped.end() ? empty : found->second;
        std::string value = values.at(row.command);
        if (policy.record_single_provider) {
            value = activation_selection_value(providers, value);
        }
        else {
            value = dependency_value(providers, value);
        }
        emit_assignment(row.command, value);
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
        const DeclaredKey declared = declared_component_key(providers);
        if (declared.kind == DeclaredKeyKind::conflict) {
            resolution.reports.push_back(
                conflict_report(command, declared.keys)
            );
        }
        const bool explicit_blank = has_saved && saved->second.empty();
        if (!policy.record_single_provider &&
            declared.kind == DeclaredKeyKind::agreed &&
            application_map_missing(declared.key) &&
            !explicit_blank) {
            continue;
        }
        if (!policy.record_single_provider &&
            providers.size() == 1 && !has_saved) {
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

        if (policy.record_single_provider && providers.size() == 1) {
            const ProviderOffer& provider = providers.front();
            binding.resolved = true;
            binding.provider = provider.provider;
            binding.description = provider.description;
            binding.application = provider.application;
        }
        else if (!has_saved || saved->second.empty()) {
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
            binding.provider = saved->second;
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

[[nodiscard]]
bool is_managed_activation_comment(const std::string_view text) {
    const auto value = trim(text);
    return value == needs_initialization_comment ||
        is_generated_choice_comment(value);
}

[[nodiscard]]
bool catalog_names_command(
    const std::filesystem::path& path,
    const std::string_view provider,
    const std::string& command
) {
    std::ifstream input {path};
    if (!input) {
        return false;
    }
    const auto alias = alias_name(provider, command);
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const auto split = line.find(" - ");
        const auto name = trim(
            split == std::string::npos ? std::string_view {line}
                                        : std::string_view {line}.substr(0, split)
        );
        if (name == command || (alias && name == *alias)) {
            return true;
        }
    }
    return false;
}

[[nodiscard]]
std::vector<ProviderOffer> promotion_providers(const std::string& command) {
    std::vector<ProviderOffer> providers;
    providers.push_back(ProviderOffer {"taskbar", {}, {}});
    const auto directory = ac::paths::keymap_directory() / "components";
    std::error_code error;
    if (!std::filesystem::exists(directory, error) || error) {
        return providers;
    }
    std::filesystem::directory_iterator iterator {directory, error};
    while (!error && iterator != std::filesystem::directory_iterator {}) {
        const bool regular = iterator->is_regular_file(error);
        if (!error && regular) {
            const std::string filename = iterator->path().filename().string();
            constexpr std::string_view suffix = ".keymap_commands.txt";
            if (filename.ends_with(suffix)) {
                const std::string provider = filename.substr(
                    0,
                    filename.size() - suffix.size()
                );
                if (!provider.empty() &&
                    provider != "taskbar" &&
                    catalog_names_command(iterator->path(), provider, command)) {
                    providers.push_back(ProviderOffer {provider, {}, {}});
                }
            }
        }
        iterator.increment(error);
    }
    return providers;
}

struct EntryReplacement {
    std::string value;
    std::string comment;
};

/**
 * \brief Replaces or appends activation assignments without rewriting
 *        unrelated lines.
 */
[[nodiscard]]
std::optional<std::string> rewrite_activation_entries(
    const std::string& existing,
    const std::map<std::string, EntryReplacement>& replacements,
    const bool append_missing
) {
    if (replacements.empty()) {
        return std::nullopt;
    }
    const std::string newline =
        existing.find("\r\n") == std::string::npos ? "\n" : "\r\n";
    const bool trailing_newline =
        existing.empty() || existing.back() == '\n';
    auto lines = split_lines(existing);
    std::optional<std::size_t> section;
    for (std::size_t index = 0; index < lines.size(); ++index) {
        if (is_named_section(lines[index].text, "activation_map")) {
            section = index;
            break;
        }
    }

    bool changed = false;
    std::vector<std::string> missing;
    if (section) {
        std::size_t section_end = lines.size();
        for (std::size_t index = *section + 1; index < lines.size(); ++index) {
            if (is_section_header(lines[index].text)) {
                section_end = index;
                break;
            }
        }
        std::vector<std::size_t> assignments;
        for (std::size_t index = *section + 1; index < section_end; ++index) {
            const auto assignment = assignment_of(lines[index].text);
            if (!assignment || !replacements.contains(assignment->first)) {
                continue;
            }
            assignments.push_back(index);
        }
        std::map<std::string, bool> seen;
        for (auto cursor = assignments.rbegin(); cursor != assignments.rend(); ++cursor) {
            const auto assignment = assignment_of(lines[*cursor].text);
            seen[assignment->first] = true;
            const EntryReplacement& replacement = replacements.at(assignment->first);
            if (assignment->second == replacement.value) {
                std::size_t comment_at = *cursor;
                while (comment_at > *section + 1 &&
                       trim(lines[comment_at - 1].text).empty()) {
                    --comment_at;
                }
                if (comment_at > *section &&
                    is_managed_activation_comment(lines[comment_at - 1].text) &&
                    trim(lines[comment_at - 1].text) == replacement.comment) {
                    continue;
                }
            }
            changed = true;
            std::size_t comment_at = *cursor;
            while (comment_at > *section + 1 &&
                   trim(lines[comment_at - 1].text).empty()) {
                --comment_at;
            }
            if (comment_at > *section &&
                is_managed_activation_comment(lines[comment_at - 1].text)) {
                lines[comment_at - 1].text = replacement.comment;
                lines[*cursor].text =
                    assignment->first + " = " + replacement.value;
            }
            else {
                lines.insert(
                    lines.begin() + static_cast<std::ptrdiff_t>(*cursor),
                    TextLine {replacement.comment}
                );
                lines[*cursor + 1].text =
                    assignment->first + " = " + replacement.value;
            }
        }
        for (const auto& [command, replacement] : replacements) {
            (void)replacement;
            if (!seen.contains(command)) {
                missing.push_back(command);
            }
        }
        if (append_missing && !missing.empty()) {
            changed = true;
            section_end = lines.size();
            for (std::size_t index = *section + 1; index < lines.size(); ++index) {
                if (is_section_header(lines[index].text)) {
                    section_end = index;
                    break;
                }
            }
            std::size_t insert_at = section_end;
            for (const std::string& command : missing) {
                const EntryReplacement& replacement = replacements.at(command);
                if (insert_at > *section + 1 &&
                    !trim(lines[insert_at - 1].text).empty()) {
                    lines.insert(
                        lines.begin() + static_cast<std::ptrdiff_t>(insert_at),
                        TextLine {}
                    );
                    ++insert_at;
                }
                lines.insert(
                    lines.begin() + static_cast<std::ptrdiff_t>(insert_at),
                    TextLine {replacement.comment}
                );
                ++insert_at;
                lines.insert(
                    lines.begin() + static_cast<std::ptrdiff_t>(insert_at),
                    TextLine {command + " = " + replacement.value}
                );
                ++insert_at;
            }
        }
    }
    else if (append_missing) {
        changed = true;
        std::vector<std::string> output;
        for (const TextLine& line : lines) {
            output.push_back(line.text);
        }
        if (existing.empty()) {
            append_preamble(output, activation_preamble);
        }
        else if (!output.empty() && !trim(output.back()).empty()) {
            output.emplace_back();
        }
        output.emplace_back("[activation_map]");
        bool first = true;
        for (const auto& [command, replacement] : replacements) {
            if (!first) {
                output.emplace_back();
            }
            first = false;
            output.push_back(replacement.comment);
            output.push_back(command + " = " + replacement.value);
        }
        return join_lines(output, newline, true);
    }

    if (!changed) {
        return std::nullopt;
    }
    std::vector<std::string> output;
    output.reserve(lines.size());
    for (const TextLine& line : lines) {
        output.push_back(line.text);
    }
    return join_lines(
        output,
        newline,
        trailing_newline || existing.empty()
    );
}

[[nodiscard]]
bool is_pending_command(const std::string_view command) {
    return is_activation_command(command);
}

[[nodiscard]]
bool is_pending_key(const std::string_view key) {
    if (key.empty()) {
        return false;
    }
    const auto first = static_cast<unsigned char>(key.front());
    if (!std::isalpha(first)) {
        return false;
    }
    return std::ranges::all_of(key, [](const char character) {
        const auto value = static_cast<unsigned char>(character);
        return std::isalnum(value) || character == '_';
    });
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

ac::taskbar::KeymapBuildLock::~KeymapBuildLock() {
    release();
}

void ac::taskbar::KeymapBuildLock::release() noexcept {
    if (handle_ == nullptr) {
        return;
    }
    const HANDLE handle = static_cast<HANDLE>(handle_);
    if (locked_) {
        OVERLAPPED overlapped {};
        UnlockFileEx(handle, 0, 1, 0, &overlapped);
        locked_ = false;
    }
    CloseHandle(handle);
    handle_ = nullptr;
}

ac::taskbar::KeymapBuildLock::Status ac::taskbar::KeymapBuildLock::acquire(
    std::string& error,
    const bool wait
) {
    release();
    const auto lock_path = ac::paths::installation_root() /
        "data" / "auto_core" / "keymap_build.lock";
    std::error_code directory_error;
    std::filesystem::create_directories(
        lock_path.parent_path(),
        directory_error
    );
    if (directory_error) {
        error = "Failed to create " + lock_path.parent_path().string() +
            ": " + directory_error.message() + ".";
        return Status::failed;
    }

    const HANDLE handle = CreateFileW(
        lock_path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    if (handle == INVALID_HANDLE_VALUE) {
        error = "Failed to open " + lock_path.string() + " (error " +
            std::to_string(GetLastError()) + ").";
        return Status::failed;
    }

    OVERLAPPED overlapped {};
    const DWORD flags = LOCKFILE_EXCLUSIVE_LOCK |
        (wait ? DWORD {0} : LOCKFILE_FAIL_IMMEDIATELY);
    if (!LockFileEx(handle, flags, 0, 1, 0, &overlapped)) {
        const DWORD lock_error = GetLastError();
        CloseHandle(handle);
        if (lock_error == ERROR_LOCK_VIOLATION) {
            return Status::busy;
        }
        error = "Failed to lock " + lock_path.string() + " (error " +
            std::to_string(lock_error) + ").";
        return Status::failed;
    }
    handle_ = handle;
    locked_ = true;
    return Status::acquired;
}

ac::taskbar::ActivationEntryUpdate ac::taskbar::note_missing_application_map(
    const std::string_view,
    const std::string_view
) {
    ActivationEntryUpdate update;
    update.ok = true;
    return update;
}

ac::taskbar::ActivationEntryUpdate ac::taskbar::promote_ready_application_map(
    const std::string_view,
    const std::vector<std::string>&
) {
    ActivationEntryUpdate update;
    update.ok = true;
    return update;
}
