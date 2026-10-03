#include "catch_amalgamated.hpp"
#include "../main/itunes_config_detail.hpp"

#include <filesystem>

import itunes_defaults;

namespace detail = itunes::config::detail;

TEST_CASE("iTunes configuration uses component defaults", "[itunes][config][unit]") {
    const auto settings = detail::resolve({});

    CHECK(settings.directory == "components/itunes");
    CHECK(settings.auto_start);
}

TEST_CASE("iTunes ini defaults omit tab_end", "[itunes][config][unit]") {
    const auto seeded = std::string {itunes::defaults::ini_text};
    const auto rewritten = itunes::defaults::ini_for(
        itunes::defaults::directory,
        itunes::defaults::auto_start
    );

    CHECK(seeded.find("tab_end") == std::string::npos);
    CHECK(rewritten.find("tab_end") == std::string::npos);
    CHECK(seeded.find("directory = components/itunes\n") != std::string::npos);
    CHECK(seeded.find("auto_start = on\n") != std::string::npos);
}

TEST_CASE("iTunes directory keeps a non-empty stored value", "[itunes][config][unit]") {
    CHECK(detail::resolve({.directory = "D:/Music"}).directory == "D:/Music");
    CHECK(detail::resolve({.directory = "  library  "}).directory == "library");
    CHECK(detail::resolve({.directory = ""}).directory == "components/itunes");
    CHECK(detail::resolve({.directory = "   "}).directory == "components/itunes");

    const detail::Settings custom {.directory = "keep-me"};
    CHECK(detail::resolve({}, custom).directory == "keep-me");
    CHECK(detail::resolve({.directory = ""}, custom).directory == "keep-me");
}

TEST_CASE("iTunes configuration resolves explicit values", "[itunes][config][unit]") {
    CHECK(detail::resolve({.auto_start = "on"}).auto_start);
}

TEST_CASE("iTunes auto_start accepts only lowercase on and off", "[itunes][config][unit]") {
    CHECK_FALSE(detail::resolve({.auto_start = "off"}).auto_start);
    CHECK(detail::resolve({.auto_start = "on"}).auto_start);

    CHECK(detail::resolve({.auto_start = "ON"}).auto_start);
    CHECK(detail::resolve({.auto_start = "true"}).auto_start);
    CHECK(detail::resolve({.auto_start = "false"}).auto_start);
    CHECK(detail::resolve({.auto_start = "1"}).auto_start);
    CHECK_FALSE(detail::resolve({.auto_start = "ON"}, {.auto_start = false})
        .auto_start);
    CHECK(detail::resolve({.auto_start = "ON"}, {.auto_start = true})
        .auto_start);
    CHECK_FALSE(detail::resolve({.auto_start = "off"}, {.auto_start = true})
        .auto_start);
}

TEST_CASE("iTunes directory resolves against the installation root", "[itunes][config][unit]") {
    const std::filesystem::path root {R"(C:\Example\Auto Core)"};
    const auto fallback = (root / "components" / "itunes").lexically_normal();

    CHECK(detail::resolve_directory(std::nullopt, root) == fallback);
    CHECK(detail::resolve_directory("", root) == fallback);
    CHECK(detail::resolve_directory("   ", root) == fallback);
    CHECK(
        detail::resolve_directory("components/itunes", root) == fallback
    );
    CHECK(
        detail::resolve_directory(R"(components\itunes)", root) == fallback
    );
    CHECK(
        detail::resolve_directory(R"(D:\itunes-data)", root) ==
        std::filesystem::path {R"(D:\itunes-data)"}
    );
    CHECK(
        detail::resolve_directory("components/itunes/../library", root) ==
        (root / "components" / "library").lexically_normal()
    );
}

TEST_CASE("iTunes directory resolution does not create the directory", "[itunes][config][unit]") {
    const auto root =
        std::filesystem::temp_directory_path() / "ac_itunes_directory_resolution";
    std::error_code error;
    std::filesystem::remove_all(root, error);

    const auto resolved = detail::resolve_directory("components/itunes", root);

    CHECK(resolved == (root / "components" / "itunes").lexically_normal());
    CHECK_FALSE(std::filesystem::exists(resolved));
    CHECK_FALSE(std::filesystem::exists(root));
}
