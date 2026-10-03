#include "catch_amalgamated.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <variant>

import journal_factories;

namespace {

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {
        std::istreambuf_iterator<char> {input},
        std::istreambuf_iterator<char> {}
    };
}

const journal::factories::PrintChoiceArguments* choice(
    const journal::factories::AliasDefinition& definition
) {
    return std::get_if<journal::factories::PrintChoiceArguments>(
        &definition.arguments
    );
}

const journal::factories::InsertTextArguments* insert(
    const journal::factories::AliasDefinition& definition
) {
    return std::get_if<journal::factories::InsertTextArguments>(
        &definition.arguments
    );
}

} // namespace

TEST_CASE("Print choice aliases parse to structured arguments", "[journal][unit]") {
    const auto parsed = journal::factories::parse_alias_line(
        R"(print_Tabby_choice = make_print_choice("Tabby", 0))",
        journal::factories::FactoryId::print_choice
    );
    REQUIRE(parsed);
    CHECK(parsed->alias == "print_Tabby_choice");
    const auto* arguments = choice(*parsed);
    REQUIRE(arguments);
    CHECK(arguments->name == "Tabby");
    CHECK(arguments->lower_bound == 0);
    CHECK_FALSE(journal::factories::validate(*parsed));
}

TEST_CASE("Omitted print choice arguments use defaults", "[journal][unit]") {
    const auto parsed = journal::factories::parse_alias_line(
        R"(print_Tabby_choice = make_print_choice("Tabby"))",
        journal::factories::FactoryId::print_choice
    );
    REQUIRE(parsed);
    const auto* arguments = choice(*parsed);
    REQUIRE(arguments);
    CHECK(arguments->lower_bound == 1);
}

TEST_CASE("Boolean shorthand becomes an integer lower bound", "[journal][unit]") {
    const auto enabled = journal::factories::parse_alias_line(
        R"(print_example = make_print_choice("42nd", true))",
        journal::factories::FactoryId::print_choice
    );
    const auto disabled = journal::factories::parse_alias_line(
        R"(print_example = make_print_choice("42nd", false))",
        journal::factories::FactoryId::print_choice
    );
    REQUIRE(enabled);
    REQUIRE(disabled);
    CHECK(choice(*enabled)->lower_bound == 0);
    CHECK(choice(*disabled)->lower_bound == 1);
}

TEST_CASE("A third print choice argument is rejected", "[journal][unit]") {
    const auto parsed = journal::factories::parse_alias_line(
        R"(print_example = make_print_choice("Tabby", 0, 1))",
        journal::factories::FactoryId::print_choice
    );
    REQUIRE_FALSE(parsed);
    CHECK(parsed.error().find("Too many arguments") != std::string::npos);
}

TEST_CASE("Insert text keeps escaped quotes", "[journal][unit]") {
    const auto parsed = journal::factories::parse_alias_line(
        R"(print_quote = print_and_insert_into_journal("say \"hi\""))",
        journal::factories::FactoryId::insert_text
    );
    REQUIRE(parsed);
    REQUIRE(insert(*parsed));
    CHECK(insert(*parsed)->text == R"(say "hi")");
    CHECK(journal::factories::serialize_alias(*parsed).find(R"(\"hi\")") !=
        std::string::npos);
}

TEST_CASE("A bad escape is rejected", "[journal][unit]") {
    const auto parsed = journal::factories::parse_alias_line(
        R"(print_quote = print_and_insert_into_journal("bad\n"))",
        journal::factories::FactoryId::insert_text
    );
    REQUIRE_FALSE(parsed);
    CHECK(parsed.error().find("escape") != std::string::npos);
}

TEST_CASE("Blank lines are ignored and comment lines are invalid", "[journal][unit]") {
    const auto file = journal::factories::parse_alias_file(
        "\n"
        "; comment\n"
        "# hash\n"
        "print_one_is_selected = print_and_insert_into_journal(\"1 is selected.\")\n",
        journal::factories::FactoryId::insert_text
    );
    REQUIRE(file.aliases.size() == 1);
    REQUIRE(file.issues.size() == 2);
    CHECK(file.aliases[0].alias == "print_one_is_selected");

    const journal::factories::AliasDefinition aliases[] {file.aliases[0]};
    const std::string serialized = journal::factories::serialize_alias_file(
        journal::factories::FactoryId::insert_text,
        aliases
    );
    CHECK(serialized.starts_with("print_one_is_selected = "));
    CHECK(journal::factories::serialize_alias_file(
        journal::factories::FactoryId::roll_dice,
        {}
    ) == "\n");
}

