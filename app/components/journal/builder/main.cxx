import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import journal_data_directory;
import journal_factories;
import components_editor_request;

import <iostream>;
import auto_core.core.shell;

namespace {

ac::Component journal_builder {
    "journal_builder",
    ac::logging::config::LoggingScope {"journal"}
};

std::string_view trim(std::string_view value) {
    const std::size_t first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const std::size_t last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

std::optional<std::string> prompt_default(
    std::string_view label,
    std::string_view current
) {
    std::cout << label << " [" << current << "]: ";
    std::string input;
    if (!std::getline(std::cin, input)) {
        return std::nullopt;
    }
    const auto value = trim(input);
    if (value == "cancel") {
        journal_builder.log_print("Cancelled.");
        return std::nullopt;
    }
    if (value.empty()) {
        return std::string {current};
    }
    return std::string {value};
}

std::optional<std::string> prompt_line(std::string_view label) {
    std::cout << label;
    std::string input;
    if (!std::getline(std::cin, input)) {
        return std::nullopt;
    }
    return std::string {trim(input)};
}

void print_issues(const std::vector<std::string>& issues) {
    for (const std::string& issue : issues) {
        std::cout << issue << '\n';
    }
}

journal::factories::ReadAliasFile load_factory(
    journal::factories::FactoryId id
) {
    return journal::factories::read_alias_file(
        journal::factories::alias_path(journal::data_directory(), id),
        id
    );
}

std::vector<journal::factories::AliasDefinition> other_aliases(
    journal::factories::FactoryId id,
    std::string_view replaced
) {
    std::vector<journal::factories::AliasDefinition> aliases;
    for (const journal::factories::FactoryDescriptor& descriptor :
         journal::factories::factories()) {
        const journal::factories::ReadAliasFile loaded = load_factory(descriptor.id);
        for (const journal::factories::AliasDefinition& definition :
             loaded.file.aliases) {
            if (descriptor.id == id && definition.alias == replaced) {
                continue;
            }
            aliases.push_back(definition);
        }
    }
    return aliases;
}

bool save_aliases(
    journal::factories::FactoryId id,
    std::span<const journal::factories::AliasDefinition> aliases
) {
    const std::filesystem::path path = journal::factories::alias_path(
        journal::data_directory(),
        id
    );
    const std::string text = journal::factories::serialize_alias_file(id, aliases);
    if (!journal::factories::write_alias_file(path, text, true)) {
        journal_builder.log_print("Failed to write {}", path.string());
        std::cout << "Could not write " << path.string() << ".\n";
        return false;
    }
    journal_builder.log_print("Wrote {}", path.string());
    std::cout << "Wrote " << path.string() << ".\n";
    std::cout << "Restart Journal before the catalog changes.\n";
    return true;
}

bool add_alias(const journal::factories::FactoryDescriptor& descriptor) {
    const journal::factories::ReadAliasFile loaded = load_factory(descriptor.id);
    print_issues(loaded.file.issues);
    if (!loaded.file.issues.empty()) {
        std::cout << "Invalid lines are omitted when this file is saved.\n";
    }

    auto alias_name = prompt_line("Alias name: ");
    if (!alias_name) {
        return false;
    }
    std::vector<std::string> values;
    values.reserve(journal::factories::fields(descriptor.id).size());
    for (const journal::factories::Field& field :
         journal::factories::fields(descriptor.id)) {
        const std::string label = field.optional
            ? std::format(
                "{} [{}]: ",
                field.prompt,
                field.default_integer
            )
            : std::format("{}: ", field.prompt);
        auto value = prompt_line(label);
        if (!value) {
            return false;
        }
        if (value->empty() && field.optional) {
            value = std::to_string(field.default_integer);
        }
        values.push_back(std::move(*value));
    }

    auto definition = journal::factories::alias_from_fields(
        descriptor.id,
        *alias_name,
        values
    );
    if (!definition) {
        std::cout << definition.error() << '\n';
        return true;
    }
    if (const auto reason = journal::factories::rejection_reason(
            *definition,
            other_aliases(descriptor.id, {})
        )) {
        std::cout << *reason << '\n';
        return true;
    }

    auto aliases = loaded.file.aliases;
    aliases.push_back(std::move(*definition));
    return save_aliases(descriptor.id, aliases);
}

bool edit_alias(const journal::factories::FactoryDescriptor& descriptor) {
    const journal::factories::ReadAliasFile loaded = load_factory(descriptor.id);
    print_issues(loaded.file.issues);
    if (loaded.file.aliases.empty()) {
        std::cout << "No aliases in this file.\n";
        return true;
    }
    if (!loaded.file.issues.empty()) {
        std::cout << "Invalid lines are omitted when this file is saved.\n";
    }

    for (std::size_t index = 0; index < loaded.file.aliases.size(); ++index) {
        std::cout << index + 1 << ". " << loaded.file.aliases[index].alias << '\n';
    }
    auto choice = prompt_line("Alias number: ");
    if (!choice) {
        return false;
    }
    int number = 0;
    const auto parsed = std::from_chars(
        choice->data(),
        choice->data() + choice->size(),
        number
    );
    if (parsed.ec != std::errc {} ||
        parsed.ptr != choice->data() + choice->size() ||
        number < 1 ||
        static_cast<std::size_t>(number) > loaded.file.aliases.size()) {
        std::cout << "Enter an alias number from the list.\n";
        return true;
    }

    const journal::factories::AliasDefinition& current =
        loaded.file.aliases[static_cast<std::size_t>(number - 1)];
    auto alias_name = prompt_line(std::format("Alias name [{}]: ", current.alias));
    if (!alias_name) {
        return false;
    }
    if (alias_name->empty()) {
        *alias_name = current.alias;
    }

    std::vector<std::string> values;
    for (const journal::factories::Field& field :
         journal::factories::fields(descriptor.id)) {
        const std::string shown = journal::factories::field_value(current, field.name);
        auto value = prompt_line(std::format("{} [{}]: ", field.prompt, shown));
        if (!value) {
            return false;
        }
        if (value->empty()) {
            *value = shown;
        }
        values.push_back(std::move(*value));
    }

    auto definition = journal::factories::alias_from_fields(
        descriptor.id,
        *alias_name,
        values
    );
    if (!definition) {
        std::cout << definition.error() << '\n';
        return true;
    }
    if (const auto reason = journal::factories::rejection_reason(
            *definition,
            other_aliases(descriptor.id, current.alias)
        )) {
        std::cout << *reason << '\n';
        return true;
    }

    auto aliases = loaded.file.aliases;
    aliases[static_cast<std::size_t>(number - 1)] = std::move(*definition);
    return save_aliases(descriptor.id, aliases);
}

bool delete_alias(const journal::factories::FactoryDescriptor& descriptor) {
    const journal::factories::ReadAliasFile loaded = load_factory(descriptor.id);
    print_issues(loaded.file.issues);
    if (loaded.file.aliases.empty()) {
        std::cout << "No aliases in this file.\n";
        return true;
    }

    for (std::size_t index = 0; index < loaded.file.aliases.size(); ++index) {
        std::cout << index + 1 << ". " << loaded.file.aliases[index].alias << '\n';
    }
    auto choice = prompt_line("Alias number: ");
    if (!choice) {
        return false;
    }
    int number = 0;
    const auto parsed = std::from_chars(
        choice->data(),
        choice->data() + choice->size(),
        number
    );
    if (parsed.ec != std::errc {} ||
        parsed.ptr != choice->data() + choice->size() ||
        number < 1 ||
        static_cast<std::size_t>(number) > loaded.file.aliases.size()) {
        std::cout << "Enter an alias number from the list.\n";
        return true;
    }

    const std::string& name =
        loaded.file.aliases[static_cast<std::size_t>(number - 1)].alias;
    auto confirm = prompt_line(std::format("Delete {}? [y/N]: ", name));
    if (!confirm) {
        return false;
    }
    if (*confirm != "y" && *confirm != "Y") {
        return true;
    }

    auto aliases = loaded.file.aliases;
    aliases.erase(aliases.begin() + (number - 1));
    return save_aliases(descriptor.id, aliases);
}

bool factory_menu(const journal::factories::FactoryDescriptor& descriptor) {
    while (true) {
        std::cout
            << '\n'
            << descriptor.menu_label << " (" << descriptor.name << ")\n"
            << "  1. Add alias\n"
            << "  2. Edit alias\n"
            << "  3. Delete alias\n"
            << "  4. Back\n"
            << "> ";
        std::string choice;
        if (!std::getline(std::cin, choice)) {
            return false;
        }
        choice = std::string {trim(choice)};
        if (choice == "1") {
            if (!add_alias(descriptor)) {
                return false;
            }
        }
        else if (choice == "2") {
            if (!edit_alias(descriptor)) {
                return false;
            }
        }
        else if (choice == "3") {
            if (!delete_alias(descriptor)) {
                return false;
            }
        }
        else if (choice == "4") {
            return true;
        }
    }
}

int run_menu() {
    const auto catalog = journal::factories::factories();
    while (true) {
        std::cout << "\nJournal Builder\n";
        for (std::size_t index = 0; index < catalog.size(); ++index) {
            std::cout
                << "  " << index + 1 << ". " << catalog[index].menu_label
                << " (" << catalog[index].name << ")\n";
        }
        std::cout << "  " << catalog.size() + 1 << ". Exit\n> ";
        std::string choice;
        if (!std::getline(std::cin, choice)) {
            return 0;
        }
        choice = std::string {trim(choice)};
        int number = 0;
        const auto parsed = std::from_chars(
            choice.data(),
            choice.data() + choice.size(),
            number
        );
        if (parsed.ec != std::errc {} ||
            parsed.ptr != choice.data() + choice.size() ||
            number < 1 ||
            static_cast<std::size_t>(number) > catalog.size() + 1) {
            continue;
        }
        if (static_cast<std::size_t>(number) == catalog.size() + 1) {
            return 0;
        }
        if (!factory_menu(catalog[static_cast<std::size_t>(number - 1)])) {
            return 0;
        }
    }
}

int initialize_missing_factory(
    const journal::factories::FactoryDescriptor& descriptor
) {
    const std::filesystem::path path = journal::factories::alias_path(
        journal::data_directory(),
        descriptor.id
    );
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error) {
        journal_builder.log_print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return 1;
    }
    namespace req = ac::config::components_request;
    if (exists) {
        req::log_initialization_skipped(journal_builder, path.string());
        return 0;
    }

    const std::string starter = journal::factories::starter_file_text(descriptor.id);
    const journal::factories::AliasFile parsed =
        journal::factories::parse_alias_file(starter, descriptor.id);
    if (parsed.aliases.empty() || !parsed.issues.empty()) {
        journal_builder.log_print(
            "The starter for {} is not a valid alias file.",
            path.string()
        );
        return 1;
    }

    std::cout << descriptor.filename << '\n' << starter;
    std::vector<journal::factories::AliasDefinition> aliases;
    for (const journal::factories::AliasDefinition& starter_alias : parsed.aliases) {
        while (true) {
            const auto alias_name = prompt_default("Alias name", starter_alias.alias);
            if (!alias_name) {
                return 1;
            }
            std::vector<std::string> values;
            bool cancelled = false;
            for (const journal::factories::Field& field :
                 journal::factories::fields(descriptor.id)) {
                const auto value = prompt_default(
                    field.prompt,
                    journal::factories::field_value(starter_alias, field.name)
                );
                if (!value) {
                    cancelled = true;
                    break;
                }
                values.push_back(*value);
            }
            if (cancelled) {
                return 1;
            }

            auto definition = journal::factories::alias_from_fields(
                descriptor.id,
                *alias_name,
                values
            );
            if (!definition) {
                std::cout << definition.error() << '\n';
                continue;
            }
            std::vector<journal::factories::AliasDefinition> loaded = aliases;
            for (const journal::factories::AliasDefinition& existing :
                 other_aliases(descriptor.id, {})) {
                loaded.push_back(existing);
            }
            if (const auto reason = journal::factories::rejection_reason(
                    *definition,
                    loaded
                )) {
                std::cout << *reason << '\n';
                continue;
            }
            aliases.push_back(std::move(*definition));
            break;
        }
    }

    if (!save_aliases(descriptor.id, aliases)) {
        return 1;
    }
    return 0;
}

int run_init() {
    for (const journal::factories::FactoryDescriptor& descriptor :
         journal::factories::factories()) {
        if (const int code = initialize_missing_factory(descriptor); code != 0) {
            return code;
        }
    }
    return 0;
}

int run_seed() {
    try {
        const std::filesystem::path directory = journal::data_directory();
        if (journal::factories::seed_missing_alias_files(directory) != 0) {
            journal_builder.log_print("Failed to create journal alias files.");
            return 1;
        }
        return 0;
    }
    catch (const std::exception& error) {
        journal_builder.log_print("{}", error.what());
        return 1;
    }
}

} // namespace

int main(int argc, char* argv[]) {
    std::setvbuf(stdin, nullptr, _IONBF, 0);
    ac::shell::set_process_app_user_model_id();
    journal_builder.log_main("journal_builder.exe started");
    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }
    namespace req = ac::config::components_request;
    req::log_config_request(journal_builder, *launch);
    try {
        if (launch->seed) {
            return run_seed();
        }
        if (launch->init) {
            return run_init();
        }
    }
    catch (const std::exception& error) {
        journal_builder.log_print("{}", error.what());
        return 1;
    }
    if (argc > 1) {
        std::cerr << "Usage: journal_builder.exe [--seed | --init]\n";
        return 1;
    }

    try {
        return run_menu();
    }
    catch (const std::exception& error) {
        journal_builder.log_print("{}", error.what());
        return 1;
    }
}
