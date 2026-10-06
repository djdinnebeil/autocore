#include "catch_amalgamated.hpp"
#include "../runtime/itunes_formatting_detail.hpp"
#include "../shared/library_format_detail.hpp"

namespace detail = itunes::formatting::detail;
namespace library = itunes::library_format::detail;

namespace {

library::CompiledLibraryFormat compiled(std::string_view text) {
    return *library::compile_library_format(text);
}

} // namespace

TEST_CASE("iTunes queue formatting keeps one column", "[itunes][formatting][unit]") {
    CHECK(
        detail::format_queue_item("Song\tArtist\tAlbum", compiled("[column]"), 1) ==
        "[Song]"
    );
}

TEST_CASE("iTunes queue formatting keeps two columns", "[itunes][formatting][unit]") {
    CHECK(
        detail::format_queue_item("Song\tArtist\tAlbum", compiled("[column]"), 2) ==
        "[Song] [Artist]"
    );
}

TEST_CASE("iTunes queue formatting keeps three columns", "[itunes][formatting][unit]") {
    CHECK(
        detail::format_queue_item("Song\tArtist\tAlbum", compiled("[column]"), 3) ==
        "[Song] [Artist] [Album]"
    );
}

TEST_CASE("iTunes queue formatting ignores columns after column_count", "[itunes][formatting][unit]") {
    CHECK(
        detail::format_queue_item(
            "Song\tArtist\tAlbum\tIgnored",
            compiled("[column]"),
            3
        ) == "[Song] [Artist] [Album]"
    );
}

TEST_CASE("iTunes queue formatting keeps a short row", "[itunes][formatting][unit]") {
    CHECK(
        detail::format_queue_item("Song\tArtist", compiled("[column]"), 3) ==
        "[Song] [Artist]"
    );
    CHECK(detail::format_queue_item("Song", compiled("[column]"), 3) == "[Song]");
    CHECK(detail::format_queue_item("", compiled("[column]"), 3) == "[]");
}

TEST_CASE("iTunes queue formatting removes carriage returns", "[itunes][formatting][unit]") {
    CHECK(
        detail::format_queue_item("Song\r\tArtist\r", compiled("[column]"), 3) ==
        "[Song] [Artist]"
    );
}

TEST_CASE("iTunes queue formatting surrounds columns with the chosen symbols", "[itunes][formatting][unit]") {
    CHECK(
        detail::format_queue_item("Song\tArtist\tAlbum", compiled("{column}"), 3) ==
        "{Song} {Artist} {Album}"
    );
}

TEST_CASE("iTunes queue formatting joins columns with a separator", "[itunes][formatting][unit]") {
    CHECK(
        detail::format_queue_item(
            "Song\tArtist\tAlbum",
            compiled("column - column"),
            3
        ) == "Song - Artist - Album"
    );
    CHECK(
        detail::format_queue_item(
            "Song\tArtist\tAlbum",
            compiled("column | column"),
            3
        ) == "Song | Artist | Album"
    );
    CHECK(
        detail::format_queue_item(
            "Song\tArtist\tAlbum",
            compiled("column - column"),
            2
        ) == "Song - Artist"
    );
}

TEST_CASE("iTunes queue formatting keeps empty columns", "[itunes][formatting][unit]") {
    CHECK(
        detail::format_queue_item("Song\t\tAlbum", compiled("[column]"), 3) ==
        "[Song] [] [Album]"
    );
    CHECK(
        detail::format_queue_item("Song\t", compiled("[column]"), 3) ==
        "[Song] []"
    );
    CHECK(
        detail::format_queue_item("\tArtist", compiled("[column]"), 3) ==
        "[] [Artist]"
    );
    CHECK(detail::format_queue_item("", compiled("column - column"), 3).empty());
}
