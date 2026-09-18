#include "catch_amalgamated.hpp"
#include "../src/config_defaults.hpp"
#include "../src/core_config_detail.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace detail = ac::config::detail;

TEST_CASE("Core configuration keeps the winkey warning by default", "[core-config][unit]") {
    const auto settings = detail::resolve({});

    CHECK(settings.warn_without_winkey_mapping);
}

TEST_CASE("Core configuration keeps the winkey warning when true", "[core-config][unit]") {
    const auto settings = detail::resolve({
        .warn_without_winkey_mapping = "true"
    });

    CHECK(settings.warn_without_winkey_mapping);
}

TEST_CASE("Core configuration silences the winkey warning only for false", "[core-config][unit]") {
    const auto settings = detail::resolve({
        .warn_without_winkey_mapping = "false"
    });

    CHECK_FALSE(settings.warn_without_winkey_mapping);
}

TEST_CASE("Core configuration keeps the winkey warning for invalid values", "[core-config][unit]") {
    const auto settings = detail::resolve({
        .warn_without_winkey_mapping = "no"
    });

    CHECK(settings.warn_without_winkey_mapping);
}

TEST_CASE("Portable config defaults match the documented keys", "[core-config][unit]") {
    CHECK(detail::auto_core_ini.find("[auto_core]") != std::string_view::npos);
    CHECK(detail::auto_core_ini.find("warn_without_winkey_mapping = true") !=
        std::string_view::npos);
    CHECK(detail::auto_core_ini.find("program_title") == std::string_view::npos);
    CHECK(detail::logger_ini.find("enabled") == std::string_view::npos);
    CHECK(detail::logger_ini.find("directory = logs") != std::string_view::npos);
    CHECK(detail::logger_ini.find("write_to_console = false") !=
        std::string_view::npos);
    CHECK(detail::components_list.find("[components]") !=
        std::string_view::npos);
    CHECK(detail::components_list.find("logger\n") != std::string_view::npos);
    CHECK(detail::components_list.find("taskbar\n") != std::string_view::npos);
    CHECK(detail::components_list.find("journal\n") != std::string_view::npos);
    CHECK(detail::components_list.find("itunes\n") != std::string_view::npos);
    CHECK(detail::components_list.find("spotify\n") != std::string_view::npos);
    CHECK(detail::components_list.find("wake\n") != std::string_view::npos);
    CHECK(detail::components_list.find("writer\n") != std::string_view::npos);
    CHECK(detail::components_list.find("server\n") != std::string_view::npos);
    CHECK(detail::components_list.find("dash\n") != std::string_view::npos);
    CHECK(detail::components_list.find("slash\n") != std::string_view::npos);
    CHECK(detail::server_ini.find("port = 8585") != std::string_view::npos);
    CHECK(detail::server_ini.find("document_root = server") != std::string_view::npos);
    CHECK(detail::crash_recovery_ini.find("default_response = no") !=
        std::string_view::npos);
    CHECK(detail::shutdown_ini.find("delayed_shutdown_prompt = popup") !=
        std::string_view::npos);
    CHECK(detail::shutdown_ini.find("shutdown_timeout_ms = 5000") !=
        std::string_view::npos);
    CHECK(detail::itunes_ini.find("auto_start = false") !=
        std::string_view::npos);
    CHECK(detail::itunes_ini.find("tab_end = 3") != std::string_view::npos);
    CHECK(detail::journal_ini.find("directory = journal") != std::string_view::npos);
    CHECK(detail::journal_ini.find("series =\n") != std::string_view::npos);
    CHECK(detail::journal_ini.find("series = Journal") == std::string_view::npos);
    CHECK(detail::journal_ini.find("remote_sync = disable") != std::string_view::npos);
    CHECK(detail::journal_ini.find("day_rollover_hour = 0") != std::string_view::npos);
    CHECK(detail::journal_choices_ini.find("print_Tabby_choice =") !=
        std::string_view::npos);
    CHECK(detail::journal_choices_ini.find("print_one_is_selected =") !=
        std::string_view::npos);
    CHECK(detail::taskbar_ini.find("[taskbar]") != std::string_view::npos);
    CHECK(detail::taskbar_ini.find("directory = taskbar") != std::string_view::npos);
    CHECK(detail::taskbar_ini.find("mode = live") != std::string_view::npos);
    CHECK(detail::taskbar_ini.find("[settings]") == std::string_view::npos);
    CHECK(detail::taskbar_ini.find("warn_auto_core_without_winkey_mapping") ==
        std::string_view::npos);
    CHECK(detail::keymap_ini.find("[keymap]") != std::string_view::npos);
    CHECK(detail::keymap_ini.find("trace_enabled = false") != std::string_view::npos);
    CHECK(detail::keymap_ini.find("silence_nonset_warning = false") !=
        std::string_view::npos);
    CHECK(detail::spotify_ini.find("[spotify]") != std::string_view::npos);
    CHECK(detail::spotify_ini.find("directory = spotify") !=
        std::string_view::npos);
    CHECK(detail::writer_ini.find("[writer]") != std::string_view::npos);
    CHECK(detail::writer_ini.find("directory = writer") !=
        std::string_view::npos);
    CHECK(detail::writer_ini.find("notes_directory = notes") !=
        std::string_view::npos);
}

namespace {

std::filesystem::path repository_root() {
    std::filesystem::path start {__FILE__};
    if (start.is_relative()) {
        start = std::filesystem::current_path() / start;
    }
    start = start.lexically_normal();
    for (auto directory = start.parent_path();
        !directory.empty() && directory != directory.root_path();
        directory = directory.parent_path()) {
        if (std::filesystem::exists(
                directory / "defaults" / "config" / "writer.ini"
            )) {
            return directory;
        }
    }
    FAIL("Unable to locate defaults/config/writer.ini from the test source");
    return {};
}

std::string read_default_ini(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    REQUIRE(input);
    std::string bytes(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>()
    );
    bytes.erase(
        std::remove(bytes.begin(), bytes.end(), '\r'),
        bytes.end()
    );
    return bytes;
}

}

TEST_CASE("Tracked defaults/ files match portable config defaults", "[core-config][unit]") {
    const auto repo = repository_root();
    const auto config = repo / "defaults" / "config";
    const auto journal = repo / "defaults" / "journal";

    CHECK(read_default_ini(config / "auto_core.ini") ==
        detail::auto_core_ini);
    CHECK(read_default_ini(config / "logger.ini") == detail::logger_ini);
    CHECK(read_default_ini(config / "components.list") ==
        detail::components_list);
    CHECK(read_default_ini(config / "server.ini") == detail::server_ini);
    CHECK(read_default_ini(config / "crash_recovery.ini") ==
        detail::crash_recovery_ini);
    CHECK(read_default_ini(config / "shutdown.ini") == detail::shutdown_ini);
    CHECK(read_default_ini(config / "itunes.ini") == detail::itunes_ini);
    CHECK(read_default_ini(config / "journal.ini") ==
        detail::journal_ini);
    CHECK(read_default_ini(config / "taskbar.ini") ==
        detail::taskbar_ini);
    CHECK(read_default_ini(config / "keymap.ini") == detail::keymap_ini);
    CHECK(read_default_ini(config / "spotify.ini") ==
        detail::spotify_ini);
    CHECK(read_default_ini(config / "writer.ini") == detail::writer_ini);
    CHECK(read_default_ini(journal / "journal_choices.ini") ==
        detail::journal_choices_ini);
}
