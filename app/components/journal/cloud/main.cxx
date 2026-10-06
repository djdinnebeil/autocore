import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.pipes;
import auto_core.core.process;
import journal_cloud_http;
import journal_cloud_protocol;
import journal_firebase;
import components_editor_request;

import <iostream>;
import <Windows.h>;
import auto_core.core.shell;

namespace {

ac::Component journal_cloud {
    "journal_cloud",
    ac::logging::config::LoggingScope {"journal"}
};

constexpr std::wstring_view serve_mutex_name = L"Local\\AutoCoreJournalCloudServe";

bool send_status(
    ac::pipes::Pipe& pipe,
    const std::string_view status,
    const std::string_view detail
) {
    if (const auto sent = ac::pipes::send_string(pipe, status); !sent) {
        return false;
    }
    if (detail.empty() && status == "ok") {
        return true;
    }
    return static_cast<bool>(ac::pipes::send_string(pipe, detail));
}

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

bool file_present(bool& present) {
    const auto path = journal::firebase::file_path();
    std::error_code error;
    present = std::filesystem::exists(path, error);
    if (error) {
        journal_cloud.log_print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return false;
    }
    return true;
}

int run_seed() {
    bool present = false;
    if (!file_present(present)) {
        return 1;
    }
    const auto path = journal::firebase::file_path();
    namespace req = ac::config::components_request;
    if (present) {
        req::log_seed_skipped(journal_cloud, path.string());
        return 0;
    }
    req::log_writing_defaults(journal_cloud);
    const auto written = journal::firebase::write_blank(path);
    if (!written) {
        journal_cloud.log_print("{}", written.error());
        return 1;
    }
    journal_cloud.log_print("Wrote {}", path.string());
    req::log_configuration_initialized(journal_cloud);
    return 0;
}

int run_init() {
    bool present = false;
    if (!file_present(present)) {
        return 1;
    }
    const auto path = journal::firebase::file_path();
    namespace req = ac::config::components_request;
    if (present) {
        req::log_initialization_skipped(journal_cloud, path.string());
        return 0;
    }
    req::log_configuration_missing(journal_cloud);
    while (true) {
        std::cout << "Firebase ID []: ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            journal_cloud.log_print("Journal Firebase was not initialized.");
            return 1;
        }
        const auto value = trim(input);
        if (value == "cancel") {
            journal_cloud.log_print("Cancelled.");
            return 1;
        }
        if (value.empty()) {
            const auto written = journal::firebase::write_blank(path);
            if (!written) {
                journal_cloud.log_print("{}", written.error());
                return 1;
            }
            journal_cloud.log_print("Wrote {}", path.string());
            return 0;
        }
        const auto written = journal::firebase::write_url(path, value);
        if (!written) {
            journal_cloud.log_print("{}", written.error());
            std::cout << written.error() << '\n';
            continue;
        }
        journal_cloud.log_print("Wrote {}", path.string());
        return 0;
    }
}

int run_menu() {
    while (true) {
        const auto url = journal::firebase::read_url(journal::firebase::file_path());
        std::cout << '\n' << journal::firebase::menu_text(url) << "> ";
        std::string choice;
        if (!std::getline(std::cin, choice)) {
            return 0;
        }
        const auto first = choice.find_first_not_of(" \t");
        const auto last = choice.find_last_not_of(" \t");
        if (first != std::string::npos) {
            choice = choice.substr(first, last - first + 1);
        }
        else {
            choice.clear();
        }
        if (choice == "1") {
            std::cout << "Firebase URL: ";
            std::string input;
            if (!std::getline(std::cin, input)) {
                return 0;
            }
            const auto written = journal::firebase::write_url(
                journal::firebase::file_path(),
                input
            );
            if (!written) {
                journal_cloud.log_print("{}", written.error());
                std::cout << written.error() << '\n';
                continue;
            }
            journal_cloud.log_print("Wrote {}", journal::firebase::file_path().string());
        }
        else if (choice == "2" || choice == "q" || choice == "Q") {
            return 0;
        }
        else {
            std::cout << "Unknown option.\n";
        }
    }
}

