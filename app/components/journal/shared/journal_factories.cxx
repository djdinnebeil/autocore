module;

#include <Windows.h>

module journal_factories;

import std;
import auto_core.core.console;
import journal_data_directory;
import auto_core.core.thread;
import command_registry;
import journal_component;
import journal_protocol;

namespace journal::factories {

namespace {

std::mutex prompt_mutex;

std::string_view trim(std::string_view value) {
    const std::size_t first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const std::size_t last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

constexpr FactoryDescriptor factory_descriptors[] {
    {
        .id = FactoryId::print_choice,
        .name = "make_print_choice",
        .menu_label = "Print choice",
        .filename = "make_print_choice.list",
        .autocomplete = R"(make_print_choice("", false))",
    },
    {
        .id = FactoryId::insert_text,
        .name = "print_and_insert_into_journal",
        .menu_label = "Insert text",
        .filename = "print_and_insert_into_journal.list",
        .autocomplete = R"(print_and_insert_into_journal(""))",
    },
    {
        .id = FactoryId::roll_dice,
        .name = "roll_dice",
        .menu_label = "Roll dice",
        .filename = "roll_dice.list",
        .autocomplete = "roll_dice(1, 6)",
    },
};

constexpr Field print_choice_fields[] {
    {.name = "name", .prompt = "Name", .kind = FieldKind::text},
    {
        .name = "lower_bound",
        .prompt = "Lower bound",
        .kind = FieldKind::integer,
        .optional = true,
        .default_integer = 1,
    },
};

constexpr Field insert_text_fields[] {
    {.name = "text", .prompt = "Text", .kind = FieldKind::text},
};

constexpr Field roll_dice_fields[] {
    {
        .name = "number_of_dice",
        .prompt = "Number of dice",
        .kind = FieldKind::integer,
    },
    {
        .name = "number_of_sides",
        .prompt = "Number of sides",
        .kind = FieldKind::integer,
    },
};

constexpr int minimum_dice = 1;
constexpr int maximum_dice = 100;
constexpr int minimum_sides = 2;
constexpr int maximum_sides = 1000;

int random_number(int low, int high) {
    static thread_local std::mt19937 engine {std::random_device {}()};
    return std::uniform_int_distribution<int> {low, high}(engine);
}

std::optional<int> prompt_for_upper_choice() {
    const auto target_window = ac::console::focus_for_prompt_via_winkey();
    if (!target_window) {
        journal_component().log_print(
            ac::console::error_message(target_window.error())
        );
        return std::nullopt;
    }

    journal_component().printnl("Enter number of choices: ");
    std::string value;
    std::getline(std::cin, value);

    int upper = 2;
    if (!value.empty()) {
        const auto parsed = std::from_chars(
            value.data(),
            value.data() + value.size(),
            upper
        );
        if (parsed.ec != std::errc {} ||
            parsed.ptr != value.data() + value.size()) {
            upper = 2;
        }
    }

    upper = (std::max)(upper, 1);
    journal_component().log_main("{}", upper);
    SetForegroundWindow(static_cast<HWND>(*target_window));
    return upper;
}

void run_choice(std::string name, int lower_bound) {
    const std::scoped_lock prompt_lock {prompt_mutex};
    const auto upper_choice = prompt_for_upper_choice();
    if (!upper_choice) {
        return;
    }

    const int upper = *upper_choice < lower_bound
        ? lower_bound + 1
        : *upper_choice;
    journal_component().print_and_insert(std::format(
        "{} selects {}.",
        name,
        random_number(lower_bound, upper)
    ));
}

void start_choice(std::string name, int lower_bound) {
    std::thread worker([
        name = std::move(name),
        lower_bound
    ] {
        ac::thread::run_with_exception_handling(
            [name, lower_bound] {
                run_choice(name, lower_bound);
            },
            journal_component()
        );
    });
    worker.detach();
}

std::optional<int> parse_int(std::string_view value) {
    if (value.empty()) {
        return std::nullopt;
    }
    int parsed = 0;
    const auto result = std::from_chars(
        value.data(),
        value.data() + value.size(),
        parsed
    );
    if (result.ec != std::errc {} ||
        result.ptr != value.data() + value.size()) {
        return std::nullopt;
    }
    return parsed;
}

std::expected<std::string, std::string> parse_quoted(std::string_view& input) {
    input = trim(input);
    if (input.empty() || input.front() != '"') {
        return std::unexpected("A quoted string is required");
    }

    std::string text;
    for (std::size_t index = 1; index < input.size(); ++index) {
        const char character = input[index];
        if (character == '\\') {
            if (index + 1 >= input.size()) {
                return std::unexpected("Invalid string escape");
            }
            const char escaped = input[++index];
            if (escaped != '\\' && escaped != '"') {
                return std::unexpected("Invalid string escape");
            }
            text.push_back(escaped);
            continue;
        }
        if (character == '"') {
            input.remove_prefix(index + 1);
            return text;
        }
        text.push_back(character);
    }
    return std::unexpected("Unclosed quoted string");
}

std::string quote_text(std::string_view text) {
    std::string quoted;
    quoted.push_back('"');
    for (const char character : text) {
        if (character == '\\' || character == '"') {
            quoted.push_back('\\');
        }
        quoted.push_back(character);
    }
    quoted.push_back('"');
    return quoted;
}

std::expected<std::string_view, std::string> take_token(std::string_view& input) {
    input = trim(input);
    if (input.empty()) {
        return std::unexpected("Missing argument");
    }
    if (input.front() == ',') {
        return std::unexpected("Missing argument");
    }
    const std::size_t comma = input.find(',');
    const std::string_view token = comma == std::string_view::npos
        ? input
        : trim(input.substr(0, comma));
    input = comma == std::string_view::npos
        ? std::string_view {}
        : input.substr(comma + 1);
    if (token.empty()) {
        return std::unexpected("Missing argument");
    }
    return token;
}

std::expected<FactoryArguments, std::string> parse_print_choice_arguments(
    std::string_view arguments
) {
    auto name = parse_quoted(arguments);
    if (!name) {
        return std::unexpected(name.error());
    }
    arguments = trim(arguments);

    int lower_bound = 1;
    if (!arguments.empty()) {
        if (arguments.front() != ',') {
            return std::unexpected("Unexpected text after the name");
        }
        arguments.remove_prefix(1);
        auto first = take_token(arguments);
        if (!first) {
            return std::unexpected(first.error());
        }
        if (!trim(arguments).empty()) {
            return std::unexpected("Too many arguments");
        }
        if (*first == "true") {
            lower_bound = 0;
        }
        else if (*first == "false") {
            lower_bound = 1;
        }
        else if (const auto parsed = parse_int(*first)) {
            lower_bound = *parsed;
        }
        else {
            return std::unexpected("Lower bound must be an integer, true, or false");
        }
    }

    return PrintChoiceArguments {
        std::move(*name),
        lower_bound,
    };
}

std::expected<FactoryArguments, std::string> parse_insert_arguments(
    std::string_view arguments
) {
    auto text = parse_quoted(arguments);
    if (!text) {
        return std::unexpected(text.error());
    }
    if (!trim(arguments).empty()) {
        return std::unexpected("Too many arguments");
    }
    return InsertTextArguments {std::move(*text)};
}

std::optional<std::string> dice_bounds(int number_of_dice, int number_of_sides) {
    if (number_of_dice < minimum_dice || number_of_dice > maximum_dice) {
        return std::format(
            "Number of dice must be an integer from {} through {}",
            minimum_dice,
            maximum_dice
        );
    }
    if (number_of_sides < minimum_sides || number_of_sides > maximum_sides) {
        return std::format(
            "Number of sides must be an integer from {} through {}",
            minimum_sides,
            maximum_sides
        );
    }
    return std::nullopt;
}

std::expected<FactoryArguments, std::string> parse_dice_arguments(
    std::string_view arguments
) {
    auto dice_token = take_token(arguments);
    if (!dice_token) {
        return std::unexpected(dice_token.error());
    }
    auto sides_token = take_token(arguments);
    if (!sides_token) {
        return std::unexpected(sides_token.error());
    }
    if (!trim(arguments).empty()) {
        return std::unexpected("Too many arguments");
    }

    const auto number_of_dice = parse_int(*dice_token);
    const auto number_of_sides = parse_int(*sides_token);
    if (!number_of_dice) {
        return std::unexpected("Number of dice must be an integer");
    }
    if (!number_of_sides) {
        return std::unexpected("Number of sides must be an integer");
    }
    if (const auto error = dice_bounds(*number_of_dice, *number_of_sides)) {
        return std::unexpected(*error);
    }
    return DiceArguments {*number_of_dice, *number_of_sides};
}

std::expected<FactoryArguments, std::string> parse_arguments(
    FactoryId factory,
    std::string_view arguments
) {
    switch (factory) {
    case FactoryId::print_choice:
        return parse_print_choice_arguments(arguments);
    case FactoryId::insert_text:
        return parse_insert_arguments(arguments);
    case FactoryId::roll_dice:
        return parse_dice_arguments(arguments);
    }
    return std::unexpected("Unknown factory");
}

std::expected<int, std::string> integer_field(
    const Field& field,
    std::string_view text,
    bool bool_shorthand
) {
    text = trim(text);
    if (text.empty()) {
        if (!field.optional) {
            return std::unexpected(std::format("{} is required", field.prompt));
        }
        return field.default_integer;
    }
    if (bool_shorthand && text == "true") {
        return 0;
    }
    if (bool_shorthand && text == "false") {
        return 1;
    }
    const auto parsed = parse_int(text);
    if (!parsed) {
        return std::unexpected(std::format("{} must be an integer", field.prompt));
    }
    return *parsed;
}

const PrintChoiceArguments* print_choice(const AliasDefinition& definition) {
    return std::get_if<PrintChoiceArguments>(&definition.arguments);
}

const InsertTextArguments* insert_text(const AliasDefinition& definition) {
    return std::get_if<InsertTextArguments>(&definition.arguments);
}

const DiceArguments* dice(const AliasDefinition& definition) {
    return std::get_if<DiceArguments>(&definition.arguments);
}

void roll_and_insert(int number_of_dice, int number_of_sides) {
    std::string output = std::format("{}d{}: ", number_of_dice, number_of_sides);
    int total = 0;
    for (int index = 0; index < number_of_dice; ++index) {
        const int roll = random_number(1, number_of_sides);
        total += roll;
        if (index != 0) {
            output += " + ";
        }
        output += std::to_string(roll);
    }
    output += std::format(" = {}", total);
    journal_component().print_and_insert(output);
}

bool arguments_match(const AliasDefinition& definition) {
    switch (definition.factory) {
    case FactoryId::print_choice:
        return print_choice(definition) != nullptr;
    case FactoryId::insert_text:
        return insert_text(definition) != nullptr;
    case FactoryId::roll_dice:
        return dice(definition) != nullptr;
    }
    return false;
}

std::vector<AliasDefinition> starter_definitions(FactoryId factory) {
    if (factory == FactoryId::print_choice) {
        return {
            {
                "print_auto_core_choice",
                factory,
                PrintChoiceArguments {"Auto Core", 1},
            },
        };
    }
    if (factory == FactoryId::insert_text) {
        return {
            {
                "print_hello_journal",
                factory,
                InsertTextArguments {"Hello Journal!"},
            },
        };
    }
    return {
        {
            "r7d8",
            factory,
            DiceArguments {7, 8},
        },
    };
}

} // namespace

std::span<const FactoryDescriptor> factories() {
    return factory_descriptors;
}

const FactoryDescriptor* find_factory(FactoryId id) {
    for (const FactoryDescriptor& descriptor : factory_descriptors) {
        if (descriptor.id == id) {
            return &descriptor;
        }
    }
    return nullptr;
}

const FactoryDescriptor* find_factory(std::string_view name) {
    for (const FactoryDescriptor& descriptor : factory_descriptors) {
        if (descriptor.name == name) {
            return &descriptor;
        }
    }
    return nullptr;
}

std::span<const Field> fields(FactoryId id) {
    switch (id) {
    case FactoryId::print_choice:
        return print_choice_fields;
    case FactoryId::insert_text:
        return insert_text_fields;
    case FactoryId::roll_dice:
        return roll_dice_fields;
    }
    return {};
}

std::filesystem::path alias_path(
    const std::filesystem::path& directory,
    FactoryId id
) {
    const FactoryDescriptor* descriptor = find_factory(id);
    if (!descriptor) {
        return directory / "alias.list";
    }
    return directory / std::string {descriptor->filename};
}

bool is_alias_name(std::string_view name) {
    if (name.empty()) {
        return false;
    }
    const auto head = static_cast<unsigned char>(name.front());
    if (!(std::isalpha(head) || name.front() == '_')) {
        return false;
    }
    for (const char character : name) {
        const auto value = static_cast<unsigned char>(character);
        if (!(std::isalnum(value) || character == '_')) {
            return false;
        }
    }
    return true;
}

bool is_reserved_alias_name(std::string_view name) {
    if (name == ::print_extended_timestamp.name ||
        name == ::print_episode_title.name ||
        name == ::save_file_and_create_new_file.name) {
        return true;
    }
    return find_factory(name) != nullptr;
}

std::expected<AliasDefinition, std::string> parse_alias_line(
    std::string_view line,
    FactoryId file_factory
) {
    const std::string_view trimmed = trim(line);
    const auto equals = trimmed.find('=');
    if (equals == std::string_view::npos) {
        return std::unexpected(std::format(
            "Invalid journal alias line format: {}",
            trimmed
        ));
    }

    const std::string alias {trim(trimmed.substr(0, equals))};
    const std::string_view expression = trim(trimmed.substr(equals + 1));
    if (!is_alias_name(alias) || expression.empty()) {
        return std::unexpected(std::format(
            "Invalid journal alias line format: {}",
            trimmed
        ));
    }

    const FactoryDescriptor* descriptor = find_factory(file_factory);
    if (!descriptor) {
        return std::unexpected("Unknown factory");
    }

    const auto opening = expression.find('(');
    if (opening == std::string_view::npos || expression.back() != ')') {
        return std::unexpected(std::format(
            "Invalid journal alias line format: {}",
            trimmed
        ));
    }

    const std::string_view factory_name = trim(expression.substr(0, opening));
    if (factory_name != descriptor->name) {
        return std::unexpected(std::format(
            "Journal alias '{}' skipped because '{}' is not {}",
            alias,
            factory_name,
            descriptor->name
        ));
    }

    const std::string_view arguments = expression.substr(
        opening + 1,
        expression.size() - opening - 2
    );
    auto parsed = parse_arguments(file_factory, arguments);
    if (!parsed) {
        return std::unexpected(std::format(
            "Journal alias '{}' skipped because {}",
            alias,
            parsed.error()
        ));
    }

    return AliasDefinition {
        alias,
        file_factory,
        std::move(*parsed),
    };
}

AliasFile parse_alias_file(std::string_view text, FactoryId file_factory) {
    AliasFile result;
    std::size_t offset = 0;
    while (offset <= text.size()) {
        const std::size_t end = text.find('\n', offset);
        std::string_view line = text.substr(
            offset,
            end == std::string_view::npos ? std::string_view::npos : end - offset
        );
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }
        offset = end == std::string_view::npos ? text.size() + 1 : end + 1;

        const std::string_view trimmed = trim(line);
        if (trimmed.empty()) {
            if (end == std::string_view::npos) {
                break;
            }
            continue;
        }

        auto parsed = parse_alias_line(line, file_factory);
        if (!parsed) {
            result.issues.push_back(parsed.error());
            if (end == std::string_view::npos) {
                break;
            }
            continue;
        }

        const bool duplicate = std::ranges::any_of(
            result.aliases,
            [&](const AliasDefinition& existing) {
                return existing.alias == parsed->alias;
            }
        );
        if (duplicate) {
            result.issues.push_back(std::format(
                "Journal alias '{}' skipped because that command is already defined",
                parsed->alias
            ));
        }
        else {
            result.aliases.push_back(std::move(*parsed));
        }

        if (end == std::string_view::npos) {
            break;
        }
    }
    return result;
}

ReadAliasFile read_alias_file(
    const std::filesystem::path& path,
    FactoryId file_factory
) {
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error) {
        ReadAliasFile result;
        result.file.issues.push_back(std::format(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        ));
        return result;
    }
    if (!exists) {
        return {.missing = true};
    }

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        ReadAliasFile result;
        result.file.issues.push_back(std::format(
            "Failed to read {}",
            path.string()
        ));
        return result;
    }
    const std::string text {
        std::istreambuf_iterator<char> {input},
        std::istreambuf_iterator<char> {}
    };
    return {.missing = false, .file = parse_alias_file(text, file_factory)};
}

