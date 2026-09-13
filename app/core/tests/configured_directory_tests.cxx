#include "catch_amalgamated.hpp"
#include "../src/configured_directory.hpp"

namespace detail = ac::paths::detail;

TEST_CASE(
    "Configured directory uses an explicit absolute path",
    "[configured-directory][unit]"
) {
    const auto directory = detail::resolve_configured_directory(
        std::filesystem::path {R"(D:\journal-data)"},
        R"(C:\default-journal)",
        R"(C:\app)"
    );

    CHECK(directory == std::filesystem::path {R"(D:\journal-data)"});
}

TEST_CASE(
    "Missing configured directory uses the default",
    "[configured-directory][unit]"
) {
    const auto directory = detail::resolve_configured_directory(
        std::nullopt,
        R"(C:\default-journal)",
        R"(C:\app)"
    );

    CHECK(directory == std::filesystem::path {R"(C:\default-journal)"});
}

TEST_CASE(
    "Empty configured directory uses the default",
    "[configured-directory][unit]"
) {
    const auto directory = detail::resolve_configured_directory(
        std::filesystem::path {""},
        R"(C:\default-journal)",
        R"(C:\app)"
    );

    CHECK(directory == std::filesystem::path {R"(C:\default-journal)"});
}

TEST_CASE(
    "Relative configured directory joins the executable directory",
    "[configured-directory][unit]"
) {
    const auto directory = detail::resolve_configured_directory(
        std::filesystem::path {"journal"},
        R"(C:\default-journal)",
        R"(C:\app)"
    );

    CHECK(directory == std::filesystem::path {R"(C:\app\journal)"});
}

TEST_CASE(
    "Relative configured directory is lexically normalized",
    "[configured-directory][unit]"
) {
    const auto directory = detail::resolve_configured_directory(
        std::filesystem::path {R"(journal\cache\..\data)"},
        R"(C:\default-journal)",
        R"(C:\app)"
    );

    CHECK(directory == std::filesystem::path {R"(C:\app\journal\data)"});
}