int run_serve(void* const owner) {
    HANDLE mutex = CreateMutexW(nullptr, TRUE, serve_mutex_name.data());
    if (mutex == nullptr) {
        journal_cloud.log_print("Unable to create the journal cloud service mutex.");
        return 1;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(mutex);
        journal_cloud.log_print("Journal cloud service is already running.");
        return 1;
    }
    struct MutexGuard {
        HANDLE handle;
        ~MutexGuard() {
            ReleaseMutex(handle);
            CloseHandle(handle);
        }
    } mutex_guard {mutex};

    auto server = ac::pipes::create_pipe_server(
        std::wstring {journal::cloud::pipe_name}
    );
    if (!server) {
        journal_cloud.log_print(
            "Unable to open the journal cloud pipe. Error: {}",
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
            journal_cloud.log_print("Unable to watch the journal cloud owner process.");
            return 1;
        }
    }
    if (ConnectNamedPipe(pipe.native_handle(), nullptr) == FALSE &&
        GetLastError() != ERROR_PIPE_CONNECTED) {
        if (dispatcher.stop_requested()) {
            return 0;
        }
        journal_cloud.log_print(
            "Unable to accept a journal cloud client. Error: {}",
            GetLastError()
        );
        return 1;
    }
    const auto hello = ac::pipes::read_string(pipe);
    if (!hello || *hello != journal::cloud::protocol_id) {
        (void)send_status(pipe, "protocol", "Unsupported journal cloud protocol.");
        journal_cloud.log_print("Rejected a journal cloud client using an unexpected protocol.");
        return 1;
    }
    if (!send_status(pipe, "ok", {})) {
        journal_cloud.log_print("Unable to accept the journal cloud client.");
        return 1;
    }

    dispatcher.set_command(
        journal::cloud::to_wire(journal::cloud::Request::push),
        [&pipe, &dispatcher] {
            const auto name = ac::pipes::read_string(pipe);
            const auto count_text = ac::pipes::read_string(pipe);
            if (!name || !count_text) {
                journal_cloud.log_print("Failed to read a cloud push.");
                dispatcher.request_stop();
                return;
            }
            int count = 0;
            const auto parsed = std::from_chars(
                count_text->data(),
                count_text->data() + count_text->size(),
                count
            );
            if (parsed.ec != std::errc {} ||
                parsed.ptr != count_text->data() + count_text->size()) {
                if (!send_status(pipe, "error", "Cloud count must be a whole number.")) {
                    dispatcher.request_stop();
                }
                return;
            }
            const auto url = journal::firebase::read_url(journal::firebase::file_path());
            if (!url) {
                journal_cloud.log_print("No Firebase URL.");
                if (!send_status(pipe, "error", "No Firebase URL.")) {
                    dispatcher.request_stop();
                }
                return;
            }
            const auto pushed = journal::cloud::http::push(*url, *name, count);
            if (!pushed) {
                journal_cloud.log_print("{}", pushed.error());
                if (!send_status(pipe, "error", pushed.error())) {
                    dispatcher.request_stop();
                }
                return;
            }
            journal_cloud.log_main("Updated data: {} {}", *name, count);
            if (!send_status(pipe, "ok", {})) {
                dispatcher.request_stop();
            }
        }
    );
    dispatcher.set_command(
        journal::cloud::to_wire(journal::cloud::Request::shutdown),
        [&dispatcher] {
            journal_cloud.log_main("shutdown signal received");
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
        journal_cloud.log_print(
            "Journal cloud pipe failed. Error: {}",
            result.error().system_error
        );
        return 1;
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    std::setvbuf(stdin, nullptr, _IONBF, 0);
    ac::shell::set_process_app_user_model_id();
    journal_cloud.log_main("journal_cloud.exe started");
    if (argc >= 2 && std::string_view {argv[1]} == "--serve") {
        void* owner = nullptr;
        if (argc == 4 && std::string_view {argv[2]} == "--owner-handle") {
            const auto handle = ac::process::parse_owner_handle(argv[3]);
            if (!handle) {
                std::cerr << "Invalid --owner-handle.\n";
                return 1;
            }
            owner = *handle;
        }
        else if (argc != 2) {
            std::cerr << "Usage: journal_cloud.exe [--serve | --seed | --init]\n";
            return 1;
        }
        return run_serve(owner);
    }

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }
    namespace req = ac::config::components_request;
    req::log_config_request(journal_cloud, *launch);
    if (launch->seed) {
        return run_seed();
    }
    if (launch->init) {
        return run_init();
    }
    if (argc > 1) {
        std::cerr << "Usage: journal_cloud.exe [--serve | --seed | --init]\n";
        return 1;
    }
    return run_menu();
}
