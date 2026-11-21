/**
 * \file command_registry.ixx
 * \brief Stores and resolves commands available to runtime configuration.
 */
export module command_registry;

import std;

export namespace command_registry {

/** A bound keymap action with no arguments. */
using Action = std::function<void()>;
/** Builds an `Action` from the text inside `name(arguments)`. */
using Factory = std::function<Action(std::string_view)>;

/** Separator between a catalog identity and its description. */
inline constexpr std::string_view catalog_description_separator = " - ";

/**
 * \brief One registered command prepared for catalog publication.
 *
 * `hello_identity` is the command name, or the factory autocomplete text.
 */
struct CatalogEntry {
    std::string name;
    std::string description;
    std::string hello_identity;
    /** Provider id for the merged keymap catalog. Empty for hello and export. */
    std::string provider;
};

/**
 * \brief Turns line breaks into spaces and trims the description.
 * \return An empty string when nothing remains.
 */
[[nodiscard]]
inline std::string normalize_catalog_description(
    const std::string_view description
) {
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

    const auto first = normalized.find_first_not_of(" \t");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = normalized.find_last_not_of(" \t");
    return normalized.substr(first, last - first + 1);
}

/**
 * \brief Formats one catalog line.
 *
 * An empty description is the identity alone. A description is appended
 * with ` - ` and never leaves a dangling separator.
 */
[[nodiscard]]
inline std::string format_catalog_line(
    const std::string_view identity,
    const std::string_view description
) {
    const std::string normalized = normalize_catalog_description(description);
    if (normalized.empty()) {
        return std::string {identity};
    }

    std::string line;
    line.reserve(
        identity.size() + catalog_description_separator.size() +
        normalized.size()
    );
    line.append(identity);
    line.append(catalog_description_separator);
    line.append(normalized);
    return line;
}

/**
 * \brief Formats one merged keymap catalog line.
 *
 * The identity is `name [provider]`. An empty description omits ` - `.
 * `provider` is the display label, such as `Taskbar` or `Unset`.
 */
[[nodiscard]]
inline std::string format_merged_catalog_line(
    const std::string_view name,
    const std::string_view provider,
    const std::string_view description
) {
    std::string identity;
    identity.reserve(name.size() + provider.size() + 3);
    identity.append(name);
    identity.append(" [");
    identity.append(provider);
    identity.append("]");
    return format_catalog_line(identity, description);
}

class Registry {
public:
    /**
     * \brief Registers a no-argument command.
     *
     * `description` is catalog text. It is not part of the command name.
     * \throws std::logic_error if `name` is already registered.
     */
    template<typename Function>
        requires std::invocable<Function> &&
                 std::same_as<std::invoke_result_t<Function>, void>
    void add(
        std::string name,
        Function function,
        std::string description = {},
        std::string provider = {}
    ) {
        const auto [entry, inserted] = commands_.emplace(
            std::move(name),
            CommandEntry {
                Action {std::move(function)},
                std::move(description),
                std::move(provider)
            }
        );

        if (!inserted) {
            throw std::logic_error(
                "Duplicate runtime command: " + entry->first
            );
        }
    }

    /**
     * \brief Registers a parameterized command factory.
     *
     * An empty `autocomplete_value` becomes `name()`. `description` is
     * catalog text and is not part of the factory name.
     * \throws std::logic_error if `name` is already registered as a factory.
     */
    void add_factory(
        std::string name,
        Factory factory,
        std::string autocomplete_value = {},
        std::string description = {},
        std::string provider = {}
    ) {
        if (autocomplete_value.empty()) {
            autocomplete_value = name + "()";
        }

        const auto [entry, inserted] = factories_.emplace(
            std::move(name),
            FactoryEntry {
                std::move(factory),
                std::move(autocomplete_value),
                std::move(description),
                std::move(provider)
            }
        );

        if (!inserted) {
            throw std::logic_error(
                "Duplicate runtime command factory: " + entry->first
            );
        }
    }

    /** \brief Returns whether a command or factory uses `name`. */
    [[nodiscard]] bool contains(const std::string_view name) const {
        const std::string key {name};
        return commands_.contains(key) || factories_.contains(key);
    }

