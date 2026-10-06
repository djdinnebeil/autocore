#include "catch_amalgamated.hpp"
#include "../config/installation_layout.hpp"
#include "../config/configured_directory.hpp"

#include <filesystem>
#include <fstream>
#include <string>

import auto_core.core.paths;

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
    "Relative directory joins the installation root",
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

namespace {

std::filesystem::path temp_ini(const std::string_view name) {
    const auto directory = std::filesystem::temp_directory_path() /
        "ac_configured_directory" / name;
    std::error_code error;
    std::filesystem::remove_all(directory, error);
    std::filesystem::create_directories(directory);
    return directory / "sample.ini";
}

void write_bytes(
    const std::filesystem::path& path,
    const std::string_view bytes
) {
    std::ofstream output {path, std::ios::binary | std::ios::trunc};
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    REQUIRE(output.good());
}

const std::filesystem::path fallback {R"(C:\configured-directory-fallback)"};

} // namespace

TEST_CASE(
    "Configured directory uses the fallback when the INI is missing",
    "[configured-directory][unit]"
) {
    const auto& directory = ac::paths::configured_directory(
        temp_ini("missing"),
        "sample",
        "directory",
        fallback
    );

    CHECK(directory == fallback);
}

TEST_CASE(
    "Configured directory uses the fallback when the key is missing",
    "[configured-directory][unit]"
) {
    const auto ini = temp_ini("missing-key");
    write_bytes(ini, "[sample]\nother = value\n");

    const auto& directory = ac::paths::configured_directory(
        ini,
        "sample",
        "directory",
        fallback
    );

    CHECK(directory == fallback);
}

TEST_CASE(
    "Configured directory uses the fallback when the value is empty",
    "[configured-directory][unit]"
) {
    const auto ini = temp_ini("empty");
    write_bytes(ini, "[sample]\ndirectory =\n");

    const auto& directory = ac::paths::configured_directory(
        ini,
        "sample",
        "directory",
        fallback
    );

    CHECK(directory == fallback);
}

TEST_CASE(
    "Configured directory joins a relative value to the installation root",
    "[configured-directory][unit]"
) {
    const auto ini = temp_ini("relative");
    write_bytes(ini, "[sample]\ndirectory = configured\\child\n");

    const auto& directory = ac::paths::configured_directory(
        ini,
        "sample",
        "directory",
        fallback
    );

    CHECK(
        directory ==
        (ac::paths::installation_root() / "configured" / "child").lexically_normal()
    );
}

TEST_CASE(
    "Configured directory keeps an absolute value",
    "[configured-directory][unit]"
) {
    const auto ini = temp_ini("absolute");
    write_bytes(ini, "[sample]\ndirectory = D:\\ac_configured_absolute\n");

    const auto& directory = ac::paths::configured_directory(
        ini,
        "sample",
        "directory",
        fallback
    );

    CHECK(directory == std::filesystem::path {R"(D:\ac_configured_absolute)"});
}

TEST_CASE(
    "Configured directory lexically normalizes a relative value",
    "[configured-directory][unit]"
) {
    const auto ini = temp_ini("normalized");
    write_bytes(ini, "[sample]\ndirectory = configured\\cache\\..\\data\n");

    const auto& directory = ac::paths::configured_directory(
        ini,
        "sample",
        "directory",
        fallback
    );

    CHECK(
        directory ==
        (ac::paths::installation_root() / "configured" / "data").lexically_normal()
    );
}

TEST_CASE(
    "Configured directory uses the fallback for invalid UTF-8",
    "[configured-directory][unit]"
) {
    const auto ini = temp_ini("invalid-utf8");
    write_bytes(ini, std::string {"[sample]\ndirectory = \xFF\n"});

    const auto& directory = ac::paths::configured_directory(
        ini,
        "sample",
        "directory",
        fallback
    );

    CHECK(directory == fallback);
}

TEST_CASE(
    "Configured directory keeps the first resolved path for the process",
    "[configured-directory][unit]"
) {
    const auto ini = temp_ini("cached");
    write_bytes(ini, "[sample]\ndirectory = configured\\first\n");

    const auto& first = ac::paths::configured_directory(
        ini,
        "sample",
        "directory",
        fallback
    );
    const auto& second = ac::paths::configured_directory(
        ini,
        "sample",
        "directory",
        fallback
    );
    write_bytes(ini, "[sample]\ndirectory = D:\\ac_configured_changed\n");
    const auto& third = ac::paths::configured_directory(
        ini,
        "sample",
        "directory",
        fallback
    );

    CHECK(&first == &second);
    CHECK(&first == &third);
    CHECK(
        first ==
        (ac::paths::installation_root() / "configured" / "first").lexically_normal()
    );
}
