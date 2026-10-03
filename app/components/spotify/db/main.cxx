import std;
import auto_core.core.component;
import auto_core.core.ini;
import auto_core.core.logging.config;
import auto_core.core.paths;
import auto_core.core.pipes;
import spotify_db_protocol;
import spotify_sqlite;

import <iostream>;
import <Windows.h>;
import auto_core.core.shell;

namespace {

ac::Component spotify_db {
    "spotify_db",
    ac::logging::config::LoggingScope {"spotify"}
};

constexpr std::wstring_view serve_mutex_name = L"Local\\AutoCoreSpotifyDbServe";

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

void print_error(const std::string& message) {
    spotify_db.log_print("{}", message);
}

bool database_exists() {
    std::error_code error;
    return std::filesystem::exists(spotify::sqlite::file_path(), error) && !error;
}

int run_seed() {
    const auto ini_path = ac::paths::config_directory() / "spotify.ini";
    if (!ac::ini::read(ini_path)) {
        spotify_db.log_print(
            "config/spotify.ini is missing or unreadable. spotify_config.exe owns it."
        );
        return 1;
    }

    std::error_code error;
    const bool exists = std::filesystem::exists(spotify::sqlite::file_path(), error);
    if (error) {
        spotify_db.log_print(
            "Failed to inspect {}: {}",
            spotify::sqlite::file_path().string(),
            error.message()
        );
        return 1;
    }
    if (exists) {
        spotify_db.log_print("Seed skipped; history.db already exists");
        return 0;
    }

    auto store = spotify::sqlite::open();
    if (!store) {
        print_error(store.error());
        return 1;
    }
    spotify_db.log_print("Created history.db.");
    return 0;
}

void create_database() {
    auto store = spotify::sqlite::open();
    if (!store) {
        print_error(store.error());
        return;
    }
    spotify_db.log_print("Created history.db.");
}

void select_all() {
    auto store = spotify::sqlite::open();
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
        std::cout << "No tracks yet.\n";
        return;
    }
    std::cout << "id\tname\tartist\talbum\tduration\tplaycount\tcreated_at\tlast_played\n";
    for (const auto& row : *rows) {
        std::cout << std::format(
            "{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\n",
            row.id,
            row.name,
            row.artist,
            row.album,
            row.duration,
            row.playcount,
            row.created_at,
            row.last_played
        );
    }
}

int run_menu() {
    std::cout << "Database: " << spotify::sqlite::file_path().string() << '\n';
    while (true) {
        const bool exists = database_exists();
        std::cout << "\nspotify database\n";
        if (exists) {
            std::cout << "  1. select *\n";
        }
        else {
            std::cout << "  1. Create history.db\n";
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

int run_serve() {
    HANDLE mutex = CreateMutexW(nullptr, TRUE, serve_mutex_name.data());
    if (mutex == nullptr) {
        spotify_db.log_print("Unable to create the Spotify database service mutex.");
        return 1;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(mutex);
        spotify_db.log_print("Spotify database service is already running.");
        return 1;
    }
    struct MutexGuard {
        HANDLE handle;
        ~MutexGuard() {
            ReleaseMutex(handle);
            CloseHandle(handle);
        }
    } mutex_guard {mutex};

    auto store = spotify::sqlite::open();
    if (!store) {
        spotify_db.log_print("{}", store.error());
        return 1;
    }

    auto server = ac::pipes::create_pipe_server(std::wstring {spotify::db::pipe_name});
    if (!server) {
        spotify_db.log_print(
            "Unable to open the Spotify database pipe. Error: {}",
            server.error().system_error
        );
        return 1;
    }
    ac::pipes::Pipe pipe = std::move(*server);
    if (ConnectNamedPipe(static_cast<HANDLE>(pipe.native_handle()), nullptr) == FALSE &&
        GetLastError() != ERROR_PIPE_CONNECTED) {
        spotify_db.log_print(
            "Unable to accept a Spotify database client. Error: {}",
            GetLastError()
        );
        return 1;
    }

    const auto hello = ac::pipes::read_string(pipe);
    if (!hello || *hello != spotify::db::protocol_id) {
        (void)send_status(pipe, "protocol", "Unsupported Spotify database protocol.");
        spotify_db.log_print("Rejected a Spotify database client using an unexpected protocol.");
        return 1;
    }
    if (!send_status(pipe, "ok", {})) {
        spotify_db.log_print("Unable to accept the Spotify database client.");
        return 1;
    }

    ac::pipes::CommandDispatcher dispatcher;
    dispatcher.set_command(
        spotify::db::to_wire(spotify::db::Request::record_play),
        [&pipe, &store, &dispatcher] {
            std::string fields[4];
            for (std::string& field : fields) {
                auto value = ac::pipes::read_string(pipe);
                if (!value) {
                    spotify_db.log_print("Failed to read a Spotify history request.");
                    dispatcher.request_stop();
                    return;
                }
                field = std::move(*value);
            }
            int duration = 0;
            const auto parsed = std::from_chars(
                fields[3].data(),
                fields[3].data() + fields[3].size(),
                duration
            );
            if (parsed.ec != std::errc {} ||
                parsed.ptr != fields[3].data() + fields[3].size()) {
                (void)send_status(pipe, "error", "Duration must be a whole number of seconds.");
                return;
            }
            const auto recorded = store->record_play(fields[0], fields[1], fields[2], duration);
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
        spotify::db::to_wire(spotify::db::Request::shutdown),
        [&dispatcher] {
            spotify_db.log_main("shutdown signal received");
            dispatcher.request_stop();
        }
    );

    if (const auto result = dispatcher.process(pipe); !result) {
        spotify_db.log_print(
            "Spotify database pipe failed. Error: {}",
            result.error().system_error
        );
        return 1;
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    spotify_db.log_main("spotify_db.exe started");
    if (argc > 2) {
        std::cerr << "Usage: spotify_db.exe [--serve | --seed]\n";
        return 1;
    }
    if (argc == 2) {
        const std::string_view argument {argv[1]};
        if (argument == "--serve") {
            return run_serve();
        }
        if (argument == "--seed") {
            return run_seed();
        }
        std::cerr << "Usage: spotify_db.exe [--serve | --seed]\n";
        return 1;
    }
    return run_menu();
}
