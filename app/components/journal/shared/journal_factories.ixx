/**
 * \file journal_factories.ixx
 * \brief Compiled Journal factories and runtime alias definitions.
 *
 * Factory types are compiled. Alias names and arguments live in
 * `<factory_name>.list` under the journal data directory.
 * `journal_builder.exe` writes those files. `journal_ac.exe` reads them.
 */
export module journal_factories;

import std;
import command_registry;

export namespace journal::factories {

enum class FactoryId {
    print_choice,
    insert_text,
    roll_dice,
};

struct PrintChoiceArguments {
    std::string name;
    int lower_bound = 1;
};

struct InsertTextArguments {
    std::string text;
};

struct DiceArguments {
    int number_of_dice = 1;
    int number_of_sides = 6;
};

using FactoryArguments = std::variant<
    PrintChoiceArguments,
    InsertTextArguments,
    DiceArguments
>;

struct AliasDefinition {
    std::string alias;
    FactoryId factory = FactoryId::print_choice;
    FactoryArguments arguments;
};

struct FactoryDescriptor {
    FactoryId id;
    std::string_view name;
    std::string_view menu_label;
    std::string_view filename;
    std::string_view autocomplete;
};

enum class FieldKind {
    text,
    integer,
};

struct Field {
    std::string_view name;
    std::string_view prompt;
    FieldKind kind;
    bool optional = false;
    int default_integer = 0;
};

struct AliasFile {
    std::vector<AliasDefinition> aliases;
    std::vector<std::string> issues;
};

struct ReadAliasFile {
    bool missing = false;
    AliasFile file;
};

[[nodiscard]] std::span<const FactoryDescriptor> factories();

[[nodiscard]] const FactoryDescriptor* find_factory(FactoryId id);

[[nodiscard]] const FactoryDescriptor* find_factory(std::string_view name);

[[nodiscard]] std::span<const Field> fields(FactoryId id);

[[nodiscard]] std::filesystem::path alias_path(
    const std::filesystem::path& directory,
    FactoryId id
);

[[nodiscard]] bool is_alias_name(std::string_view name);

[[nodiscard]] bool is_reserved_alias_name(std::string_view name);

[[nodiscard]] std::expected<AliasDefinition, std::string> parse_alias_line(
    std::string_view line,
    FactoryId file_factory
);

[[nodiscard]] AliasFile parse_alias_file(
    std::string_view text,
    FactoryId file_factory
);

[[nodiscard]] ReadAliasFile read_alias_file(
    const std::filesystem::path& path,
    FactoryId file_factory
);

[[nodiscard]] std::optional<std::string> validate(const AliasDefinition& definition);

[[nodiscard]] std::optional<std::string> rejection_reason(
    const AliasDefinition& candidate,
    std::span<const AliasDefinition> already_loaded
);

[[nodiscard]] std::string field_value(
    const AliasDefinition& definition,
    std::string_view field_name
);

[[nodiscard]] std::expected<AliasDefinition, std::string> alias_from_fields(
    FactoryId factory,
    std::string alias,
    std::span<const std::string> values
);

[[nodiscard]] std::string serialize_alias(const AliasDefinition& definition);

[[nodiscard]] std::string serialize_alias_file(
    FactoryId factory,
    std::span<const AliasDefinition> aliases
);

[[nodiscard]] std::string starter_file_text(FactoryId factory);

[[nodiscard]] command_registry::Action materialize(const AliasDefinition& definition);

void register_factory_commands(command_registry::Registry& registry);

void load_alias_commands(command_registry::Registry& registry);

[[nodiscard]] bool write_alias_file(
    const std::filesystem::path& path,
    std::string_view contents,
    bool replace_existing
);

[[nodiscard]] int seed_missing_alias_files(const std::filesystem::path& directory);

} // namespace journal::factories
