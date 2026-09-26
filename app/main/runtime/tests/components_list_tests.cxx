#include "catch_amalgamated.hpp"
#include "components_catalog_detail.hpp"
#include "components_list_detail.hpp"
#include "config_defaults.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string_view>

namespace list = ac::config::components_list;

namespace {

[[nodiscard]]
bool enabled(const list::ParseResult& result, const std::string_view name) {
    return std::ranges::find(result.enabled, name) != result.enabled.end();
}

void write_empty_file(const std::filesystem::path& path) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    REQUIRE(output);
}

}

TEST_CASE("Header comment is ignored under [components]", "[components-list][unit]") {
    const auto result = list::parse(
        "# Leave blank for on; use explicit on/off to override\n"
        "[components]\n"
        "journal\n"
    );

    REQUIRE(result.ok);
    CHECK(enabled(result, "journal"));
    CHECK(result.invalid_names.empty());
    CHECK(result.malformed_values.empty());
}

TEST_CASE("Bare component name defaults to on", "[components-list][unit]") {
    const auto result = list::parse(
        "[components]\n"
        "journal\n"
    );

    REQUIRE(result.ok);
    CHECK(enabled(result, "journal"));
    CHECK_FALSE(enabled(result, "taskbar"));
}

TEST_CASE("Explicit on enables a component", "[components-list][unit]") {
    const auto result = list::parse(
        "[components]\n"
        "weather on\n"
    );

    REQUIRE(result.ok);
    CHECK(enabled(result, "weather"));
    CHECK(result.invalid_names.empty());
    CHECK(result.specials.empty());
}

TEST_CASE("Explicit off disables a component", "[components-list][unit]") {
    const auto result = list::parse(
        "[components]\n"
        "journal off\n"
    );

    REQUIRE(result.ok);
    CHECK_FALSE(enabled(result, "journal"));
    CHECK(list::name_is_listed(result, "journal"));
    CHECK(result.malformed_values.empty());
}

TEST_CASE("Listed enablement distinguishes unread from disabled", "[components-list][unit]") {
    const auto parsed = list::parse(
        "[components]\n"
        "dash\n"
        "spotify on\n"
        "itunes off\n"
        "logger\n"
        "journal maybe\n"
    );

    CHECK(list::listed_enablement(parsed, "dash") ==
        list::ListedEnablement::enabled);
    CHECK(list::listed_enablement(parsed, "spotify") ==
        list::ListedEnablement::enabled);
    CHECK(list::listed_enablement(parsed, "logger") ==
        list::ListedEnablement::enabled);
    CHECK(list::listed_enablement(parsed, "itunes") ==
        list::ListedEnablement::disabled);
    CHECK(list::listed_enablement(parsed, "journal") ==
        list::ListedEnablement::disabled);
    CHECK(list::listed_enablement(parsed, "wake") ==
        list::ListedEnablement::disabled);

    list::ParseResult unread;
    unread.ok = false;
    CHECK(list::listed_enablement(unread, "spotify") ==
        list::ListedEnablement::unavailable);
}

TEST_CASE("Malformed value disables the component and is recorded", "[components-list][unit]") {
    const auto result = list::parse(
        "[components]\n"
        "journal offf\n"
        "itunes true\n"
        "spotify on extra\n"
        "wake ON\n"
        "writer =\n"
    );

    REQUIRE(result.ok);
    CHECK_FALSE(enabled(result, "journal"));
    CHECK_FALSE(enabled(result, "itunes"));
    CHECK_FALSE(enabled(result, "spotify"));
    CHECK_FALSE(enabled(result, "wake"));
    CHECK_FALSE(enabled(result, "writer"));
    REQUIRE(result.malformed_values.size() == 5);
}

TEST_CASE("Last duplicate [components] value wins", "[components-list][unit]") {
    const auto result = list::parse(
        "[components]\n"
        "journal off\n"
        "journal\n"
        "itunes\n"
        "itunes off\n"
    );

    REQUIRE(result.ok);
    CHECK(enabled(result, "journal"));
    CHECK_FALSE(enabled(result, "itunes"));
}

TEST_CASE("Known specials are not v1-enabled", "[components-list][unit]") {
    const auto result = list::parse(
        "[components]\n"
        "journal\n"
        "dash off\n"
        "slash on\n"
    );

    REQUIRE(result.ok);
    CHECK(enabled(result, "journal"));
    CHECK_FALSE(enabled(result, "dash"));
    CHECK_FALSE(enabled(result, "slash"));
    CHECK_FALSE(list::special_enabled(result, "dash"));
    CHECK(list::special_enabled(result, "slash"));
    REQUIRE(result.specials.size() == 1);
    CHECK(result.specials[0] == "slash");
}

