import std;
import auto_core.core.component;
import auto_core.core.ini;
import auto_core.core.logging.config;
import auto_core.core.paths;
import auto_core.core.pipes;
import auto_core.core.process;
import itunes_db_protocol;
import itunes_sqlite;

import <iostream>;
import <Windows.h>;
import auto_core.core.shell;

namespace {

ac::Component itunes_db {
    "itunes_db",
    ac::logging::config::LoggingScope {"itunes"}
};

constexpr std::wstring_view serve_mutex_name = L"Local\\AutoCoreItunesDbServe";

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
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

void print_error(const std::string& message) {
    itunes_db.log_print("{}", message);
}

bool database_exists() {
    std::error_code error;
    return std::filesystem::exists(itunes::sqlite::file_path(), error) && !error;
}

void create_database() {
    auto store = itunes::sqlite::open();
    if (!store) {
        print_error(store.error());
        return;
    }
    itunes_db.log_print("Created a listening database.");
}

int run_seed() {
    const auto ini_path = ac::paths::config_directory() / "itunes.ini";
    if (!ac::ini::read(ini_path)) {
        itunes_db.log_print(
            "config/itunes.ini is missing or unreadable. itunes_config.exe owns it."
        );
        return 1;
    }

    const auto path = itunes::sqlite::file_path();
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error) {
        itunes_db.log_print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return 1;
    }
    if (exists) {
        itunes_db.log_print("Seed skipped; history.db already exists");
        return 0;
    }

    auto store = itunes::sqlite::open();
    if (!store) {
        print_error(store.error());
        return 1;
    }
    itunes_db.log_print("Created a listening database.");
    return 0;
}

void select_all() {
    auto store = itunes::sqlite::open();
    if (!store) {
        print_error(store.error());
        return;
    }
    const auto rows = store->select_all();
    if (!rows) {
        print_error(rows.error());
        return;
    }
    if (rows->empty()) {
        std::cout << "No listening history yet.\n";
        return;
    }
    std::cout
        << "id\ttrack_id\ttitle\tartist\talbum\tduration_seconds\t"
           "started_at\tlast_observed_at\tended_at\tlistened_seconds\n";
    for (const auto& row : *rows) {
        std::cout << std::format(
            "{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\n",
            row.id,
            row.track_id,
            row.title,
            row.artist,
            row.album,
            row.duration_seconds,
            row.started_at,
            row.last_observed_at,
            row.ended_at,
            row.listened_seconds
        );
    }
}

int run_menu() {
    std::cout << "Database: " << itunes::sqlite::file_path().string() << '\n';
    while (true) {
        const bool exists = database_exists();
        std::cout << "\niTunes Database\n";
        if (exists) {
            std::cout << "  1. Select *\n";
        }
        else {
            std::cout << "  1. Create a listening database\n";
        }
        std::cout
            << "  2. Exit\n"
            << "> ";
        std::string choice;
        if (!std::getline(std::cin, choice)) {
            return 0;
        }
        choice = std::string {trim(choice)};
        if (choice == "1") {
            if (exists) {
                select_all();
            }
            else {
                create_database();
            }
        }
        else if (choice == "2" || choice == "q" || choice == "Q") {
            return 0;
        }
        else {
            std::cout << "Unknown option.\n";
        }
    }
}

bool send_status(ac::pipes::Pipe& pipe, std::string_view status, std::string_view detail) {
    if (const auto sent = ac::pipes::send_string(pipe, status); !sent) {
        return false;
    }
    if (detail.empty() && status == "ok") {
        return true;
    }
    return static_cast<bool>(ac::pipes::send_string(pipe, detail));
}

std::expected<int, std::string> parse_int(std::string_view text) {
    int value = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    if (parsed.ec != std::errc {} || parsed.ptr != text.data() + text.size()) {
        return std::unexpected("Expected a whole number.");
    }
    return value;
}

std::expected<std::int64_t, std::string> parse_int64(std::string_view text) {
    std::int64_t value = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    if (parsed.ec != std::errc {} || parsed.ptr != text.data() + text.size()) {
        return std::unexpected("Expected a track id.");
    }
    return value;
}

