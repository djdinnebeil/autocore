import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.pipes;
import journal_db_protocol;
import journal_sqlite;
import components_editor_request;

import <iostream>;
import <Windows.h>;
import auto_core.core.shell;

namespace {

ac::Component journal_db {
    "journal_db",
    ac::logging::config::LoggingScope {"journal"}
};

constexpr std::wstring_view serve_mutex_name = L"Local\\AutoCoreJournalDbServe";
constexpr std::string_view seed_series_name = "Auto Core";
constexpr int seed_series_padding = 2;

std::optional<int> parse_int(const std::string_view text) {
    int value = 0;
    const auto parsed = std::from_chars(
        text.data(),
        text.data() + text.size(),
        value
    );
    if (parsed.ec != std::errc {} || parsed.ptr != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
}

bool send_status(ac::pipes::Pipe& pipe, const std::string_view status, const std::string_view detail) {
    if (const auto sent = ac::pipes::send_string(pipe, status); !sent) {
        return false;
    }
    if (detail.empty() && status == "ok") {
        return true;
    }
    return static_cast<bool>(ac::pipes::send_string(pipe, detail));
}

bool send_series(ac::pipes::Pipe& pipe, const journal::sqlite::Series& series) {
    if (const auto sent = ac::pipes::send_string(pipe, series.name); !sent) {
        return false;
    }
    if (const auto sent = ac::pipes::send_string(pipe, std::to_string(series.next_episode));
        !sent) {
        return false;
    }
    return static_cast<bool>(
        ac::pipes::send_string(pipe, std::to_string(series.padding))
    );
}

void fail_pipe(ac::pipes::CommandDispatcher& dispatcher, const std::string_view message) {
    journal_db.log_print("{}", message);
    dispatcher.request_stop();
}

int run_serve() {
    HANDLE mutex = CreateMutexW(nullptr, TRUE, serve_mutex_name.data());
    if (mutex == nullptr) {
        journal_db.log_print("Unable to create the journal database service mutex.");
        return 1;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(mutex);
        journal_db.log_print("Journal database service is already running.");
        return 1;
    }
    struct MutexGuard {
        HANDLE handle;
        ~MutexGuard() {
            ReleaseMutex(handle);
            CloseHandle(handle);
        }
    } mutex_guard {mutex};

    auto store = journal::sqlite::open();
    if (!store) {
        journal_db.log_print("{}", store.error());
        return 1;
    }

    bool shutdown = false;
    while (!shutdown) {
        auto server = ac::pipes::create_pipe_server(std::wstring {journal::db::pipe_name});
        if (!server) {
            journal_db.log_print(
                "Unable to open the journal database pipe. Error: {}",
                server.error().system_error
            );
            return 1;
        }
        ac::pipes::Pipe pipe = std::move(*server);
        if (ConnectNamedPipe(pipe.native_handle(), nullptr) == FALSE &&
            GetLastError() != ERROR_PIPE_CONNECTED) {
            journal_db.log_print(
                "Unable to accept a journal database client. Error: {}",
                GetLastError()
            );
            return 1;
        }

        const auto hello = ac::pipes::read_string(pipe);
        if (!hello) {
            journal_db.log_print(
                "Journal database client did not identify itself. Error: {}",
                hello.error().system_error
            );
            continue;
        }
        if (*hello != journal::db::protocol_id) {
            (void)send_status(pipe, "protocol", "Unsupported journal database protocol.");
            journal_db.log_print(
                "Rejected a journal database client using an unexpected protocol."
            );
            continue;
        }
        if (!send_status(pipe, "ok", {})) {
            journal_db.log_print("Unable to accept the journal database client.");
            continue;
        }

        ac::pipes::CommandDispatcher dispatcher;
        dispatcher.set_command(
            journal::db::to_wire(journal::db::Request::allocate_episode),
            [&pipe, &store, &dispatcher] {
                const auto series = ac::pipes::read_string(pipe);
                if (!series) {
                    fail_pipe(dispatcher, "Failed to read an episode request.");
                    return;
                }
                const auto episode = store->allocate_episode(*series);
                if (!episode) {
                    if (!send_status(pipe, "error", episode.error())) {
                        fail_pipe(dispatcher, "Unable to report an episode allocation error.");
                    }
                    return;
                }
                if (const auto sent = ac::pipes::send_string(pipe, "ok"); !sent) {
                    fail_pipe(dispatcher, "Unable to report an allocated episode.");
                    return;
                }
                if (const auto sent = ac::pipes::send_string(pipe, episode->name); !sent) {
                    fail_pipe(dispatcher, "Unable to report an allocated episode.");
                    return;
                }
                if (const auto sent = ac::pipes::send_string(
                        pipe,
                        std::to_string(episode->allocated)
                    );
                    !sent) {
                    fail_pipe(dispatcher, "Unable to report an allocated episode.");
                    return;
                }
                if (const auto sent = ac::pipes::send_string(
                        pipe,
                        std::to_string(episode->next_episode)
                    );
                    !sent) {
                    fail_pipe(dispatcher, "Unable to report an allocated episode.");
                    return;
                }
                if (const auto sent = ac::pipes::send_string(
                        pipe,
                        std::to_string(episode->padding)
                    );
                    !sent) {
                    fail_pipe(dispatcher, "Unable to report an allocated episode.");
                }
            }
        );
        dispatcher.set_command(
            journal::db::to_wire(journal::db::Request::list_series),
            [&pipe, &store, &dispatcher] {
                const auto series = store->list_series();
                if (!series) {
                    if (!send_status(pipe, "error", series.error())) {
                        fail_pipe(dispatcher, "Unable to list journal series.");
                    }
                    return;
                }
                if (const auto sent = ac::pipes::send_string(pipe, "ok"); !sent) {
                    fail_pipe(dispatcher, "Unable to list journal series.");
                    return;
                }
                if (const auto sent = ac::pipes::send_string(
                        pipe,
                        std::to_string(series->size())
                    );
                    !sent) {
                    fail_pipe(dispatcher, "Unable to list journal series.");
                    return;
                }
                for (const auto& entry : *series) {
                    if (!send_series(pipe, entry)) {
                        fail_pipe(dispatcher, "Unable to list journal series.");
                        return;
                    }
                }
            }
        );
        dispatcher.set_command(
            journal::db::to_wire(journal::db::Request::add_series),
            [&pipe, &store, &dispatcher] {
                const auto name = ac::pipes::read_string(pipe);
                const auto padding_text = ac::pipes::read_string(pipe);
                if (!name || !padding_text) {
                    fail_pipe(dispatcher, "Failed to read a series name.");
                    return;
                }
                const auto padding = parse_int(*padding_text);
                if (!padding) {
                    if (!send_status(pipe, "error", "Padding must be a whole number.")) {
                        fail_pipe(dispatcher, "Unable to add a journal series.");
                    }
                    return;
                }
                const auto added = store->add_series(*name, *padding);
                if (!added) {
                    if (!send_status(pipe, "error", added.error())) {
                        fail_pipe(dispatcher, "Unable to add a journal series.");
                    }
                    return;
                }
                if (!send_status(pipe, "ok", {})) {
                    fail_pipe(dispatcher, "Unable to add a journal series.");
                }
            }
        );
        dispatcher.set_command(
            journal::db::to_wire(journal::db::Request::set_counter),
            [&pipe, &store, &dispatcher] {
                const auto name = ac::pipes::read_string(pipe);
                const auto counter_text = ac::pipes::read_string(pipe);
                if (!name || !counter_text) {
                    fail_pipe(dispatcher, "Failed to read a series count.");
                    return;
                }
                const auto counter = parse_int(*counter_text);
                if (!counter) {
                    if (!send_status(pipe, "error", "Count must be a whole number.")) {
                        fail_pipe(dispatcher, "Unable to update a journal count.");
                    }
                    return;
                }
                const auto updated = store->set_counter(*name, *counter);
                if (!updated) {
                    if (!send_status(pipe, "error", updated.error())) {
                        fail_pipe(dispatcher, "Unable to update a journal count.");
                    }
                    return;
                }
                if (const auto sent = ac::pipes::send_string(pipe, "ok"); !sent) {
                    fail_pipe(dispatcher, "Unable to update a journal count.");
                    return;
                }
                if (!send_series(pipe, *updated)) {
                    fail_pipe(dispatcher, "Unable to update a journal count.");
                }
            }
        );
        dispatcher.set_command(
            journal::db::to_wire(journal::db::Request::set_padding),
            [&pipe, &store, &dispatcher] {
                const auto name = ac::pipes::read_string(pipe);
                const auto padding_text = ac::pipes::read_string(pipe);
                if (!name || !padding_text) {
                    fail_pipe(dispatcher, "Failed to read series padding.");
                    return;
                }
                const auto padding = parse_int(*padding_text);
                if (!padding) {
                    if (!send_status(pipe, "error", "Padding must be a whole number.")) {
                        fail_pipe(dispatcher, "Unable to update journal padding.");
                    }
                    return;
                }
                const auto updated = store->set_padding(*name, *padding);
                if (!updated) {
                    if (!send_status(pipe, "error", updated.error())) {
                        fail_pipe(dispatcher, "Unable to update journal padding.");
                    }
                    return;
                }
                if (const auto sent = ac::pipes::send_string(pipe, "ok"); !sent) {
                    fail_pipe(dispatcher, "Unable to update journal padding.");
                    return;
                }
                if (!send_series(pipe, *updated)) {
                    fail_pipe(dispatcher, "Unable to update journal padding.");
                }
            }
        );
        dispatcher.set_command(
            journal::db::to_wire(journal::db::Request::find_series),
            [&pipe, &store, &dispatcher] {
                const auto name = ac::pipes::read_string(pipe);
                if (!name) {
                    fail_pipe(dispatcher, "Failed to read a series lookup.");
                    return;
                }
                const auto found = store->find_series(*name);
                if (!found) {
                    if (!send_status(pipe, "error", found.error())) {
                        fail_pipe(dispatcher, "Unable to look up a journal series.");
                    }
                    return;
                }
                if (const auto sent = ac::pipes::send_string(pipe, "ok"); !sent) {
                    fail_pipe(dispatcher, "Unable to look up a journal series.");
                    return;
                }
                if (!send_series(pipe, *found)) {
                    fail_pipe(dispatcher, "Unable to look up a journal series.");
                }
            }
        );
        dispatcher.set_command(
            journal::db::to_wire(journal::db::Request::shutdown),
            [&dispatcher, &shutdown] {
                journal_db.log_main("shutdown signal received");
                shutdown = true;
                dispatcher.request_stop();
            }
        );

        // The next-command read fails with ERROR_BROKEN_PIPE when a short
        // client closes after its handshake or command. That is the end of
        // the session. Any other process failure is logged.
        if (const auto result = dispatcher.process(pipe); !result && !shutdown) {
            if (result.error().system_error != ERROR_BROKEN_PIPE) {
                journal_db.log_print(
                    "Journal database client failed. Error: {}",
                    result.error().system_error
                );
            }
        }
    }
    return 0;
}

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

bool database_present(bool& present) {
    const auto path = journal::sqlite::file_path();
    std::error_code error;
    present = std::filesystem::exists(path, error);
    if (error) {
        journal_db.log_print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return false;
    }
    return true;
}

void remove_quiet(const std::filesystem::path& path) {
    std::error_code error;
    std::filesystem::remove(path, error);
}

int create_series(const std::string_view name, const int padding) {
    const auto path = journal::sqlite::file_path();
    const auto temporary = std::filesystem::path {path.wstring() + L".new"};
    remove_quiet(temporary);
    {
        auto opened = journal::sqlite::open_at(temporary);
        if (!opened) {
            journal_db.log_print("{}", opened.error());
            remove_quiet(temporary);
            return 1;
        }
        std::optional<journal::sqlite::Store> store = std::move(*opened);
        const auto added = store->add_series(name, padding);
        store.reset();
        if (!added) {
            journal_db.log_print("{}", added.error());
            remove_quiet(temporary);
            remove_quiet(std::filesystem::path {temporary.wstring() + L"-journal"});
            remove_quiet(std::filesystem::path {temporary.wstring() + L"-wal"});
            remove_quiet(std::filesystem::path {temporary.wstring() + L"-shm"});
            return 1;
        }
    }
    if (!MoveFileExW(
            temporary.c_str(),
            path.c_str(),
            MOVEFILE_WRITE_THROUGH
        )) {
        journal_db.log_print(
            "Unable to create {}.",
            path.string()
        );
        remove_quiet(temporary);
        return 1;
    }
    journal_db.log_print("Wrote {}", path.string());
    return 0;
}

int run_seed() {
    bool present = false;
    if (!database_present(present)) {
        return 1;
    }
    const auto path = journal::sqlite::file_path().string();
    namespace req = ac::config::components_request;
    if (present) {
        req::log_seed_skipped(journal_db, path);
        return 0;
    }
    req::log_writing_defaults(journal_db);
    if (create_series(seed_series_name, seed_series_padding) != 0) {
        return 1;
    }
    req::log_configuration_initialized(journal_db);
    return 0;
}

std::optional<std::string> prompt_series_name() {
    while (true) {
        std::cout << "Series name [" << seed_series_name << "]: ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            journal_db.log_print("Journal database was not initialized.");
            return std::nullopt;
        }
        const auto value = trim(input);
        if (value == "cancel") {
            journal_db.log_print("Cancelled.");
            return std::nullopt;
        }
        const std::string name = value.empty() ? std::string {seed_series_name} : std::string {value};
        if (name.empty()) {
            std::cout << "Enter a series name.\n";
            continue;
        }
        return name;
    }
}

std::optional<int> prompt_padding() {
    while (true) {
        std::cout << "Padding [" << seed_series_padding << "]: ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            journal_db.log_print("Journal database was not initialized.");
            return std::nullopt;
        }
        const auto value = trim(input);
        if (value == "cancel") {
            journal_db.log_print("Cancelled.");
            return std::nullopt;
        }
        const std::string text = value.empty()
            ? std::to_string(seed_series_padding)
            : std::string {value};
        int padding = 0;
        const auto parsed = std::from_chars(
            text.data(),
            text.data() + text.size(),
            padding
        );
        if (parsed.ec != std::errc {} || parsed.ptr != text.data() + text.size()) {
            std::cout << "Enter a whole number.\n";
            continue;
        }
        if (padding < 0 || padding > 16) {
            std::cout << "Padding must be from 0 through 16.\n";
            continue;
        }
        return padding;
    }
}

int run_init() {
    bool present = false;
    if (!database_present(present)) {
        return 1;
    }
    const auto path = journal::sqlite::file_path().string();
    namespace req = ac::config::components_request;
    if (present) {
        req::log_initialization_skipped(journal_db, path);
        return 0;
    }
    req::log_configuration_missing(journal_db);
    const auto name = prompt_series_name();
    if (!name) {
        return 1;
    }
    const auto padding = prompt_padding();
    if (!padding) {
        return 1;
    }
    return create_series(*name, *padding);
}

} // namespace

int main(int argc, char* argv[]) {
    std::setvbuf(stdin, nullptr, _IONBF, 0);
    ac::shell::set_process_app_user_model_id();
    journal_db.log_main("journal_db.exe started");
    if (argc == 2 && std::string_view {argv[1]} == "--serve") {
        return run_serve();
    }

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }
    namespace req = ac::config::components_request;
    req::log_config_request(journal_db, *launch);
    if (launch->seed) {
        return run_seed();
    }
    if (launch->init) {
        return run_init();
    }
    std::cerr << "Usage: journal_db.exe [--serve | --seed | --init]\n";
    return 1;
}
