#include "catch_amalgamated.hpp"
#include "../shared/library_format_detail.hpp"

#include <filesystem>
#include <fstream>

namespace library = itunes::library_format::detail;

namespace {

std::filesystem::path test_directory(std::string_view name) {
    const auto directory = std::filesystem::temp_directory_path() /
        "ac_itunes_library_format" / name;
    std::error_code error;
    std::filesystem::remove_all(directory, error);
    std::filesystem::create_directories(directory, error);
    return directory;
}

void write_text(const std::filesystem::path& path, std::string_view text) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(text.data(), static_cast<std::streamsize>(text.size()));
}

library::LibraryFormat sample(std::string_view text, int column_count) {
    library::LibraryFormat format;
    format.format_text = std::string {text};
    format.compiled = *library::compile_library_format(text);
    format.column_count = column_count;
    return format;
}

} // namespace

TEST_CASE("iTunes library format defaults are bracketed columns", "[itunes][format][unit]") {
    CHECK(library::default_column_count == 4);
    CHECK(library::default_library_format == "[column]");

    const library::LibraryFormat format;
    CHECK(format.format_text == "[column]");
    CHECK(format.column_count == 4);
    CHECK(format.compiled.mode == library::LibraryFormatMode::surround);
    CHECK(format.compiled.left_symbol == '[');
    CHECK(format.compiled.right_symbol == ']');
    CHECK(
        library::library_format_text(format) ==
        "format = [column]\ncolumn_count = 4\n"
    );
    CHECK(
        library::library_format_text(sample("column - column", 12)) ==
        "format = column - column\ncolumn_count = 12\n"
    );
}

TEST_CASE("iTunes library format rejects non-positive column counts", "[itunes][format][unit]") {
    CHECK(library::parse_column_count("1") == 1);
    CHECK(library::parse_column_count("4") == 4);
    CHECK_FALSE(library::parse_column_count("0"));
    CHECK_FALSE(library::parse_column_count("-2"));
    CHECK_FALSE(library::parse_column_count("3x"));
    CHECK_FALSE(library::parse_column_count(""));
    CHECK_FALSE(library::parse_column_count("three"));
}

TEST_CASE("iTunes library format accepts the two supported forms", "[itunes][format][unit]") {
    const auto brackets = library::compile_library_format("[column]");
    const auto braces = library::compile_library_format("{column}");
    const auto parentheses = library::compile_library_format("(column)");
    const auto angles = library::compile_library_format("<column>");
    const auto dash = library::compile_library_format("column - column");
    const auto bar = library::compile_library_format("column | column");
    const auto slash = library::compile_library_format("column / column");
    const auto spaced = library::compile_library_format("column  -  column");
    const auto tabbed = library::compile_library_format("column\t-\tcolumn");

    REQUIRE(brackets);
    CHECK(brackets->mode == library::LibraryFormatMode::surround);
    CHECK(brackets->left_symbol == '[');
    CHECK(brackets->right_symbol == ']');
    REQUIRE(braces);
    CHECK(braces->left_symbol == '{');
    CHECK(braces->right_symbol == '}');
    REQUIRE(parentheses);
    CHECK(parentheses->left_symbol == '(');
    CHECK(parentheses->right_symbol == ')');
    REQUIRE(angles);
    REQUIRE(dash);
    CHECK(dash->mode == library::LibraryFormatMode::separate);
    CHECK(dash->separator == " - ");
    REQUIRE(bar);
    CHECK(bar->separator == " | ");
    REQUIRE(slash);
    CHECK(slash->separator == " / ");
    REQUIRE(spaced);
    CHECK(spaced->separator == "  -  ");
    REQUIRE(tabbed);
    CHECK(tabbed->separator == "\t-\t");
}

TEST_CASE("iTunes library format rejects unsupported expressions", "[itunes][format][unit]") {
    CHECK_FALSE(library::compile_library_format(""));
    CHECK_FALSE(library::compile_library_format("song"));
    CHECK_FALSE(library::compile_library_format("column"));
    CHECK_FALSE(library::compile_library_format("column - column - column"));
    CHECK_FALSE(library::compile_library_format("[column] - [column]"));
    CHECK_FALSE(library::compile_library_format("[column] {column}"));
    CHECK_FALSE(library::compile_library_format("[[column]]"));
    CHECK_FALSE(library::compile_library_format("[column] "));
    CHECK_FALSE(library::compile_library_format("column -- column"));
    CHECK_FALSE(library::compile_library_format("column - - column"));
    CHECK_FALSE(library::compile_library_format("prefix column - column"));
    CHECK_FALSE(library::compile_library_format("column - column suffix"));
}