TEST_CASE("Absent specials are disabled", "[components-list][unit]") {
    const auto result = list::parse(
        "[components]\n"
        "journal\n"
    );

    REQUIRE(result.ok);
    CHECK_FALSE(list::special_enabled(result, "dash"));
    CHECK_FALSE(list::special_enabled(result, "slash"));
}

TEST_CASE("Invalid names are not normalized", "[components-list][unit]") {
    const auto result = list::parse(
        "[components]\n"
        "Weather\n"
        "weather\n"
    );

    REQUIRE(result.ok);
    CHECK(enabled(result, "weather"));
    REQUIRE(result.invalid_names.size() == 1);
    CHECK(result.invalid_names[0] == "Weather");
}

TEST_CASE("Readable empty [components] enables nothing", "[components-list][unit]") {
    const auto result = list::parse(
        "[settings]\n"
        "new_components = on\n"
        "\n"
        "[components]\n"
    );

    REQUIRE(result.ok);
    CHECK(result.enabled.empty());
    CHECK(result.specials.empty());
    CHECK(result.malformed_values.empty());
}

TEST_CASE("File-open failure fails closed", "[components-list][unit]") {
    const auto path =
        std::filesystem::temp_directory_path() /
        "auto_core_missing_components.list";
    std::filesystem::remove(path);

    const auto result = list::load(path);

    CHECK_FALSE(result.ok);
    CHECK(result.enabled.empty());
    CHECK_FALSE(list::special_enabled(result, "dash"));
    CHECK(result.error.find("Unable to open") != std::string::npos);
}

TEST_CASE("Portable default components.list enables the known names", "[components-list][unit]") {
    const auto result = list::parse(ac::config::detail::components_list);

    REQUIRE(result.ok);
    CHECK(enabled(result, "journal"));
    CHECK(enabled(result, "logger"));
    CHECK(enabled(result, "writer"));
    CHECK(list::special_enabled(result, "dash"));
    CHECK(list::special_enabled(result, "slash"));
    CHECK(result.invalid_names.empty());
}

TEST_CASE("Discovery includes specials and valid *_ac.exe names", "[components-list][unit]") {
    const auto directory =
        std::filesystem::temp_directory_path() /
        "auto_core_component_discovery";
    std::filesystem::remove_all(directory);
    write_empty_file(directory / "dash_ac.exe");
    write_empty_file(directory / "slash_ac.exe");
    write_empty_file(directory / "journal_ac.exe");
    write_empty_file(directory / "skip.txt");
    write_empty_file(directory / "Weather_ac.exe");
    write_empty_file(directory / "_ac.exe");

    const auto names = list::discover_ac_executables(directory);
    REQUIRE(names.size() == 3);
    CHECK(names[0] == "dash");
    CHECK(names[1] == "journal");
    CHECK(names[2] == "slash");

    const auto catalog = list::catalog_from_names(names);
    REQUIRE(catalog.ok);
    CHECK(enabled(catalog, "journal"));
    CHECK(list::special_enabled(catalog, "dash"));
    CHECK(list::special_enabled(catalog, "slash"));

    std::filesystem::remove_all(directory);
}

TEST_CASE("Missing list load_runtime_catalog discovers executables", "[components-list][unit]") {
    const auto directory =
        std::filesystem::temp_directory_path() /
        "auto_core_runtime_catalog_discovery";
    std::filesystem::remove_all(directory);
    write_empty_file(directory / "weather_ac.exe");
    write_empty_file(directory / "dash_ac.exe");

    const auto missing =
        directory / "missing" / "components.list";
    const auto catalog = list::load_runtime_catalog(missing, directory);

    CHECK(catalog.used_discovery);
    CHECK(catalog.result.ok);
    CHECK(enabled(catalog.result, "weather"));
    CHECK(list::special_enabled(catalog.result, "dash"));

    std::filesystem::remove_all(directory);
}

TEST_CASE("Readable components.list is authoritative over discovery", "[components-list][unit]") {
    const auto directory =
        std::filesystem::temp_directory_path() /
        "auto_core_runtime_catalog_list";
    std::filesystem::remove_all(directory);
    write_empty_file(directory / "weather_ac.exe");
    const auto list_file = directory / "components.list";
    {
        std::ofstream output(list_file, std::ios::binary | std::ios::trunc);
        REQUIRE(output);
        output << "[components]\njournal\n";
    }

    const auto catalog = list::load_runtime_catalog(list_file, directory);
    CHECK_FALSE(catalog.used_discovery);
    CHECK(enabled(catalog.result, "journal"));
    CHECK_FALSE(enabled(catalog.result, "weather"));

    std::filesystem::remove_all(directory);
}

