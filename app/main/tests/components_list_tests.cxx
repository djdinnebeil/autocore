#include "catch_amalgamated.hpp"
#include "components_list_detail.hpp"
#include "config_defaults.hpp"

#include <algorithm>
#include <filesystem>
#include <string_view>

namespace list = ac::config::components_list;

namespace {

[[nodiscard]]
bool enabled(const list::ParseResult& result, const std::string_view name) {
    return std::ranges::find(result.enabled, name) != result.enabled.end();
}

}

TEST_CASE("Bare component name is enabled", "[components-list][unit]") {
    const auto result = list::parse(
        "[components]\n"
        "journal\n"
    );

    REQUIRE(result.ok);
    CHECK(enabled(result, "journal"));
    CHECK_FALSE(enabled(result, "taskbar"));
}

TEST_CASE("Open catalog enables an unknown lowercase name", "[components-list][unit]") {
    const auto result = list::parse(
        "[components]\n"
        "weather\n"
    );

    REQUIRE(result.ok);
    CHECK(enabled(result, "weather"));
    CHECK(result.invalid_names.empty());
    CHECK(result.specials.empty());
}

TEST_CASE("Component on is enabled", "[components-list][unit]") {
    const auto result = list::parse(
        "[components]\n"
        "journal on\n"
    );

    REQUIRE(result.ok);
    CHECK(enabled(result, "journal"));
}

TEST_CASE("Component off is disabled", "[components-list][unit]") {
    const auto result = list::parse(
        "[components]\n"
        "journal off\n"
    );

    REQUIRE(result.ok);
    CHECK_FALSE(enabled(result, "journal"));
}

TEST_CASE("Malformed state disables the component", "[components-list][unit]") {
    const auto result = list::parse(
        "[components]\n"
        "journal offf\n"
        "itunes true\n"
        "spotify on extra\n"
        "wake ON\n"
    );

    REQUIRE(result.ok);
    CHECK_FALSE(enabled(result, "journal"));
    CHECK_FALSE(enabled(result, "itunes"));
    CHECK_FALSE(enabled(result, "spotify"));
    CHECK_FALSE(enabled(result, "wake"));
}

TEST_CASE("Duplicate entries disable that component", "[components-list][unit]") {
    const auto result = list::parse(
        "[components]\n"
        "journal\n"
        "journal on\n"
        "itunes off\n"
        "itunes\n"
    );

    REQUIRE(result.ok);
    CHECK_FALSE(enabled(result, "journal"));
    CHECK_FALSE(enabled(result, "itunes"));
}

TEST_CASE("Known specials are not v1-enabled", "[components-list][unit]") {
    const auto result = list::parse(
        "[components]\n"
        "journal\n"
        "logger\n"
        "dash off\n"
        "slash\n"
    );

    REQUIRE(result.ok);
    CHECK(enabled(result, "journal"));
    CHECK_FALSE(enabled(result, "logger"));
    CHECK_FALSE(enabled(result, "dash"));
    CHECK_FALSE(enabled(result, "slash"));
    CHECK(list::special_enabled(result, "logger"));
    CHECK_FALSE(list::special_enabled(result, "dash"));
    CHECK(list::special_enabled(result, "slash"));
    REQUIRE(result.specials.size() == 2);
    CHECK(result.specials[0] == "logger");
    CHECK(result.specials[1] == "slash");
}

TEST_CASE("Duplicate specials are disabled", "[components-list][unit]") {
    const auto result = list::parse(
        "[components]\n"
        "logger\n"
        "logger on\n"
        "dash off\n"
        "dash\n"
    );

    REQUIRE(result.ok);
    CHECK_FALSE(list::special_enabled(result, "logger"));
    CHECK_FALSE(list::special_enabled(result, "dash"));
    CHECK(result.specials.empty());
}

TEST_CASE("Absent specials are disabled", "[components-list][unit]") {
    const auto result = list::parse(
        "[components]\n"
        "journal\n"
    );

    REQUIRE(result.ok);
    CHECK_FALSE(list::special_enabled(result, "logger"));
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

TEST_CASE("Missing section fails closed", "[components-list][unit]") {
    const auto result = list::parse("journal\njournal on\n");

    CHECK_FALSE(result.ok);
    CHECK(result.enabled.empty());
    CHECK(result.invalid_names.empty());
    CHECK(result.specials.empty());
    CHECK_FALSE(list::special_enabled(result, "logger"));
    CHECK(result.error.find("[components]") != std::string::npos);
}

TEST_CASE("File-open failure fails closed", "[components-list][unit]") {
    const auto path =
        std::filesystem::temp_directory_path() /
        "auto_core_missing_components.list";
    std::filesystem::remove(path);

    const auto result = list::load(path);

    CHECK_FALSE(result.ok);
    CHECK(result.enabled.empty());
    CHECK_FALSE(list::special_enabled(result, "logger"));
    CHECK(result.error.find("Unable to open") != std::string::npos);
}

TEST_CASE("Portable default enables every shipped component", "[components-list][unit]") {
    const auto result = list::parse(ac::config::detail::components_list);

    REQUIRE(result.ok);
    CHECK(result.invalid_names.empty());
    CHECK(enabled(result, "taskbar"));
    CHECK(enabled(result, "journal"));
    CHECK(enabled(result, "itunes"));
    CHECK(enabled(result, "spotify"));
    CHECK(enabled(result, "wake"));
    CHECK(enabled(result, "writer"));
    CHECK(enabled(result, "server"));
    CHECK_FALSE(enabled(result, "logger"));
    CHECK_FALSE(enabled(result, "dash"));
    CHECK_FALSE(enabled(result, "slash"));
    CHECK(list::special_enabled(result, "logger"));
    CHECK(list::special_enabled(result, "dash"));
    CHECK(list::special_enabled(result, "slash"));
}