std::optional<std::string> validate(const AliasDefinition& definition) {
    if (!definition.alias.empty() && !is_alias_name(definition.alias)) {
        return "Invalid journal alias name";
    }
    if (!arguments_match(definition)) {
        return "Factory arguments do not match the factory";
    }
    if (const DiceArguments* rolled = dice(definition)) {
        return dice_bounds(rolled->number_of_dice, rolled->number_of_sides);
    }
    return std::nullopt;
}

std::optional<std::string> rejection_reason(
    const AliasDefinition& candidate,
    std::span<const AliasDefinition> already_loaded
) {
    if (const auto error = validate(candidate)) {
        return error;
    }
    if (!is_alias_name(candidate.alias)) {
        return "Invalid journal alias name";
    }
    if (is_reserved_alias_name(candidate.alias)) {
        return std::format(
            "Journal alias '{}' skipped because that command is reserved",
            candidate.alias
        );
    }
    const bool duplicate = std::ranges::any_of(
        already_loaded,
        [&](const AliasDefinition& existing) {
            return existing.alias == candidate.alias;
        }
    );
    if (duplicate) {
        return std::format(
            "Journal alias '{}' skipped because that command is already defined",
            candidate.alias
        );
    }
    return std::nullopt;
}

std::string field_value(
    const AliasDefinition& definition,
    std::string_view field_name
) {
    if (const PrintChoiceArguments* choice = print_choice(definition)) {
        if (field_name == "name") {
            return choice->name;
        }
        if (field_name == "lower_bound") {
            return std::to_string(choice->lower_bound);
        }
    }
    if (const InsertTextArguments* insert = insert_text(definition)) {
        if (field_name == "text") {
            return insert->text;
        }
    }
    if (const DiceArguments* rolled = dice(definition)) {
        if (field_name == "number_of_dice") {
            return std::to_string(rolled->number_of_dice);
        }
        if (field_name == "number_of_sides") {
            return std::to_string(rolled->number_of_sides);
        }
    }
    return {};
}