    /** \brief Sorted command and factory names (no argument templates). */
    [[nodiscard]] std::vector<std::string> registered_names() const {
        std::vector<std::string> names;
        names.reserve(commands_.size() + factories_.size());

        for (const auto& [name, action] : commands_) {
            names.push_back(name);
        }

        for (const auto& [name, entry] : factories_) {
            names.push_back(name);
        }

        std::ranges::sort(names);
        return names;
    }

    /**
     * \brief Sorted editor autocomplete strings.
     *
     * Plain commands use their name. Factories use their autocomplete value.
     */
    [[nodiscard]] std::vector<std::string> autocomplete_values() const {
        std::vector<std::string> values;
        values.reserve(commands_.size() + factories_.size());

        for (const auto& [name, action] : commands_) {
            values.push_back(name);
        }

        for (const auto& [name, entry] : factories_) {
            values.push_back(entry.autocomplete_value);
        }

        std::ranges::sort(values);
        return values;
    }

    /**
     * \brief Sorted catalog records.
     *
     * Names stay separate from descriptions. Factory hello text stays in
     * `hello_identity`.
     */
    [[nodiscard]] std::vector<CatalogEntry> catalog_entries() const {
        std::vector<CatalogEntry> entries;
        entries.reserve(commands_.size() + factories_.size());

        for (const auto& [name, entry] : commands_) {
            entries.push_back(CatalogEntry {
                name,
                entry.description,
                name,
                entry.provider
            });
        }

        for (const auto& [name, entry] : factories_) {
            entries.push_back(CatalogEntry {
                name,
                entry.description,
                entry.autocomplete_value,
                entry.provider
            });
        }

        std::ranges::sort(entries, {}, &CatalogEntry::name);
        return entries;
    }

    /** \brief Sorted `name` or `name - description` lines for catalog files. */
    [[nodiscard]] std::vector<std::string> catalog_lines() const {
        const std::vector<CatalogEntry> entries = catalog_entries();
        std::vector<std::string> lines;
        lines.reserve(entries.size());
        for (const CatalogEntry& entry : entries) {
            lines.push_back(format_catalog_line(entry.name, entry.description));
        }
        return lines;
    }

    /**
     * \brief Sorted hello lines.
     *
     * A plain command uses its name. A factory uses its autocomplete text.
     * A description follows that identity when one remains after normalization.
     */
    [[nodiscard]] std::vector<std::string> hello_lines() const {
        const std::vector<CatalogEntry> entries = catalog_entries();
        std::vector<std::string> lines;
        lines.reserve(entries.size());
        for (const CatalogEntry& entry : entries) {
            lines.push_back(format_catalog_line(
                entry.hello_identity,
                entry.description
            ));
        }
        return lines;
    }

    /**
     * \brief Resolves a keymap expression to an action.
     *
     * Surrounding whitespace is ignored. `name` looks up a plain command.
     * `name(arguments)` looks up a factory. Unknown names, a missing closing
     * parenthesis, or factory failure yield an empty `Action`.
     */
    [[nodiscard]] Action resolve(std::string_view expression) const {
        expression = trim(expression);
        const std::size_t opening_parenthesis = expression.find('(');

        if (opening_parenthesis == std::string_view::npos) {
            const auto command = commands_.find(std::string {expression});
            return command != commands_.end() ? command->second.action : nullptr;
        }

        if (expression.empty() || expression.back() != ')') {
            return {};
        }

        const std::string_view name = trim(
            expression.substr(0, opening_parenthesis)
        );
        const auto factory = factories_.find(std::string {name});

        if (factory == factories_.end()) {
            return {};
        }

        const std::string_view arguments = expression.substr(
            opening_parenthesis + 1,
            expression.size() - opening_parenthesis - 2
        );
        return factory->second.resolve(arguments);
    }

private:
    struct CommandEntry {
        Action action;
        std::string description;
        std::string provider;
    };

    struct FactoryEntry {
        Factory resolve;
        std::string autocomplete_value;
        std::string description;
        std::string provider;
    };

    static std::string_view trim(std::string_view value) {
        const std::size_t first = value.find_first_not_of(" \t");

        if (first == std::string_view::npos) {
            return {};
        }

        const std::size_t last = value.find_last_not_of(" \t");
        return value.substr(first, last - first + 1);
    }

    std::unordered_map<std::string, CommandEntry> commands_;
    std::unordered_map<std::string, FactoryEntry> factories_;
};

} // namespace command_registry