int run_serve(void* const owner) {
    HANDLE mutex = CreateMutexW(nullptr, TRUE, serve_mutex_name.data());
    if (mutex == nullptr) {
        itunes_db.log_print("Unable to create the iTunes database service mutex.");
        return 1;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(mutex);
        itunes_db.log_print("iTunes database service is already running.");
        return 1;
    }
    struct MutexGuard {
        HANDLE handle;
        ~MutexGuard() {
            ReleaseMutex(handle);
            CloseHandle(handle);
        }
    } mutex_guard {mutex};

    auto store = itunes::sqlite::open();
    if (!store) {
        itunes_db.log_print("{}", store.error());
        return 1;
    }
    struct CloseGuard {
        itunes::sqlite::Store* database = nullptr;
        ~CloseGuard() {
            if (database == nullptr) {
                return;
            }
            if (const auto closed = database->close_current(local_timestamp()); !closed) {
                itunes_db.log_print("{}", closed.error());
            }
        }
    } close_guard {&(*store)};

    auto server = ac::pipes::create_pipe_server(std::wstring {itunes::db::pipe_name});
    if (!server) {
        itunes_db.log_print(
            "Unable to open the iTunes database pipe. Error: {}",
            server.error().system_error
        );
        return 1;
    }
    ac::pipes::Pipe pipe = std::move(*server);
    ac::pipes::CommandDispatcher dispatcher;
    ac::process::OwnerWatch owner_watch;
    if (owner != nullptr) {
        owner_watch = ac::process::OwnerWatch(owner, [&dispatcher] {
            dispatcher.request_stop();
        });
        if (!owner_watch.active()) {
            itunes_db.log_print("Unable to watch the iTunes database owner process.");
            return 1;
        }
    }
    if (ConnectNamedPipe(static_cast<HANDLE>(pipe.native_handle()), nullptr) == FALSE &&
        GetLastError() != ERROR_PIPE_CONNECTED) {
        if (dispatcher.stop_requested()) {
            return 0;
        }
        itunes_db.log_print(
            "Unable to accept an iTunes database client. Error: {}",
            GetLastError()
        );
        return 1;
    }

    const auto hello = ac::pipes::read_string(pipe);
    if (!hello || *hello != itunes::db::protocol_id) {
        (void)send_status(pipe, "protocol", "Unsupported iTunes database protocol.");
        itunes_db.log_print("Rejected an iTunes database client using an unexpected protocol.");
        return 1;
    }
    if (!send_status(pipe, "ok", {})) {
        itunes_db.log_print("Unable to accept the iTunes database client.");
        return 1;
    }

    dispatcher.set_command(
        itunes::db::to_wire(itunes::db::Request::observe),
        [&pipe, &store, &dispatcher] {
            std::string fields[7];
            for (std::string& field : fields) {
                auto value = ac::pipes::read_string(pipe);
                if (!value) {
                    itunes_db.log_print("Failed to read an iTunes listening observation.");
                    dispatcher.request_stop();
                    return;
                }
                field = std::move(*value);
            }
            const auto track_id = parse_int64(fields[0]);
            const auto duration = parse_int(fields[4]);
            const auto credit = parse_int(fields[5]);
            if (!track_id || !duration || !credit) {
                const std::string message = !track_id ? track_id.error()
                    : !duration ? duration.error()
                                : credit.error();
                (void)send_status(pipe, "error", message);
                return;
            }
            itunes::db::Observation observation;
            observation.track_id = *track_id;
            observation.title = std::move(fields[1]);
            observation.artist = std::move(fields[2]);
            observation.album = std::move(fields[3]);
            observation.duration_seconds = *duration;
            observation.credit_seconds = *credit;
            observation.observed_at = std::move(fields[6]);
            const auto recorded = store->observe(observation);
            if (!recorded) {
                (void)send_status(pipe, "error", recorded.error());
                return;
            }
            if (!send_status(pipe, "ok", {})) {
                dispatcher.request_stop();
            }
        }
    );
    dispatcher.set_command(
        itunes::db::to_wire(itunes::db::Request::shutdown),
        [&dispatcher] {
            itunes_db.log_main("shutdown signal received");
            dispatcher.request_stop();
        }
    );

    if (dispatcher.stop_requested()) {
        return 0;
    }
    if (const auto result = dispatcher.process(pipe); !result) {
        if (dispatcher.stop_requested()) {
            return 0;
        }
        itunes_db.log_print(
            "iTunes database pipe failed. Error: {}",
            result.error().system_error
        );
        return 1;
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    itunes_db.log_main("itunes_db.exe started");
    bool serve = false;
    bool seed = false;
    void* owner = nullptr;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument {argv[index]};
        if (argument == "--serve") {
            serve = true;
        }
        else if (argument == "--seed") {
            seed = true;
        }
        else if (argument == "--owner-handle") {
            if (index + 1 >= argc) {
                std::cerr << "Usage: itunes_db.exe [--serve | --seed] [--owner-handle <handle>]\n";
                return 1;
            }
            const auto handle = ac::process::parse_owner_handle(argv[++index]);
            if (!handle) {
                std::cerr << "Invalid --owner-handle.\n";
                return 1;
            }
            owner = *handle;
        }
        else {
            std::cerr << "Usage: itunes_db.exe [--serve | --seed] [--owner-handle <handle>]\n";
            return 1;
        }
    }
    if (serve == seed) {
        if (argc == 1) {
            return run_menu();
        }
        std::cerr << "Usage: itunes_db.exe [--serve | --seed] [--owner-handle <handle>]\n";
        return 1;
    }
    if (seed) {
        return run_seed();
    }
    return run_serve(owner);
}
