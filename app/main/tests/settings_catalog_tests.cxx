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
    "Settings startup shows the menu when the sentinel exists",
    "[settings][unit]"
) {
    CHECK(
        settings::startup_step(true) == settings::StartupStep::show_menu
    );
    CHECK(
        settings::recheck_after_init(true) == settings::InitRecheck::show_menu
    );
}

TEST_CASE(
    "Settings startup launches init and stops when the sentinel stays missing",
    "[settings][unit]"
) {
    CHECK(
        settings::startup_step(false) == settings::StartupStep::launch_init
    );
    CHECK(
        settings::recheck_after_init(false) ==
        settings::InitRecheck::exit_uninitialized
    );
}

TEST_CASE(
    "Settings discovery uses installed settings executables",
    "[settings][unit]"
) {
    const auto bin = make_bin();
    write_empty_file(bin / "zebra_settings.exe");
    write_empty_file(bin / "alpha_settings.exe");
    write_empty_file(bin / "journal_Settings.EXE");
    write_empty_file(bin / "auto_core_settings.exe");
    write_empty_file(bin / "AUTO_CORE_SETTINGS.EXE");
    write_empty_file(bin / "spotify_ac.exe");
    write_empty_file(bin / "BadName_settings.exe");
    write_empty_file(bin / "1bad_settings.exe");
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
    CHECK(rows[0].executable == "auto_core_config.exe");
    CHECK(rows[1].label == "Modify alpha settings");
    CHECK(rows[1].executable == "alpha_settings.exe");
    CHECK(rows[2].label == "Modify journal settings");
    CHECK(rows[3].label == "Modify zebra settings");

    std::filesystem::remove_all(bin);
}

TEST_CASE(
    "Settings discovery keeps a settings executable that has no runtime",
    "[settings][unit]"
) {
    const auto bin = make_bin();
    write_empty_file(bin / "spotify_ac.exe");
    write_empty_file(bin / "itunes_settings.exe");

    const auto names = settings::discover_settings_executables(bin);

    REQUIRE(names.size() == 1);
    CHECK(names[0] == "itunes");

    std::filesystem::remove_all(bin);
}
