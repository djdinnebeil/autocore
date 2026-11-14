#include "catch_amalgamated.hpp"
#include "../settings/settings_catalog.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>

namespace settings = ac::main::settings;

namespace {

void write_empty_file(const std::filesystem::path& path) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    REQUIRE(output);
}

[[nodiscard]]
std::filesystem::path make_bin() {
    const auto directory = std::filesystem::temp_directory_path() /
        "auto_core_settings_catalog_tests" /
        std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()
        );
    std::filesystem::create_directories(directory);
    return directory;
}

}

TEST_CASE(
    "Settings discovery uses installed component executables",
    "[settings][unit]"
) {
    const auto bin = make_bin();
    write_empty_file(bin / "zebra_ac.exe");
    write_empty_file(bin / "alpha_ac.exe");
    write_empty_file(bin / "journal_AC.EXE");
    write_empty_file(bin / "auto_core_settings.exe");
    write_empty_file(bin / "AUTO_CORE_SETTINGS.EXE");
    write_empty_file(bin / "journal_settings.exe");
    write_empty_file(bin / "BadName_ac.exe");
    write_empty_file(bin / "1bad_ac.exe");
    write_empty_file(bin / "notes.txt");
    write_empty_file(bin / "components.list");
    std::ofstream list_file(bin / "components.list", std::ios::binary);
    list_file << "[components]\nspotify off\nalpha off\n";
    list_file.close();

    const auto names = settings::discover_settings_executables(bin);

    REQUIRE(names.size() == 3);
    CHECK(names[0] == "alpha");
    CHECK(names[1] == "journal");
    CHECK(names[2] == "zebra");

    const auto rows = settings::menu_rows(names);
    REQUIRE(rows.size() == 4);
    CHECK(rows[0].label == "Modify Auto Core settings");
    CHECK(rows[0].executable == "auto_core_settings.exe");
    REQUIRE(rows[0].arguments.size() == 1);
    CHECK(rows[0].arguments[0] == "--config");
    CHECK(rows[1].label == "Modify Alpha settings");
    CHECK(rows[1].executable == "alpha_ac.exe");
    REQUIRE(rows[1].arguments.size() == 1);
    CHECK(rows[1].arguments[0] == "--settings");
    CHECK(rows[2].label == "Modify Journal settings");
    CHECK(rows[2].executable == "journal_ac.exe");
    REQUIRE(rows[2].arguments.size() == 1);
    CHECK(rows[2].arguments[0] == "--settings");
    CHECK(rows[3].label == "Modify Zebra settings");
    CHECK(rows[3].executable == "zebra_ac.exe");

    std::filesystem::remove_all(bin);
}

TEST_CASE(
    "A settings executable without a runtime is not a settings row",
    "[settings][unit]"
) {
    const auto bin = make_bin();
    write_empty_file(bin / "notes_settings.exe");
    write_empty_file(bin / "notes_config.exe");

    const auto names = settings::discover_settings_executables(bin);

    CHECK(names.empty());

    std::filesystem::remove_all(bin);
}

TEST_CASE(
    "Every discovered component launches its runtime settings command",
    "[settings][unit]"
) {
    const auto bin = make_bin();
    write_empty_file(bin / "logger_settings.exe");
    write_empty_file(bin / "logger_config.exe");
    write_empty_file(bin / "logger_ac.exe");
    write_empty_file(bin / "spotify_ac.exe");
    write_empty_file(bin / "notes_settings.exe");

    const auto names = settings::discover_settings_executables(bin);

    REQUIRE(names.size() == 2);
    CHECK(names[0] == "logger");
    CHECK(names[1] == "spotify");

    const auto rows = settings::menu_rows(names);
    REQUIRE(rows.size() == 3);
    CHECK(rows[1].executable == "logger_ac.exe");
    REQUIRE(rows[1].arguments.size() == 1);
    CHECK(rows[1].arguments[0] == "--settings");
    CHECK(rows[2].executable == "spotify_ac.exe");
    REQUIRE(rows[2].arguments.size() == 1);
    CHECK(rows[2].arguments[0] == "--settings");

    std::filesystem::remove_all(bin);
}