namespace catalog = ac::config::components_catalog;

TEST_CASE("Catalog settings parse is case-insensitive", "[components-catalog][unit]") {
    const auto settings = catalog::parse_settings(
        "[settings]\n"
        "new_components = Prompt\n"
        "sort_components = OFF\n"
        "remove_missing_components = On\n"
    );

    CHECK(settings.new_components ==
        catalog::NewComponentsPolicy::prompt);
    CHECK_FALSE(settings.sort_components);
    CHECK(settings.remove_missing_components);
}

TEST_CASE("Catalog list parse uses blank and off forms", "[components-catalog][unit]") {
    const auto document = catalog::parse_list(
        "[components]\n"
        "journal\n"
        "wake off\n"
    );

    REQUIRE(document.components.size() == 2);
    CHECK(document.components[0].name == "journal");
    CHECK(document.components[0].enabled);
    CHECK(document.components[1].name == "wake");
    CHECK_FALSE(document.components[1].enabled);
}

TEST_CASE("Catalog rewrite is canonical lowercase", "[components-catalog][unit]") {
    catalog::Document document;
    document.settings.new_components = catalog::NewComponentsPolicy::on;
    document.settings.sort_components = true;
    document.settings.remove_missing_components = false;
    document.components.push_back({.name = "wake", .enabled = false});
    document.components.push_back({.name = "journal", .enabled = true});
    catalog::apply_sort(document);

    CHECK(catalog::format_settings(document.settings) ==
        "[settings]\n"
        "new_components = on\n"
        "sort_components = on\n"
        "remove_missing_components = off\n");
    CHECK(catalog::format_list(document) ==
        "# Leave blank for on; use explicit on/off to override\n"
        "[components]\n"
        "journal\n"
        "wake off\n");
}

TEST_CASE("Explicit state rewrite touches one component", "[components-catalog][unit]") {
    const auto original =
        "# Leave blank for on; use explicit on/off to override\n"
        "[components]\n"
        "dash\n"
        "spotify\n"
        "wake off\n";

    const auto disabled = catalog::set_listed_state(original, "spotify", false);
    REQUIRE(disabled);
    CHECK(*disabled ==
        "# Leave blank for on; use explicit on/off to override\n"
        "[components]\n"
        "dash\n"
        "spotify off\n"
        "wake off\n");

    const auto enabled = catalog::set_listed_state(*disabled, "spotify", true);
    REQUIRE(enabled);
    CHECK(*enabled ==
        "# Leave blank for on; use explicit on/off to override\n"
        "[components]\n"
        "dash\n"
        "spotify on\n"
        "wake off\n");
}

TEST_CASE("Explicit state rewrite appends inside the components section", "[components-catalog][unit]") {
    const auto updated = catalog::set_listed_state(
        "[components]\n"
        "journal\n"
        "\n"
        "[notes]\n"
        "keep\n",
        "spotify",
        false
    );

    REQUIRE(updated);
    CHECK(*updated ==
        "[components]\n"
        "journal\n"
        "\n"
        "spotify off\n"
        "[notes]\n"
        "keep\n");
    CHECK_FALSE(catalog::set_listed_state(updated.value(), "Spotify", true));
}

TEST_CASE("Catalog sort off preserves order and appends", "[components-catalog][unit]") {
    catalog::Document document;
    document.settings.sort_components = false;
    document.components.push_back({.name = "wake", .enabled = true});
    catalog::add_discovered(
        document,
        {"journal", "wake"},
        [](std::string_view) { return false; }
    );
    catalog::apply_sort(document);

    REQUIRE(document.components.size() == 2);
    CHECK(document.components[0].name == "wake");
    CHECK(document.components[1].name == "journal");
    CHECK_FALSE(document.components[1].enabled);
}

TEST_CASE("Full sync can remove missing names", "[components-catalog][unit]") {
    catalog::Document document;
    document.settings.remove_missing_components = true;
    document.settings.sort_components = true;
    document.components.push_back({.name = "gone", .enabled = true});
    document.components.push_back({.name = "journal", .enabled = false});
    catalog::full_sync(
        document,
        {"journal", "wake"},
        [](std::string_view) { return true; }
    );

    REQUIRE(document.components.size() == 2);
    CHECK(document.components[0].name == "journal");
    CHECK_FALSE(document.components[0].enabled);
    CHECK(document.components[1].name == "wake");
    CHECK(document.components[1].enabled);
}
