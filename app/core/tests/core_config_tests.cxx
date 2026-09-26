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
    CHECK(detail::auto_core_ini ==
        "# The presence of this file indicates that Auto Core has been initialized.\n"
        "\n"
        "[auto_core]\n"
        "warn_without_winkey_mapping = true\n");
    CHECK(detail::auto_core_ini.find("initialized =") == std::string_view::npos);
    CHECK(detail::components_ini.find("[settings]") !=
        std::string_view::npos);
    CHECK(detail::components_ini.find("new_components = on") !=
        std::string_view::npos);
    CHECK(detail::components_ini.find("sort_components = on") !=
        std::string_view::npos);
    CHECK(detail::components_ini.find("remove_missing_components = off") !=
        std::string_view::npos);
    CHECK(detail::components_ini.find("[components]") ==
        std::string_view::npos);
    CHECK(detail::components_list.find("[components]") !=
        std::string_view::npos);
    CHECK(detail::components_list.find("writer") != std::string_view::npos);
    CHECK(detail::components_ini.find("rewrite_list") ==
        std::string_view::npos);
    CHECK(detail::components_ini.find("[list]") == std::string_view::npos);
    CHECK(detail::components_ini.find("directory = components") ==
        std::string_view::npos);
    CHECK(detail::crash_recovery_ini.find("default_response = no") !=
        std::string_view::npos);
    CHECK(detail::shutdown_ini.find("delayed_shutdown_prompt = popup") !=
        std::string_view::npos);
    CHECK(detail::shutdown_ini.find("shutdown_timeout_ms = 2000") !=
        std::string_view::npos);
    CHECK(detail::journal_choices_ini.find("print_Tabby_choice =") !=
        std::string_view::npos);
    CHECK(detail::journal_choices_ini.find("print_one_is_selected =") !=
        std::string_view::npos);
    CHECK(detail::keymap_ini ==
        "[keymap]\n"
        "silence_nonset_warning = false\n");
    CHECK(detail::keymap_ini.find("trace_enabled") == std::string_view::npos);
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
    const auto journal = repo / "defaults" / "components" / "journal";

    CHECK(read_default_ini(config / "auto_core.ini") ==
        detail::auto_core_ini);
    CHECK(read_default_ini(config / "components.ini") ==
        detail::components_ini);
    CHECK(read_default_ini(repo / "defaults" / "components.list") ==
        detail::components_list);
    CHECK(read_default_ini(config / "crash_recovery.ini") ==
        detail::crash_recovery_ini);
    CHECK(read_default_ini(config / "shutdown.ini") == detail::shutdown_ini);
    CHECK(read_default_ini(config / "keymap.ini") == detail::keymap_ini);
    CHECK(read_default_ini(journal / "journal_choices.ini") ==
        detail::journal_choices_ini);
}