std::expected<AliasDefinition, std::string> alias_from_fields(
    FactoryId factory,
    std::string alias,
    std::span<const std::string> values
) {
    const std::span<const Field> schema = fields(factory);
    if (values.size() != schema.size()) {
        return std::unexpected("Missing factory argument");
    }

    AliasDefinition definition;
    definition.alias = std::move(alias);
    definition.factory = factory;

    if (factory == FactoryId::print_choice) {
        if (trim(values[0]).empty()) {
            return std::unexpected("Name is required");
        }
        auto lower = integer_field(schema[1], values[1], true);
        if (!lower) {
            return std::unexpected(lower.error());
        }
        definition.arguments = PrintChoiceArguments {
            std::string {trim(values[0])},
            *lower,
        };
    }
    else if (factory == FactoryId::roll_dice) {
        auto number_of_dice = integer_field(schema[0], values[0], false);
        if (!number_of_dice) {
            return std::unexpected(number_of_dice.error());
        }
        auto number_of_sides = integer_field(schema[1], values[1], false);
        if (!number_of_sides) {
            return std::unexpected(number_of_sides.error());
        }
        definition.arguments = DiceArguments {*number_of_dice, *number_of_sides};
    }
    else {
        definition.arguments = InsertTextArguments {std::string {values[0]}};
    }

    if (const auto error = validate(definition)) {
        return std::unexpected(*error);
    }
    return definition;
}

