#include "catch_amalgamated.hpp"
#include "../shared/wake_history_detail.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
namespace history = wake::history;

namespace {

fs::path make_directory() {
    static unsigned sequence = 0;
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = fs::temp_directory_path() /
        ("ac-wake-history-" + std::to_string(stamp) + "-" + std::to_string(++sequence));
    fs::remove_all(path);
    fs::create_directories(path);
    return path;
}

class TempHistory {
public:
    TempHistory() : path {make_directory()} {}
    ~TempHistory() {
        std::error_code error;
        fs::remove_all(path, error);
    }

    fs::path path;
};

void write_text(const fs::path& file, const std::string_view contents) {
    std::ofstream output(file, std::ios::binary | std::ios::trunc);
    REQUIRE(output);
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

std::string read_text(const fs::path& file) {
    std::ifstream input(file, std::ios::binary);
    REQUIRE(input);
    return std::string {
        std::istreambuf_iterator<char> {input},
        std::istreambuf_iterator<char> {}
    };
}

fs::path current_file(const fs::path& directory) {
    return directory / std::string {history::current_event_name};
}

fs::path previous_file(const fs::path& directory) {
    return directory / std::string {history::previous_event_name};
}

fs::path log_file(const fs::path& directory) {
    return directory / std::string {history::events_log_name};
}

} // namespace

TEST_CASE("first Wake capture records one baseline event", "[wake][history][unit]") {
    const TempHistory history_dir;
    const std::string stamp = "2026-10-03 at 02:28:00";

    const auto result = history::apply_capture(
        history_dir.path,
        "Wake History Count - 1",
        stamp
    );

    CHECK(result == history::ApplyResult::recorded);
    CHECK(read_text(current_file(history_dir.path)) == "Wake History Count - 1\n");
    CHECK(read_text(previous_file(history_dir.path)) == "Wake History Count - 1\n");
    CHECK(read_text(log_file(history_dir.path)) ==
        stamp + "\nWake History Count - 1\n");
}

TEST_CASE("an identical Wake capture does not append", "[wake][history][unit]") {
    const TempHistory history_dir;
    const std::string stamp = "2026-10-03 at 02:28:00";
    REQUIRE(history::apply_capture(history_dir.path, "same\n", stamp) ==
        history::ApplyResult::recorded);
    const auto log_before = read_text(log_file(history_dir.path));

    const auto result = history::apply_capture(
        history_dir.path,
        "same",
        "2026-10-03 at 02:29:00"
    );

    CHECK(result == history::ApplyResult::unchanged);
    CHECK(read_text(current_file(history_dir.path)) == "same\n");
    CHECK(read_text(previous_file(history_dir.path)) == "same\n");
    CHECK(read_text(log_file(history_dir.path)) == log_before);
}

TEST_CASE("a different Wake capture appends one event", "[wake][history][unit]") {
    const TempHistory history_dir;
    REQUIRE(history::apply_capture(
        history_dir.path,
        "first\n",
        "2026-10-03 at 02:28:00"
    ) == history::ApplyResult::recorded);

    const auto result = history::apply_capture(
        history_dir.path,
        "second",
        "2026-10-03 at 02:29:00"
    );

    CHECK(result == history::ApplyResult::recorded);
    CHECK(read_text(current_file(history_dir.path)) == "second\n");
    CHECK(read_text(previous_file(history_dir.path)) == "second\n");
    CHECK(read_text(log_file(history_dir.path)) ==
        "2026-10-03 at 02:28:00\nfirst\n"
        "2026-10-03 at 02:29:00\nsecond\n");
}

TEST_CASE("a missing previous event does not duplicate the log", "[wake][history][unit]") {
    const TempHistory history_dir;
    const std::string existing = "2026-10-03 at 01:00:00\nkept\n";
    write_text(log_file(history_dir.path), existing);

    const auto result = history::apply_capture(
        history_dir.path,
        "kept",
        "2026-10-03 at 02:28:00"
    );

    CHECK(result == history::ApplyResult::unchanged);
    CHECK(read_text(log_file(history_dir.path)) == existing);
    CHECK(read_text(previous_file(history_dir.path)) == "kept\n");
    CHECK(read_text(current_file(history_dir.path)) == "kept\n");
}

TEST_CASE("a missing log with a matching baseline is created empty", "[wake][history][unit]") {
    const TempHistory history_dir;
    write_text(previous_file(history_dir.path), "same\n");
    write_text(current_file(history_dir.path), "stale");

    const auto result = history::apply_capture(
        history_dir.path,
        "same",
        "2026-10-03 at 02:28:00"
    );

    CHECK(result == history::ApplyResult::unchanged);
    CHECK(read_text(log_file(history_dir.path)).empty());
    CHECK(read_text(previous_file(history_dir.path)) == "same\n");
    CHECK(read_text(current_file(history_dir.path)) == "same\n");
}

TEST_CASE("a missing log records the new body once", "[wake][history][unit]") {
    const TempHistory history_dir;
    write_text(previous_file(history_dir.path), "old\n");

    const auto result = history::apply_capture(
        history_dir.path,
        "new",
        "2026-10-03 at 02:28:00"
    );

    CHECK(result == history::ApplyResult::recorded);
    CHECK(read_text(log_file(history_dir.path)) ==
        "2026-10-03 at 02:28:00\nnew\n");
    CHECK(read_text(previous_file(history_dir.path)) == "new\n");
}

TEST_CASE("an empty capture creates missing files and keeps current", "[wake][history][unit]") {
    const TempHistory history_dir;
    write_text(current_file(history_dir.path), "keep\n");

    REQUIRE(history::create_missing_history_files(history_dir.path));

    CHECK(read_text(current_file(history_dir.path)) == "keep\n");
    CHECK(read_text(previous_file(history_dir.path)).empty());
    CHECK(read_text(log_file(history_dir.path)).empty());
    CHECK(history::all_history_files_present(history_dir.path));

    REQUIRE(history::create_missing_history_files(history_dir.path));
    CHECK(read_text(current_file(history_dir.path)) == "keep\n");
    CHECK(read_text(previous_file(history_dir.path)).empty());
}

TEST_CASE("an empty first apply does not append a placeholder", "[wake][history][unit]") {
    const TempHistory history_dir;

    const auto result = history::apply_capture(
        history_dir.path,
        "",
        "2026-10-03 at 02:28:00"
    );

    CHECK(result == history::ApplyResult::unchanged);
    CHECK(read_text(current_file(history_dir.path)).empty());
    CHECK(read_text(previous_file(history_dir.path)).empty());
    CHECK(read_text(log_file(history_dir.path)).empty());
}

TEST_CASE("an unreadable companion leaves Wake history unchanged", "[wake][history][unit]") {
    const TempHistory history_dir;
    write_text(current_file(history_dir.path), "keep\n");
    write_text(log_file(history_dir.path), "history\n");
    fs::create_directory(previous_file(history_dir.path));

    const auto result = history::apply_capture(
        history_dir.path,
        "new",
        "2026-10-03 at 02:28:00"
    );

    CHECK(result == history::ApplyResult::failed);
    CHECK(read_text(current_file(history_dir.path)) == "keep\n");
    CHECK(read_text(log_file(history_dir.path)) == "history\n");
    CHECK(fs::is_directory(previous_file(history_dir.path)));
}

TEST_CASE("an unreadable log leaves current and previous unchanged", "[wake][history][unit]") {
    const TempHistory history_dir;
    write_text(current_file(history_dir.path), "keep\n");
    write_text(previous_file(history_dir.path), "keep\n");
    fs::create_directory(log_file(history_dir.path));

    const auto result = history::apply_capture(
        history_dir.path,
        "keep",
        "2026-10-03 at 02:28:00"
    );

    CHECK(result == history::ApplyResult::failed);
    CHECK(read_text(current_file(history_dir.path)) == "keep\n");
    CHECK(read_text(previous_file(history_dir.path)) == "keep\n");
    CHECK(fs::is_directory(log_file(history_dir.path)));
}

TEST_CASE("legacy Wake names are left untouched", "[wake][history][unit]") {
    const TempHistory history_dir;
    const fs::path legacy = history_dir.path / "current.log";
    write_text(legacy, "legacy");
    write_text(history_dir.path / "wake_master.log", "master");

    REQUIRE(history::apply_capture(
        history_dir.path,
        "now",
        "2026-10-03 at 02:28:00"
    ) == history::ApplyResult::recorded);

    CHECK(read_text(legacy) == "legacy");
    CHECK(read_text(history_dir.path / "wake_master.log") == "master");
}

TEST_CASE("Wake history provision checks presence only", "[wake][history][unit]") {
    const TempHistory history_dir;

    CHECK_FALSE(history::all_history_files_present(history_dir.path));

    write_text(current_file(history_dir.path), {});
    write_text(previous_file(history_dir.path), {});
    CHECK_FALSE(history::all_history_files_present(history_dir.path));

    write_text(log_file(history_dir.path), {});
    CHECK(history::all_history_files_present(history_dir.path));

    fs::remove(log_file(history_dir.path));
    fs::create_directory(log_file(history_dir.path));
    CHECK(history::all_history_files_present(history_dir.path));
}
