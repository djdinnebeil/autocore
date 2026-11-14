module;

#include <Windows.h>

module itunes_db_client;

import std;
import itunes_component;
import itunes_sqlite;

namespace {

struct Service {
    std::mutex mutex;
    std::optional<itunes::sqlite::Store> store;
    std::string startup_error;
};

Service& service() {
    static Service instance;
    return instance;
}

std::string local_timestamp() {
    SYSTEMTIME time {};
    GetLocalTime(&time);
    return std::format(
        "{:04}-{:02}-{:02} {:02}:{:02}:{:02}",
        time.wYear,
        time.wMonth,
        time.wDay,
        time.wHour,
        time.wMinute,
        time.wSecond
    );
}

} // namespace

namespace itunes::db {

std::expected<void, std::string> start_service() {
    auto& state = service();
    const std::lock_guard lock {state.mutex};
    if (state.store) {
        return {};
    }
    state.startup_error.clear();
    auto opened = itunes::sqlite::open();
    if (!opened) {
        state.startup_error = opened.error();
        itunes_component.log_alert("{}", state.startup_error);
        return std::unexpected(state.startup_error);
    }
    state.store = std::move(*opened);
    itunes_component.log_info("Opened {}", itunes::sqlite::file_path().string());
    return {};
}

std::expected<void, std::string> observe(const Observation& observation) {
    auto& state = service();
    const std::lock_guard lock {state.mutex};
    if (!state.store) {
        if (!state.startup_error.empty()) {
            return std::unexpected(state.startup_error);
        }
        return std::unexpected("iTunes database is not open.");
    }
    return state.store->observe(observation);
}

void shutdown_service() {
    auto& state = service();
    const std::lock_guard lock {state.mutex};
    if (!state.store) {
        return;
    }
    if (const auto closed = state.store->close_current(local_timestamp()); !closed) {
        itunes_component.log_alert("{}", closed.error());
    }
    itunes_component.log_info("Closed {}", itunes::sqlite::file_path().string());
    state.store.reset();
}

} // namespace itunes::db
