#include "catch_amalgamated.hpp"
#include "../../components/logger/main/logger_session_detail.hpp"
#include "../../components/logger/main/merge_detail.hpp"
#include "../src/console_route_detail.hpp"

#include <fstream>

namespace {

    std::filesystem::path test_root() {
        auto root = std::filesystem::temp_directory_path() /
            "auto_core_main_log_merge";
        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "components");
        return root;
    }

    void write_log(
        const std::filesystem::path& root,
        const std::string& component,
        const std::string& body
    ) {
        const auto directory = root / "components" / component;
        std::filesystem::create_directories(directory);
        std::ofstream output(
            directory / ("2026-09-25_" + component + ".main.log"),
            std::ios::binary | std::ios::app
        );
        output << body;
    }

    std::string read_text(const std::filesystem::path& path) {
        std::ifstream input(path, std::ios::binary);
        return {
            std::istreambuf_iterator<char> {input},
            std::istreambuf_iterator<char> {}
        };
    }

}

TEST_CASE("First merge uses the parent directory and appends once", "[logging][merge]") {
    const auto root = test_root();
    write_log(
        root,
        "spotify",
        "[2026-09-25 11:42:10.123] spotify_ac.exe started\n"
    );
    write_log(
        root,
        "writer",
        "[2026-09-25 11:42:09.100] writer_ac.exe started\n"
    );

    REQUIRE(ac::logger::detail::merge_once(root).ok);
    const auto merged = read_text(root / "2026-09-25_main.log");
    CHECK(merged ==
        "[2026-09-25 11:42:09.100] [writer] writer_ac.exe started\n"
        "[2026-09-25 11:42:10.123] [spotify] spotify_ac.exe started\n");

    REQUIRE(ac::logger::detail::merge_once(root).ok);
    CHECK(read_text(root / "2026-09-25_main.log") == merged);

    write_log(
        root,
        "spotify",
        "[2026-09-25 11:42:11.000] next\n"
    );
    REQUIRE(ac::logger::detail::merge_once(root).ok);
    CHECK(read_text(root / "2026-09-25_main.log") ==
        merged + "[2026-09-25 11:42:11.000] [spotify] next\n");
    CHECK(read_text(root / "merge.state").find(
        "components/spotify/2026-09-25_spotify.main.log=") !=
        std::string::npos);
}

TEST_CASE("Component identity comes from the parent directory", "[logging][merge]") {
    const auto root = test_root();
    const auto directory = root / "components" / "spotify_star";
    std::filesystem::create_directories(directory);
    std::ofstream output(
        directory / "2026-09-25_not_the_component.main.log",
        std::ios::binary
    );
    output << "[2026-09-25 11:42:10.123] started\n";
    output.close();

    REQUIRE(ac::logger::detail::merge_once(root).ok);
    CHECK(read_text(root / "2026-09-25_main.log") ==
        "[2026-09-25 11:42:10.123] [spotify_star] started\n");
}

TEST_CASE("Timestamp collisions keep both records in path order", "[logging][merge]") {
    const auto root = test_root();
    write_log(
        root,
        "beta",
        "[2026-09-25 11:42:10.123] from beta\n"
    );
    write_log(
        root,
        "alpha",
        "[2026-09-25 11:42:10.123] from alpha\n"
    );

    REQUIRE(ac::logger::detail::merge_once(root).ok);
    CHECK(read_text(root / "2026-09-25_main.log") ==
        "[2026-09-25 11:42:10.123] [alpha] from alpha\n"
        "[2026-09-25 11:42:10.123] [beta] from beta\n");
}

TEST_CASE("A truncated source rebuilds that date", "[logging][merge]") {
    const auto root = test_root();
    write_log(
        root,
        "spotify",
        "[2026-09-25 11:42:10.123] first\n"
        "[2026-09-25 11:42:11.123] second\n"
    );
    REQUIRE(ac::logger::detail::merge_once(root).ok);

    const auto source =
        root / "components" / "spotify" / "2026-09-25_spotify.main.log";
    {
        std::ofstream output(source, std::ios::binary | std::ios::trunc);
        output << "[2026-09-25 11:42:12.123] replaced\n";
    }
    REQUIRE(ac::logger::detail::merge_once(root).ok);
    CHECK(read_text(root / "2026-09-25_main.log") ==
        "[2026-09-25 11:42:12.123] [spotify] replaced\n");
}