TEST_CASE("A line for the wrong factory is rejected", "[journal][unit]") {
    const auto parsed = journal::factories::parse_alias_line(
        R"(print_one_is_selected = print_and_insert_into_journal("1 is selected."))",
        journal::factories::FactoryId::print_choice
    );
    REQUIRE_FALSE(parsed);
    CHECK(parsed.error().find("make_print_choice") != std::string::npos);
}

TEST_CASE("A duplicate alias in one file keeps the first", "[journal][unit]") {
    const auto file = journal::factories::parse_alias_file(
        "print_Tabby_choice = make_print_choice(\"Tabby\", 0)\n"
        "print_Tabby_choice = make_print_choice(\"Other\", 1)\n",
        journal::factories::FactoryId::print_choice
    );
    REQUIRE(file.aliases.size() == 1);
    REQUIRE(file.issues.size() == 1);
    CHECK(choice(file.aliases[0])->name == "Tabby");
}

TEST_CASE("The same alias in another factory file is rejected", "[journal][unit]") {
    const auto first = journal::factories::parse_alias_line(
        R"(print_shared = make_print_choice("Tabby", 0))",
        journal::factories::FactoryId::print_choice
    );
    const auto second = journal::factories::parse_alias_line(
        R"(print_shared = print_and_insert_into_journal("1 is selected."))",
        journal::factories::FactoryId::insert_text
    );
    REQUIRE(first);
    REQUIRE(second);
    const journal::factories::AliasDefinition loaded[] {*first};
    const auto reason = journal::factories::rejection_reason(*second, loaded);
    REQUIRE(reason);
    CHECK(reason->find("already defined") != std::string::npos);
}

TEST_CASE("Reserved names are rejected", "[journal][unit]") {
    auto parsed = journal::factories::parse_alias_line(
        R"(print_extended_timestamp = make_print_choice("Tabby", 0))",
        journal::factories::FactoryId::print_choice
    );
    REQUIRE(parsed);
    const auto reason = journal::factories::rejection_reason(*parsed, {});
    REQUIRE(reason);
    CHECK(reason->find("reserved") != std::string::npos);
    CHECK(journal::factories::is_reserved_alias_name("make_print_choice"));
}

TEST_CASE("Serialize then parse keeps the structured fields", "[journal][unit]") {
    const auto parsed = journal::factories::parse_alias_line(
        R"(print_example = make_print_choice("42nd", true))",
        journal::factories::FactoryId::print_choice
    );
    REQUIRE(parsed);
    const auto line = journal::factories::serialize_alias(*parsed);
    CHECK(line == R"(print_example = make_print_choice("42nd", 0))");
    const auto again = journal::factories::parse_alias_line(
        line,
        journal::factories::FactoryId::print_choice
    );
    REQUIRE(again);
    CHECK(choice(*again)->lower_bound == 0);
}

TEST_CASE("Edit fields come from the parsed definition", "[journal][unit]") {
    const auto parsed = journal::factories::parse_alias_line(
        R"(print_Jose_choice = make_print_choice("Jose", 0))",
        journal::factories::FactoryId::print_choice
    );
    REQUIRE(parsed);
    CHECK(journal::factories::field_value(*parsed, "name") == "Jose");
    CHECK(journal::factories::field_value(*parsed, "lower_bound") == "0");

    const std::string values[] {"Jose", "1"};
    const auto edited = journal::factories::alias_from_fields(
        journal::factories::FactoryId::print_choice,
        parsed->alias,
        values
    );
    REQUIRE(edited);
    CHECK(choice(*edited)->lower_bound == 1);
    CHECK(journal::factories::serialize_alias(*edited) ==
        R"(print_Jose_choice = make_print_choice("Jose", 1))");
}

