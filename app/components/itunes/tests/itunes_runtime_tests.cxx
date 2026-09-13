#include "catch_amalgamated.hpp"

#include <filesystem>
#include <string>
#include <vector>

import itunes_runtime;

namespace {

    class fake_automation final : public itunes::runtime::automation {
    public:
        void play_pause() override { calls.emplace_back("play_pause"); }
        void next_song() override { calls.emplace_back("next_song"); }
        void prev_song() override { calls.emplace_back("prev_song"); }
        void stop_song() override { calls.emplace_back("stop_song"); }

        std::wstring get_current_track() override {
            calls.emplace_back("get_current_track");
            return current_track;
        }

        bool has_current_track() const noexcept override {
            calls.emplace_back("has_current_track");
            return current_track_available;
        }

        std::filesystem::path remove_current_track() override {
            calls.emplace_back("remove_current_track");
            return removed_path;
        }

        mutable std::vector<std::string> calls;
        std::wstring current_track = L"[Song] [Artist] [Album] [3:00]";
        bool current_track_available = true;
        std::filesystem::path removed_path = L"C:\\Music\\song.mp3";
    };

    class fake_monitor final : public itunes::runtime::playback_monitor {
    public:
        void playback_state_changed() override { ++change_count; }
        int change_count = 0;
    };

    class fake_recycler final : public itunes::runtime::file_recycler {
    public:
        bool recycle(const std::filesystem::path& path) override {
            paths.push_back(path);
            return succeeds;
        }

        bool succeeds = true;
        std::vector<std::filesystem::path> paths;
    };

    class fake_lifecycle final : public itunes::runtime::lifecycle {
    public:
        bool initialize_attempt() override {
            ++attempt_count;
            if (results.empty()) {
                return false;
            }

            const bool result = results.front();
            results.erase(results.begin());
            initialized = result;
            return result;
        }

        bool is_initialized() const noexcept override {
            return initialized;
        }

        void shutdown() noexcept override {
            initialized = false;
            ++shutdown_count;
        }

        bool initialized = false;
        int attempt_count = 0;
        int shutdown_count = 0;
        std::vector<bool> results;
    };

} // namespace

TEST_CASE("iTunes runtime refreshes the track after playback changes", "[itunes][runtime][unit]") {
    fake_automation player;

    itunes::runtime::play_pause(player);
    CHECK(player.calls == (std::vector<std::string> {
        "play_pause", "get_current_track"
    }));

    player.calls.clear();
    itunes::runtime::previous_song(player);
    CHECK(player.calls == (std::vector<std::string> {
        "prev_song", "get_current_track"
    }));
}

TEST_CASE("iTunes runtime notifies the monitor after skipping", "[itunes][runtime][unit]") {
    fake_automation player;
    fake_monitor monitor;

    itunes::runtime::next_song(player, monitor);

    CHECK(player.calls == (std::vector<std::string> {"next_song"}));
    CHECK(monitor.change_count == 1);
}

TEST_CASE("iTunes runtime preserves the stop command sequence", "[itunes][runtime][unit]") {
    fake_automation player;

    const auto current_track = itunes::runtime::stop_song(player);

    CHECK(current_track == player.current_track);
    CHECK(player.calls == (std::vector<std::string> {
        "stop_song", "play_pause", "play_pause", "get_current_track"
    }));
}

TEST_CASE("iTunes runtime does not recycle without a current track", "[itunes][runtime][unit]") {
    fake_automation player;
    fake_recycler recycler;
    player.current_track_available = false;

    const auto result = itunes::runtime::remove_song(player, recycler);

    CHECK(result.status == itunes::runtime::removal_status::no_current_track);
    CHECK(result.path.empty());
    CHECK(recycler.paths.empty());
    CHECK(player.calls == (std::vector<std::string> {
        "get_current_track", "has_current_track"
    }));
}

TEST_CASE("iTunes runtime recycles the track removed from the library", "[itunes][runtime][unit]") {
    fake_automation player;
    fake_recycler recycler;

    const auto result = itunes::runtime::remove_song(player, recycler);

    CHECK(result.status == itunes::runtime::removal_status::recycled);
    CHECK(result.path == player.removed_path);
    REQUIRE(recycler.paths.size() == 1);
    CHECK(recycler.paths.front() == player.removed_path);
    CHECK(player.calls == (std::vector<std::string> {
        "get_current_track", "has_current_track", "remove_current_track"
    }));
}

