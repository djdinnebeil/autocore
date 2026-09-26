#include "catch_amalgamated.hpp"
#include "keymap_map_detail.hpp"

TEST_CASE("Bound keymap line uses spaces around equals and pipe", "[keymap-map][unit]") {
    const auto line = ac::keymap::map_file::format_line(
        "numpad_0",
        "activate_function_key",
        "deactivate_function_key"
    );
    CHECK(line ==
        "numpad_0 = activate_function_key | deactivate_function_key\n");

    const auto mapping = ac::keymap::map_file::parse_line(line);
    REQUIRE(mapping);
    CHECK(mapping->key == "numpad_0");
    CHECK(mapping->primary == "activate_function_key");
    CHECK(mapping->secondary == "deactivate_function_key");
}

TEST_CASE("Unbound keymap line is key =", "[keymap-map][unit]") {
    const auto line = ac::keymap::map_file::format_line("numpad_2", {}, {});
    CHECK(line == "numpad_2 =\n");

    const auto mapping = ac::keymap::map_file::parse_line(line);
    REQUIRE(mapping);
    CHECK(mapping->key == "numpad_2");
    CHECK(mapping->primary.empty());
    CHECK(mapping->secondary.empty());
}

TEST_CASE("A blank side stays blank", "[keymap-map][unit]") {
    const auto mapping = ac::keymap::map_file::parse_line("numpad_2 = |");
    REQUIRE(mapping);
    CHECK(mapping->primary.empty());
    CHECK(mapping->secondary.empty());
}

TEST_CASE("Section headers are ignored", "[keymap-map][unit]") {
    CHECK_FALSE(ac::keymap::map_file::parse_line("[keymap: concise]"));
    CHECK_FALSE(ac::keymap::map_file::parse_line("[keymap]"));
}

TEST_CASE("Fully unset input formats as a blank binding", "[keymap-map][unit]") {
    const auto bare = ac::keymap::map_file::parse_line("numpad_2 =");
    REQUIRE(bare);
    CHECK(bare->primary.empty());
    CHECK(bare->secondary.empty());
    CHECK(
        ac::keymap::map_file::format_line(
            bare->key,
            bare->primary,
            bare->secondary
        ) == "numpad_2 =\n"
    );

    const auto piped = ac::keymap::map_file::parse_line("numpad_2 = |");
    REQUIRE(piped);
    CHECK(piped->primary.empty());
    CHECK(piped->secondary.empty());
    CHECK(
        ac::keymap::map_file::format_line(
            piped->key,
            piped->primary,
            piped->secondary
        ) == "numpad_2 =\n"
    );

    const auto placeholder = ac::keymap::map_file::parse_line(
        "numpad_2 = primary | secondary"
    );
    REQUIRE(placeholder);
    CHECK(placeholder->primary.empty());
    CHECK(placeholder->secondary.empty());
    CHECK(
        ac::keymap::map_file::format_line(
            placeholder->key,
            placeholder->primary,
            placeholder->secondary
        ) == "numpad_2 =\n"
    );
}

TEST_CASE("Whitespace variants of an unset binding stay unset", "[keymap-map][unit]") {
    for (const char* line : {
        "numpad_2=|",
        "numpad_2 = |",
        "numpad_2 =    |",
        "numpad_2 = primary|secondary",
        "numpad_2 = primary | secondary"
    }) {
        const auto mapping = ac::keymap::map_file::parse_line(line);
        REQUIRE(mapping);
        CHECK(mapping->primary.empty());
        CHECK(mapping->secondary.empty());
    }
}

TEST_CASE("A placeholder is unset only in its own slot", "[keymap-map][unit]") {
    const auto secondary_only = ac::keymap::map_file::parse_line(
        "numpad_2 = activate_word |"
    );
    REQUIRE(secondary_only);
    CHECK(secondary_only->primary == "activate_word");
    CHECK(secondary_only->secondary.empty());

    const auto named_secondary = ac::keymap::map_file::parse_line(
        "numpad_2 = activate_word | secondary"
    );
    REQUIRE(named_secondary);
    CHECK(named_secondary->primary == "activate_word");
    CHECK(named_secondary->secondary.empty());

    const auto primary_word = ac::keymap::map_file::parse_line(
        "numpad_2 = primary | activate_word"
    );
    REQUIRE(primary_word);
    CHECK(primary_word->primary.empty());
    CHECK(primary_word->secondary == "activate_word");

    const auto blank_primary = ac::keymap::map_file::parse_line(
        "numpad_2 = | activate_word"
    );
    REQUIRE(blank_primary);
    CHECK(blank_primary->primary.empty());
    CHECK(blank_primary->secondary == "activate_word");
}

TEST_CASE("Misplaced placeholders and extra pipes are invalid", "[keymap-map][unit]") {
    CHECK_FALSE(ac::keymap::map_file::parse_line(
        "numpad_2 = secondary | primary"
    ));
    CHECK_FALSE(ac::keymap::map_file::parse_line(
        "numpad_2 = activate_word | primary"
    ));
    CHECK_FALSE(ac::keymap::map_file::parse_line(
        "numpad_2 = secondary | activate_word"
    ));
    CHECK_FALSE(ac::keymap::map_file::parse_line("numpad_2 = activate_word"));
    CHECK_FALSE(ac::keymap::map_file::parse_line(
        "numpad_2 = activate_word | save_file | close_program"
    ));
}

TEST_CASE("Legacy brace mappings convert to pipe form", "[keymap-map][unit]") {
    const auto mapping = ac::keymap::map_file::parse_legacy_line(
        "numpad_1 = {activate_auto_core, close_program}"
    );
    REQUIRE(mapping);
    CHECK(
        ac::keymap::map_file::format_line(
            mapping->key,
            mapping->primary,
            mapping->secondary
        ) == "numpad_1 = activate_auto_core | close_program\n"
    );
}

TEST_CASE("Pipe inside parentheses is not the action split", "[keymap-map][unit]") {
    const auto mapping = ac::keymap::map_file::parse_line(
        "numpad_3 = make_print_choice(\"a|b\", true) | save_file"
    );
    REQUIRE(mapping);
    CHECK(mapping->primary == "make_print_choice(\"a|b\", true)");
    CHECK(mapping->secondary == "save_file");
}