TEST_CASE("roll_dice parses dice and sides without rolling", "[journal][unit]") {
    const auto parsed = journal::factories::parse_alias_line(
        "roll_7d6 = roll_dice(7, 6)",
        journal::factories::FactoryId::roll_dice
    );
    REQUIRE(parsed);
    const auto* arguments = std::get_if<journal::factories::DiceArguments>(
        &parsed->arguments
    );
    REQUIRE(arguments);
    CHECK(arguments->number_of_dice == 7);
    CHECK(arguments->number_of_sides == 6);
    CHECK(journal::factories::serialize_alias(*parsed) == "roll_7d6 = roll_dice(7, 6)");

    const std::string values[] {"7", "6"};
    const auto from_fields = journal::factories::alias_from_fields(
        journal::factories::FactoryId::roll_dice,
        "roll_7d6",
        values
    );
    REQUIRE(from_fields);
    CHECK(journal::factories::field_value(*from_fields, "number_of_dice") == "7");
    CHECK(journal::factories::field_value(*from_fields, "number_of_sides") == "6");

    const auto action = journal::factories::materialize(*parsed);
    CHECK(static_cast<bool>(action));

    const auto* descriptor = journal::factories::find_factory("roll_dice");
    REQUIRE(descriptor);
    CHECK(descriptor->filename == "roll_dice.list");
    CHECK(journal::factories::factories().size() == 3);
}

TEST_CASE("roll_dice rejects counts outside its bounds", "[journal][unit]") {
    const auto too_few_dice = journal::factories::parse_alias_line(
        "roll_none = roll_dice(0, 6)",
        journal::factories::FactoryId::roll_dice
    );
    const auto coin = journal::factories::parse_alias_line(
        "roll_coin = roll_dice(1, 1)",
        journal::factories::FactoryId::roll_dice
    );
    const auto too_many = journal::factories::parse_alias_line(
        "roll_many = roll_dice(101, 6)",
        journal::factories::FactoryId::roll_dice
    );
    REQUIRE_FALSE(too_few_dice);
    REQUIRE_FALSE(coin);
    REQUIRE_FALSE(too_many);
    CHECK(too_few_dice.error().find("dice") != std::string::npos);
    CHECK(coin.error().find("sides") != std::string::npos);
}

TEST_CASE("Seed creates a missing file and leaves an existing file", "[journal][unit]") {
    const auto root = std::filesystem::temp_directory_path() /
        "auto_core_journal_alias_seed";
    std::filesystem::remove_all(root);
    REQUIRE(journal::factories::seed_missing_alias_files(root) == 0);

    const auto choice_path = journal::factories::alias_path(
        root,
        journal::factories::FactoryId::print_choice
    );
    const auto insert_path = journal::factories::alias_path(
        root,
        journal::factories::FactoryId::insert_text
    );
    const std::string original = read_file(choice_path);
    CHECK(original ==
        "print_auto_core_choice = make_print_choice(\"Auto Core\", 1)\n");
    CHECK(read_file(insert_path) ==
        "print_hello_journal = print_and_insert_into_journal(\"Hello Journal!\")\n");
    const auto dice_path = journal::factories::alias_path(
        root,
        journal::factories::FactoryId::roll_dice
    );
    CHECK(read_file(dice_path) == "r7d8 = roll_dice(7, 8)\n");

    const std::string replacement = "custom alias file\n";
    {
        std::ofstream output(choice_path, std::ios::binary | std::ios::trunc);
        output << replacement;
    }
    REQUIRE(journal::factories::seed_missing_alias_files(root) == 0);
    CHECK(read_file(choice_path) == replacement);
    CHECK(std::filesystem::exists(insert_path));

    std::filesystem::remove(insert_path);
    REQUIRE(journal::factories::seed_missing_alias_files(root) == 0);
    CHECK(read_file(choice_path) == replacement);
    CHECK(read_file(insert_path) ==
        "print_hello_journal = print_and_insert_into_journal(\"Hello Journal!\")\n");

    std::filesystem::remove_all(root);
}