TEST_CASE("iTunes runtime reports file recycling failure", "[itunes][runtime][unit]") {
    fake_automation player;
    fake_recycler recycler;
    recycler.succeeds = false;

    const auto result = itunes::runtime::remove_song(player, recycler);

    CHECK(result.status == itunes::runtime::removal_status::recycle_failed);
    CHECK(result.path == player.removed_path);
}

TEST_CASE("iTunes initialization returns immediately when already ready", "[itunes][lifecycle][unit]") {
    fake_lifecycle target;
    target.initialized = true;
    int wait_count = 0;

    const bool initialized = itunes::runtime::initialize_with_retry(
        target,
        {},
        [&](std::chrono::milliseconds) { ++wait_count; }
    );

    CHECK(initialized);
    CHECK(target.attempt_count == 0);
    CHECK(wait_count == 0);
}

TEST_CASE("iTunes initialization retries until an attempt succeeds", "[itunes][lifecycle][unit]") {
    fake_lifecycle target;
    target.results = {false, false, true};
    std::vector<std::chrono::milliseconds> waits;
    const itunes::runtime::retry_policy policy {
        .maximum_attempts = 5,
        .delay = std::chrono::milliseconds {40}
    };

    const bool initialized = itunes::runtime::initialize_with_retry(
        target,
        policy,
        [&](const auto delay) { waits.push_back(delay); }
    );

    CHECK(initialized);
    CHECK(target.attempt_count == 3);
    CHECK(waits == (std::vector<std::chrono::milliseconds> {
        std::chrono::milliseconds {40},
        std::chrono::milliseconds {40}
    }));
}

TEST_CASE("iTunes initialization stops at the retry limit", "[itunes][lifecycle][unit]") {
    fake_lifecycle target;
    target.results = {false, false, false, true};
    int wait_count = 0;

    const bool initialized = itunes::runtime::initialize_with_retry(
        target,
        {.maximum_attempts = 3, .delay = std::chrono::milliseconds {1}},
        [&](std::chrono::milliseconds) { ++wait_count; }
    );

    CHECK_FALSE(initialized);
    CHECK(target.attempt_count == 3);
    CHECK(wait_count == 2);
}

TEST_CASE("A zero iTunes retry budget performs no attempt", "[itunes][lifecycle][unit]") {
    fake_lifecycle target;
    int wait_count = 0;

    const bool initialized = itunes::runtime::initialize_with_retry(
        target,
        {.maximum_attempts = 0},
        [&](std::chrono::milliseconds) { ++wait_count; }
    );

    CHECK_FALSE(initialized);
    CHECK(target.attempt_count == 0);
    CHECK(wait_count == 0);
}

TEST_CASE("iTunes serial executor uses one dedicated worker", "[itunes][executor][unit]") {
    itunes::runtime::serial_executor executor;
    const auto caller = std::this_thread::get_id();

    const auto first_worker = executor.invoke([] {
        return std::this_thread::get_id();
    });
    const auto second_worker = executor.invoke([] {
        return std::this_thread::get_id();
    });

    CHECK(first_worker != caller);
    CHECK(second_worker == first_worker);
}

TEST_CASE("iTunes serial executor supports nested synchronous work", "[itunes][executor][unit]") {
    itunes::runtime::serial_executor executor;

    const int value = executor.invoke([&executor] {
        return executor.invoke([] { return 42; });
    });

    CHECK(value == 42);
}

TEST_CASE("iTunes serial executor propagates task failures", "[itunes][executor][unit]") {
    itunes::runtime::serial_executor executor;

    CHECK_THROWS_AS(
        executor.invoke([]() -> void {
            throw std::runtime_error("task failed");
        }),
        std::runtime_error
    );
}

TEST_CASE("Stopped iTunes serial executor rejects new work", "[itunes][executor][unit]") {
    itunes::runtime::serial_executor executor;
    executor.stop();

    CHECK_THROWS_AS(executor.invoke([] {}), std::logic_error);
}