std::string serialize_alias(const AliasDefinition& definition) {
    const FactoryDescriptor* descriptor = find_factory(definition.factory);
    const std::string_view factory_name = descriptor ? descriptor->name : "factory";
    if (const PrintChoiceArguments* choice = print_choice(definition)) {
        return std::format(
            "{} = {}({}, {})",
            definition.alias,
            factory_name,
            quote_text(choice->name),
            choice->lower_bound
        );
    }
    if (const InsertTextArguments* insert = insert_text(definition)) {
        return std::format(
            "{} = {}({})",
            definition.alias,
            factory_name,
            quote_text(insert->text)
        );
    }
    if (const DiceArguments* rolled = dice(definition)) {
        return std::format(
            "{} = {}({}, {})",
            definition.alias,
            factory_name,
            rolled->number_of_dice,
            rolled->number_of_sides
        );
    }
    return {};
}

std::string serialize_alias_file(
    FactoryId,
    std::span<const AliasDefinition> aliases
) {
    if (aliases.empty()) {
        return "\n";
    }
    std::string text;
    for (const AliasDefinition& definition : aliases) {
        text += serialize_alias(definition);
        text.push_back('\n');
    }
    return text;
}

std::string starter_file_text(FactoryId factory) {
    const std::vector<AliasDefinition> aliases = starter_definitions(factory);
    return serialize_alias_file(factory, aliases);
}