TEST_CASE("Valid iTunes library.format is loaded", "[itunes][format][unit]") {
    const auto path = test_directory("valid") / "library.format";
    write_text(path, "format = {column}\ncolumn_count = 5\n");

    const auto loaded = library::load_library_format(path);

    CHECK(loaded.format.format_text == "{column}");
    CHECK(loaded.format.compiled.left_symbol == '{');
    CHECK(loaded.format.column_count == 5);
    CHECK_FALSE(loaded.error);
}

TEST_CASE("iTunes library.format ignores blank lines", "[itunes][format][unit]") {
    const auto path = test_directory("blank") / "library.format";
    write_text(path, "\n\ncolumn_count = 2\n\n");

    const auto loaded = library::load_library_format(path);

    CHECK(loaded.format.format_text == "[column]");
    CHECK(loaded.format.column_count == 2);
    CHECK_FALSE(loaded.error);
}

TEST_CASE("Missing iTunes library.format uses the compiled default", "[itunes][format][unit]") {
    const auto path = test_directory("missing") / "library.format";
    std::error_code error;
    std::filesystem::remove_all(path.parent_path(), error);

    const auto loaded = library::load_library_format(path);

    CHECK(loaded.format.format_text == "[column]");
    CHECK(loaded.format.column_count == library::default_column_count);
    CHECK_FALSE(loaded.error);
    CHECK_FALSE(std::filesystem::exists(path));
}

TEST_CASE("iTunes library.format without column_count uses the compiled default", "[itunes][format][unit]") {
    const auto path = test_directory("missing-key") / "library.format";
    write_text(path, "format = (column)\n");

    const auto loaded = library::load_library_format(path);

    CHECK(loaded.format.format_text == "(column)");
    CHECK(loaded.format.column_count == library::default_column_count);
    CHECK_FALSE(loaded.error);
}

TEST_CASE("Missing iTunes library format expression uses the compiled default", "[itunes][format][unit]") {
    const auto path = test_directory("missing-format") / "library.format";
    write_text(path, "column_count = 4\n");

    const auto loaded = library::load_library_format(path);

    CHECK(loaded.format.format_text == "[column]");
    CHECK(loaded.format.column_count == 4);
    CHECK_FALSE(loaded.error);
}

TEST_CASE("Malformed iTunes library.format falls back to the compiled default", "[itunes][format][unit]") {
    const auto path = test_directory("malformed") / "library.format";
    write_text(path, "note\ncolumn_count = no\n");

    const auto loaded = library::load_library_format(path);

    CHECK(loaded.format.format_text == "[column]");
    CHECK(loaded.format.column_count == library::default_column_count);
    CHECK(loaded.error);
}

TEST_CASE("Non-positive iTunes library.format values fall back to the compiled default", "[itunes][format][unit]") {
    const auto zero = test_directory("zero") / "library.format";
    write_text(zero, "format = column | column\ncolumn_count = 0\n");
    const auto negative = test_directory("negative") / "library.format";
    write_text(negative, "column_count = -2\n");

    const auto loaded_zero = library::load_library_format(zero);
    const auto loaded_negative = library::load_library_format(negative);

    CHECK(loaded_zero.format.format_text == "column | column");
    CHECK(loaded_zero.format.column_count == library::default_column_count);
    CHECK(loaded_zero.error);
    CHECK(loaded_negative.format.format_text == "[column]");
    CHECK(loaded_negative.format.column_count == library::default_column_count);
    CHECK(loaded_negative.error);
}

TEST_CASE("Invalid iTunes library format keeps a valid column count", "[itunes][format][unit]") {
    const auto path = test_directory("bad-format") / "library.format";
    write_text(path, "format = column\ncolumn_count = 4\n");

    const auto loaded = library::load_library_format(path);

    CHECK(loaded.format.format_text == "[column]");
    CHECK(loaded.format.compiled.left_symbol == '[');
    CHECK(loaded.format.column_count == 4);
    CHECK(loaded.error);
}

TEST_CASE("Invalid iTunes library fields fall back independently", "[itunes][format][unit]") {
    const auto path = test_directory("both-invalid") / "library.format";
    write_text(path, "format = song\ncolumn_count = -1\n");

    const auto loaded = library::load_library_format(path);

    CHECK(loaded.format.format_text == "[column]");
    CHECK(loaded.format.column_count == library::default_column_count);
    CHECK(loaded.error);
    CHECK(loaded.error->find("format") != std::string::npos);
    CHECK(loaded.error->find("column_count") != std::string::npos);
}

TEST_CASE("Unreadable iTunes library.format falls back to the compiled default", "[itunes][format][unit]") {
    const auto path = test_directory("unreadable") / "library.format";
    std::error_code error;
    std::filesystem::create_directory(path, error);

    const auto loaded = library::load_library_format(path);

    CHECK(loaded.format.format_text == "[column]");
    CHECK(loaded.format.column_count == library::default_column_count);
    CHECK(loaded.error);
    std::filesystem::remove_all(path, error);
}