TEST_CASE("A missing merged file is rebuilt from sources", "[logging][merge]") {
    const auto root = test_root();
    write_log(
        root,
        "writer",
        "[2026-09-25 11:42:09.100] kept\n"
    );
    REQUIRE(ac::logger::detail::merge_once(root).ok);
    std::filesystem::remove(root / "2026-09-25_main.log");
    REQUIRE(ac::logger::detail::merge_once(root).ok);
    CHECK(read_text(root / "2026-09-25_main.log") ==
        "[2026-09-25 11:42:09.100] [writer] kept\n");
}

TEST_CASE("Bytes past the snapshot stay unconsumed", "[logging][merge]") {
    const auto path = test_root() / "partial.main.log";
    const std::string body =
        "[2026-09-25 11:42:10.123] complete\n"
        "[2026-09-25 11:42:11.123] partial";
    {
        std::ofstream output(path, std::ios::binary);
        output << body;
    }
    const auto end_offset = std::string {
        "[2026-09-25 11:42:10.123] complete\n"
    }.size();
    const auto read = ac::logger::detail::read_records(
        path,
        "spotify",
        "components/spotify/partial.main.log",
        0,
        end_offset
    );
    REQUIRE(read.records.size() == 1);
    CHECK(read.records[0].line ==
        "[2026-09-25 11:42:10.123] [spotify] complete");
    CHECK(read.consumed_offset == end_offset);
}

TEST_CASE("Failed merged output does not write state", "[logging][merge]") {
    const auto root = test_root();
    write_log(
        root,
        "spotify",
        "[2026-09-25 11:42:10.123] blocked\n"
    );
    std::filesystem::create_directory(root / "2026-09-25_main.log");
    const auto result = ac::logger::detail::merge_once(root);
    CHECK_FALSE(result.ok);
    CHECK_FALSE(std::filesystem::exists(root / "merge.state"));
}

TEST_CASE("Manual mode is only the absent pipe", "[logging][merge]") {
    using ac::logger::detail::LaunchKind;
    CHECK(ac::logger::detail::classify_pipe_open(true, 0) ==
        LaunchKind::hosted);
    CHECK(ac::logger::detail::classify_pipe_open(
            false,
            ac::logger::detail::pipe_absent_error
        ) == LaunchKind::manual);
    CHECK(ac::logger::detail::classify_pipe_open(false, 5) ==
        LaunchKind::failed);
}

TEST_CASE("Interval zero does not schedule a periodic merge", "[logging][merge]") {
    const auto disabled = ac::logger::detail::schedule_for(0);
    const auto active = ac::logger::detail::schedule_for(60);
    CHECK_FALSE(disabled.merge_before_wait);
    CHECK(disabled.wait_forever);
    CHECK(active.merge_before_wait);
    CHECK_FALSE(active.wait_forever);
    CHECK(ac::logger::detail::interval_milliseconds(1) == 1000);
    CHECK(ac::logger::detail::interval_milliseconds(60) == 60000);
    CHECK(ac::logger::detail::interval_milliseconds(
            (ac::logger::detail::max_wait_milliseconds / 1000) + 1
        ) ==
        (ac::logger::detail::max_wait_milliseconds / 1000) * 1000);
    CHECK(ac::logger::detail::classify_wait(
            ac::logger::detail::wait_object_0
        ) == ac::logger::detail::WaitOutcome::shutdown);
    CHECK(ac::logger::detail::classify_wait(
            ac::logger::detail::wait_timeout
        ) == ac::logger::detail::WaitOutcome::interval);
    CHECK(ac::logger::detail::classify_wait(
            ac::logger::detail::wait_failed
        ) == ac::logger::detail::WaitOutcome::failed);
    CHECK(ac::logger::detail::classify_wait(0x80) ==
        ac::logger::detail::WaitOutcome::unexpected);
    CHECK_FALSE(ac::logger::detail::hosted_shutdown_merges());
    CHECK(ac::logger::detail::is_once_argument(L"--once"));
    CHECK_FALSE(ac::logger::detail::is_once_argument(L"--merge"));
    CHECK(ac::logger::detail::is_shutdown_argument(L"--shutdown"));
    CHECK_FALSE(ac::logger::detail::is_shutdown_argument(L"--once"));
}

TEST_CASE("log and log_main reach the console only through the flag", "[logging][console]") {
    using ac::component_detail::console_write_count;
    CHECK(console_write_count(false, false) == 0);
    CHECK(console_write_count(false, true) == 1);
    CHECK(console_write_count(true, false) == 1);
    CHECK(console_write_count(true, true) == 1);
}