command_registry::Action materialize(const AliasDefinition& definition) {
    if (validate(definition)) {
        return {};
    }
    if (const PrintChoiceArguments* choice = print_choice(definition)) {
        return [
            name = choice->name,
            lower_bound = choice->lower_bound
        ] {
            start_choice(name, lower_bound);
        };
    }
    if (const InsertTextArguments* insert = insert_text(definition)) {
        return [text = insert->text] {
            journal_component().print_and_insert(text);
        };
    }
    if (const DiceArguments* rolled = dice(definition)) {
        return [
            number_of_dice = rolled->number_of_dice,
            number_of_sides = rolled->number_of_sides
        ] {
            roll_and_insert(number_of_dice, number_of_sides);
        };
    }
    return {};
}

void register_factory_commands(command_registry::Registry& registry) {
    for (const FactoryDescriptor& descriptor : factory_descriptors) {
        registry.add_factory(
            std::string {descriptor.name},
            [id = descriptor.id](std::string_view arguments) {
                auto parsed = parse_arguments(id, arguments);
                if (!parsed) {
                    return command_registry::Action {};
                }
                AliasDefinition definition;
                definition.factory = id;
                definition.arguments = std::move(*parsed);
                return materialize(definition);
            },
            std::string {descriptor.autocomplete}
        );
    }
}

