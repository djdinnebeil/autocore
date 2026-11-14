module;

module spotify_db_client;

import std;
import spotify_component;
import spotify_sqlite;

namespace {

struct Service {
    std::mutex mutex;
    std::optional<spotify::sqlite::Store> store;
    std::string startup_error;
};

Service& service() {
    static Service instance;
    return instance;
}

} // namespace

namespace spotify::db {

std::expected<void, std::string> start_service() {
    auto& state = service();
    const std::lock_guard lock {state.mutex};
    if (state.store) {
        return {};
    }
    state.startup_error.clear();
    auto opened = spotify::sqlite::open();
    if (!opened) {
        state.startup_error = opened.error();
        spotify_component.log_alert("{}", state.startup_error);
        return std::unexpected(state.startup_error);
    }
    state.store = std::move(*opened);
    spotify_component.log_info("Opened {}", spotify::sqlite::file_path().string());
    return {};
}

std::expected<void, std::string> record_play(
    const std::string_view name,
    const std::string_view artist,
    const std::string_view album,
    const int duration_seconds
) {
    auto& state = service();
    const std::lock_guard lock {state.mutex};
    if (!state.store) {
        if (!state.startup_error.empty()) {
            return std::unexpected(state.startup_error);
        }
        return std::unexpected("Spotify database is not open.");
    }
    return state.store->record_play(name, artist, album, duration_seconds);
}

void shutdown_service() {
    auto& state = service();
    const std::lock_guard lock {state.mutex};
    if (!state.store) {
        return;
    }
    spotify_component.log_info("Closed {}", spotify::sqlite::file_path().string());
    state.store.reset();
}

} // namespace spotify::db
