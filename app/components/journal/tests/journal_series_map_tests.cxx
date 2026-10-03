#include "catch_amalgamated.hpp"

import journal_db_protocol;
import journal_series_map;

namespace {

journal::series_map::ActiveField parse(const std::string_view text) {
    return journal::series_map::parse_active(text);
}

} // namespace

TEST_CASE("series.map active line classifies the selector", "[journal][unit]") {
    const auto named = parse("active = Auto Core\n\n[snapshot]\nAuto Core 05\n");
    CHECK(named.kind == journal::series_map::ActiveKind::named);
    CHECK(named.name == "Auto Core");

    const auto tight = parse("active=Auto Core");
    CHECK(tight.kind == journal::series_map::ActiveKind::named);
    CHECK(tight.name == "Auto Core");

    CHECK(parse("").kind == journal::series_map::ActiveKind::missing);
    CHECK(parse("active =\n").kind == journal::series_map::ActiveKind::blank);
    CHECK(parse("active =   \r\n").kind == journal::series_map::ActiveKind::blank);
    CHECK(parse("series = Auto Core\n").kind == journal::series_map::ActiveKind::malformed);
    CHECK(parse("[snapshot]\nAuto Core 05\n").kind == journal::series_map::ActiveKind::malformed);
}

TEST_CASE("Missing active sends an empty key immediately", "[journal][unit]") {
    for (const auto kind : {
             journal::series_map::ActiveKind::missing,
             journal::series_map::ActiveKind::blank,
             journal::series_map::ActiveKind::malformed
         }) {
        const auto choice = journal::series_map::choose_allocate(
            journal::series_map::ActiveField {kind, {}}
        );
        CHECK(choice.empty_key);
        CHECK(choice.name.empty());
    }

    const auto named = journal::series_map::choose_allocate(
        journal::series_map::ActiveField {
            journal::series_map::ActiveKind::named,
            "Auto Core"
        }
    );
    CHECK_FALSE(named.empty_key);
    CHECK(named.name == "Auto Core");
}

TEST_CASE("Snapshot render uses database rows and padding", "[journal][unit]") {
    const journal::series_map::SnapshotSeries rows[] {
        {"Personal", 3, 0},
        {"Auto Core", 5, 2},
        {"Research", 9, 3},
    };
    const std::string rendered = journal::series_map::render("Auto Core", rows);
    CHECK(rendered ==
        "active = Auto Core\n"
        "\n"
        "[snapshot]\n"
        "Personal 3\n"
        "Auto Core 05\n"
        "Research 009\n");

    const auto preserved = parse(rendered);
    CHECK(preserved.kind == journal::series_map::ActiveKind::named);
    CHECK(preserved.name == "Auto Core");

    const std::string stale =
        "active = Auto Core\n"
        "\n"
        "[snapshot]\n"
        "Auto Core 99\n";
    CHECK(parse(stale).name == "Auto Core");
    CHECK(journal::series_map::render("Auto Core", rows).find("99") == std::string::npos);
}

TEST_CASE("Only an unknown series is retried with an empty key", "[journal][unit]") {
    CHECK(journal::db::is_unknown_series("Unknown journal series 'Auto Core'."));
    CHECK_FALSE(journal::db::is_unknown_series(
        "No journal series yet. Add one with journal_series.exe."
    ));
    CHECK_FALSE(journal::db::is_unknown_series("The journal database is busy."));
    CHECK_FALSE(journal::db::is_unknown_series("Journal database request failed."));
}
