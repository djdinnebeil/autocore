#include "catch_amalgamated.hpp"
#include "../itunes_formatting_detail.hpp"

namespace detail = itunes::formatting::detail;

TEST_CASE("iTunes queue formatting brackets tab-separated fields", "[itunes][formatting][unit]") {
    CHECK(
        detail::format_queue_item("Song\tArtist\tAlbum", 4) ==
        "[Song] [Artist] [Album]"
    );
}

TEST_CASE("iTunes queue formatting stops at the configured tab", "[itunes][formatting][unit]") {
    CHECK(
        detail::format_queue_item("Song\tArtist\tAlbum\tIgnored", 3) ==
        "[Song] [Artist] [Album]"
    );
    CHECK(detail::format_queue_item("Song\tArtist", 1) == "[Song]");
}

TEST_CASE("iTunes queue formatting removes carriage returns", "[itunes][formatting][unit]") {
    CHECK(
        detail::format_queue_item("Song\r\tArtist\r", 3) ==
        "[Song] [Artist]"
    );
}

TEST_CASE("iTunes queue formatting handles empty and untabbed input", "[itunes][formatting][unit]") {
    CHECK(detail::format_queue_item("", 3) == "[]");
    CHECK(detail::format_queue_item("Song", 3) == "[Song]");
}
