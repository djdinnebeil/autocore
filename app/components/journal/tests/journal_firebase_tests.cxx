#include "catch_amalgamated.hpp"

import journal_firebase;

#include <filesystem>

namespace {

std::filesystem::path temp_url() {
    const auto directory = std::filesystem::temp_directory_path() / "ac_journal_firebase_test";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    return directory / "firebase.id";
}

} // namespace

TEST_CASE("Firebase payload is a name and the next count", "[journal][unit]") {
    CHECK(journal::firebase::payload("Auto Core", 6) ==
        "{\"name\":\"Auto Core\",\"count\":6}");
    CHECK(journal::firebase::payload("A \"quoted\"", 1) ==
        "{\"name\":\"A \\\"quoted\\\"\",\"count\":1}");
}

TEST_CASE("firebase.id rejects blank input and replaces a URL", "[journal][unit]") {
    const auto path = temp_url();
    CHECK_FALSE(journal::firebase::read_url(path).has_value());
    CHECK_FALSE(journal::firebase::write_url(path, "  \n").has_value());
    REQUIRE(journal::firebase::write_url(
        path,
        "  https://example-default-rtdb.firebaseio.com/journal/current.json  "
    ));
    const auto current = journal::firebase::read_url(path);
    REQUIRE(current.has_value());
    CHECK(*current == "https://example-default-rtdb.firebaseio.com/journal/current.json");

    const std::string existing_menu = journal::firebase::menu_text(current);
    CHECK(existing_menu.find("Current Firebase URL:") != std::string::npos);
    CHECK(existing_menu.find(*current) != std::string::npos);
    CHECK(existing_menu.find("1. Add new Firebase URL\n2. Exit\n") != std::string::npos);

    REQUIRE(journal::firebase::write_url(path, "https://example.invalid/next.json"));
    CHECK(journal::firebase::read_url(path) == "https://example.invalid/next.json");
    {
        std::ifstream written(path, std::ios::binary);
        std::ostringstream stored;
        stored << written.rdbuf();
        CHECK(stored.str() == "https://example.invalid/next.json\n");
    }
    {
        std::ofstream extra(path, std::ios::binary | std::ios::trunc);
        extra << "https://example.invalid/next.json\nhttps://example.invalid/other.json\n";
    }
    CHECK_FALSE(journal::firebase::read_url(path).has_value());

    REQUIRE(journal::firebase::write_blank(path));
    CHECK(std::filesystem::file_size(path) == 0);
    CHECK_FALSE(journal::firebase::read_url(path).has_value());

    const std::string missing_menu = journal::firebase::menu_text(std::nullopt);
    CHECK(missing_menu.find("1. Add Firebase URL\n2. Exit\n") != std::string::npos);
    CHECK(missing_menu.ends_with("2. Exit\n"));
}
