module;

module journal_db_client;

import std;
import journal_component;
import journal_sqlite;

namespace {

struct Service {
    std::mutex mutex;
    std::optional<journal::sqlite::Store> store;
    std::string startup_error;
};

Service& service() {
    static Service instance;
    return instance;
}

} // namespace

namespace journal::db {

std::expected<void, std::string> start_service() {
    auto& state = service();
    const std::lock_guard lock {state.mutex};
    if (state.store) {
        return {};
    }
    state.startup_error.clear();
    auto opened = journal::sqlite::open();
    if (!opened) {
        state.startup_error = opened.error();
        journal_component().log_alert("{}", state.startup_error);
        return std::unexpected(state.startup_error);
    }
    state.store = std::move(*opened);
    journal_component().log_info("Opened {}", journal::sqlite::file_path().string());
    return {};
}

std::expected<journal::sqlite::Episode, std::string> allocate_episode(
    const std::string_view series_key
) {
    auto& state = service();
    const std::lock_guard lock {state.mutex};
    if (!state.store) {
        if (!state.startup_error.empty()) {
            return std::unexpected(state.startup_error);
        }
        return std::unexpected("Journal database is not open.");
    }
    return state.store->allocate_episode(series_key);
}

void shutdown_service() {
    auto& state = service();
    const std::lock_guard lock {state.mutex};
    if (state.store) {
        journal_component().log_info("Closed {}", journal::sqlite::file_path().string());
        state.store.reset();
    }
}

} // namespace journal::db
