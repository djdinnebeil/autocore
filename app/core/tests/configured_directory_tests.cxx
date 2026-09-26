#include "catch_amalgamated.hpp"
#include "../src/installation_layout.hpp"
#include "../src/configured_directory.hpp"

#include <filesystem>

TEST_CASE(
    "Image path under bin yields installation root as parent",
    "[installation-layout][unit]"
) {
    const auto layout = ac::paths::detail::layout_from_image_path(
        std::filesystem::path {R"(C:\Example\Auto Core\bin\auto_core.exe)"}
    );

    CHECK(layout.bin_directory == std::filesystem::path {R"(C:\Example\Auto Core\bin)"});
    CHECK(layout.installation_root == std::filesystem::path {R"(C:\Example\Auto Core)"});
}

TEST_CASE(
    "Taskbar relative directory joins the installation root not bin",
    "[configured-directory][unit]"
) {
    const auto layout = ac::paths::detail::layout_from_image_path(
        std::filesystem::path {R"(C:\Example\Auto Core\bin\auto_core.exe)"}
    );
    const auto directory = ac::paths::detail::resolve_configured_directory(
        std::filesystem::path {"taskbar"},
        layout.installation_root / "taskbar",
        layout.installation_root
    );

    CHECK(directory == std::filesystem::path {R"(C:\Example\Auto Core\taskbar)"});
    CHECK(directory != layout.bin_directory / "taskbar");
}

TEST_CASE(
    "Component journal directory joins installation root",
    "[configured-directory][unit]"
) {
    const auto layout = ac::paths::detail::layout_from_image_path(
        std::filesystem::path {R"(C:\Example\Auto Core\bin\journal_ac.exe)"}
    );
    const auto directory = ac::paths::detail::resolve_configured_directory(
        std::filesystem::path {R"(components\journal)"},
        layout.installation_root / "components" / "journal",
        layout.installation_root
    );

    CHECK(
        directory ==
        std::filesystem::path {R"(C:\Example\Auto Core\components\journal)"}
    );
}

TEST_CASE(
    "Configured directory uses an explicit absolute path",
    "[configured-directory][unit]"
) {
    const auto directory = ac::paths::detail::resolve_configured_directory(
        std::filesystem::path {R"(D:\journal-data)"},
        R"(C:\default-journal)",
        R"(C:\Example\Auto Core)"
    );

    CHECK(directory == std::filesystem::path {R"(D:\journal-data)"});
}

TEST_CASE(
    "Missing configured directory uses the default",
    "[configured-directory][unit]"
) {
    const auto directory = ac::paths::detail::resolve_configured_directory(
        std::nullopt,
        R"(C:\default-journal)",
        R"(C:\Example\Auto Core)"
    );

    CHECK(directory == std::filesystem::path {R"(C:\default-journal)"});
}

TEST_CASE(
    "Empty configured directory uses the default",
    "[configured-directory][unit]"
) {
    const auto directory = ac::paths::detail::resolve_configured_directory(
        std::filesystem::path {""},
        R"(C:\default-journal)",
        R"(C:\Example\Auto Core)"
    );

    CHECK(directory == std::filesystem::path {R"(C:\default-journal)"});
}

TEST_CASE(
    "Relative configured directory joins the installation root",
    "[configured-directory][unit]"
) {
    const auto directory = ac::paths::detail::resolve_configured_directory(
        std::filesystem::path {"journal"},
        R"(C:\default-journal)",
        R"(C:\Example\Auto Core)"
    );

    CHECK(directory == std::filesystem::path {R"(C:\Example\Auto Core\journal)"});
}

TEST_CASE(
    "Relative configured directory is lexically normalized",
    "[configured-directory][unit]"
) {
    const auto directory = ac::paths::detail::resolve_configured_directory(
        std::filesystem::path {R"(journal\cache\..\data)"},
        R"(C:\default-journal)",
        R"(C:\Example\Auto Core)"
    );

    CHECK(directory == std::filesystem::path {R"(C:\Example\Auto Core\journal\data)"});
}