void load_alias_commands(command_registry::Registry& registry) {
    const std::filesystem::path directory = journal::data_directory();
    std::vector<AliasDefinition> accepted;
    for (const FactoryDescriptor& descriptor : factory_descriptors) {
        const ReadAliasFile loaded = read_alias_file(
            alias_path(directory, descriptor.id),
            descriptor.id
        );
        if (loaded.missing) {
            continue;
        }
        for (const std::string& issue : loaded.file.issues) {
            journal_component().log_print("{}", issue);
        }
        for (const AliasDefinition& definition : loaded.file.aliases) {
            if (const auto reason = rejection_reason(definition, accepted)) {
                journal_component().log_print("{}", *reason);
                continue;
            }
            auto action = materialize(definition);
            if (!action) {
                journal_component().log_print(
                    "Journal alias '{}' skipped because it could not be loaded",
                    definition.alias
                );
                continue;
            }
            registry.add(definition.alias, std::move(action));
            accepted.push_back(definition);
        }
    }
}

bool write_alias_file(
    const std::filesystem::path& path,
    std::string_view contents,
    bool replace_existing
) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        return false;
    }

    const bool exists = std::filesystem::exists(path, error);
    if (error) {
        return false;
    }
    if (exists && !replace_existing) {
        return true;
    }

    const auto temporary = std::filesystem::path {path.wstring() + L".new"};
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) {
            return false;
        }
        output.write(
            contents.data(),
            static_cast<std::streamsize>(contents.size())
        );
        output.flush();
        if (!output) {
            output.close();
            ::DeleteFileW(temporary.c_str());
            return false;
        }
    }

    const DWORD flags = replace_existing
        ? MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
        : MOVEFILE_WRITE_THROUGH;
    if (!::MoveFileExW(temporary.c_str(), path.c_str(), flags)) {
        if (!replace_existing && std::filesystem::exists(path)) {
            ::DeleteFileW(temporary.c_str());
            return true;
        }
        ::DeleteFileW(temporary.c_str());
        return false;
    }
    return true;
}

int seed_missing_alias_files(const std::filesystem::path& directory) {
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) {
        return 1;
    }

    for (const FactoryDescriptor& descriptor : factory_descriptors) {
        const std::filesystem::path path = alias_path(directory, descriptor.id);
        const bool exists = std::filesystem::exists(path, error);
        if (error) {
            return 1;
        }
        if (exists) {
            continue;
        }
        if (!write_alias_file(path, starter_file_text(descriptor.id), false)) {
            return 1;
        }
    }
    return 0;
}

} // namespace journal::factories
