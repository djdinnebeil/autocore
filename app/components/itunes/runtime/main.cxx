/**
\file main.cxx
\brief Entry point and process-level support for the iTunes component.
*/
import std;

import auto_core.core.ini;
import auto_core.core.paths;
import auto_core.core.pipes;
import auto_core.core.shell;
import itunes_protocol;
import itunes_component;
import itunes_client;
import itunes_db_client;
import itunes_sqlite;
import itunes_monitor;
import itunes_pipe;
import itunes_removal;
import command_registry;
import itunes_registry;
import component_protocol;

import <Windows.h>;

void update_itunes_component() {
    itunes_component.update_log_file();
}

void log_init() {
    itunes_component.log_info("itunes_ac.exe started");
}

void end_itunes() {
    itunes_component.log_info("shutdown signal received");
}

namespace {

void print_database_error(const std::string& message) {
    itunes_component.log_alert("{}", message);
    std::cout << message << '\n';
}

bool database_exists() {
    std::error_code error;
    return std::filesystem::exists(itunes::sqlite::file_path(), error) && !error;
}

void create_database() {
    auto store = itunes::sqlite::open();
    if (!store) {
        print_database_error(store.error());
        return;
    }
    itunes_component.log_alert("Created a listening database.");
}

void select_all() {
    auto store = itunes::sqlite::open();
    if (!store) {
        print_database_error(store.error());
        return;
    }
    const auto rows = store->select_all();
    if (!rows) {
        print_database_error(rows.error());
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

int run_database_seed() {
    log_init();
    const auto ini_path = ac::paths::component_config_file("itunes");
    if (!ac::ini::read(ini_path)) {
        itunes_component.log_alert(
            "config/components/itunes.ini is missing or unreadable. itunes_ac.exe --config owns it."
        );
        return 1;
    }
    const auto path = itunes::sqlite::file_path();
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error) {
        itunes_component.log_alert(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return 1;
    }
    if (exists) {
        itunes_component.log_debug("Seed skipped; history.db already exists");
        return 0;
    }
    auto store = itunes::sqlite::open();
    if (!store) {
        print_database_error(store.error());
        return 1;
    }
    itunes_component.log_alert("Created a listening database.");
    return 0;
}

int run_database_menu() {
    log_init();
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
        const auto first = choice.find_first_not_of(" \t");
        const auto last = choice.find_last_not_of(" \t");
        if (first != std::string::npos) {
            choice = choice.substr(first, last - first + 1);
        }
        else {
            choice.clear();
        }
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

int export_keymap_commands() {
    struct ClientShutdown {
        ~ClientShutdown() { ac_itunes.release_quietly(); }
    } client_shutdown;

    try {
        const auto registry = create_itunes_command_registry();
        for (const std::string& name : registry.registered_names()) {
            std::cout << name << '\n';
        }
    }
    catch (const std::exception& error) {
        std::cerr << "iTunes keymap command export failed: "
                  << error.what() << '\n';
        return 1;
    }
    catch (...) {
        std::cerr << "iTunes keymap command export failed.\n";
        return 1;
    }
    std::cout.flush();
    return std::cout ? 0 : 1;
}

} // namespace

namespace ac::itunes_app::runtime {

int run(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    if (argc == 2 &&
        std::string_view {argv[1]} == "--export-keymap-commands") {
        return export_keymap_commands();
    }
    if (argc == 2 && std::string_view {argv[1]} == "--seed") {
        return run_database_seed();
    }
    if (argc == 2 && std::string_view {argv[1]} == "--database") {
        return run_database_menu();
    }

    const auto registry = create_itunes_command_registry();
    log_init();
    if (const auto database = itunes::db::start_service(); !database) {
        itunes_component.log_debug("{}", database.error());
    }
    struct DatabaseServiceGuard {
        ~DatabaseServiceGuard() {
            ac_itunes.shutdown();
            itunes::db::shutdown_service();
        }
    } database_service_guard;
    ac_itunes.set_config();
    ac::pipes::Pipe ac_itunes_pipe;
    auto connection = ac::pipes::connect_to_pipe_server(
        ac::protocol::component::pipe_name("itunes")
    );

    if (connection) {
        ac_itunes_pipe = std::move(*connection);
        ac::pipes::CommandDispatcher dispatcher;
        bool protocol_failed = false;
        register_itunes_pipe_commands(
            dispatcher,
            ac_itunes_pipe,
            registry,
            {
                .shutdown = end_itunes,
                .play_pause = itunes_play_pause,
                .next_song = itunes_next_song,
                .print_songs = print_itunes_songs,
                .print_next_up = print_next_up_song_list,
                .update_component = update_itunes_component,
                .previous_song = itunes_prev_song,
                .stop_song = itunes_stop_song,
                .remove_song = remove_itunes_song,
                .unknown_named = [](const std::string_view name) {
                    itunes_component.log_alert(
                        "Unknown iTunes command: {}", name
                    );
                }
            },
            protocol_failed
        );
        if (const auto hello = ac::pipes::send_string(
                ac_itunes_pipe,
                ac::protocol::component::make_hello(
                    registry.autocomplete_values()
                )
            ); !hello) {
            itunes_component.log_alert(
                "Failed to send iTunes hello. Error: {}",
                hello.error().system_error
            );
            ac_itunes.shutdown();
            return 1;
        }
        if (ac_itunes.auto_start && !ac_itunes.initialize_com()) {
            itunes_component.log_alert(
                "Unable to initialize iTunes automation after retrying."
            );
        }
        if (const auto result = dispatcher.process(ac_itunes_pipe);
            !result) {
            itunes_component.log_alert(
                "iTunes pipe failed. Error: {}",
                result.error().system_error
            );
        }
        if (protocol_failed) return 1;
    }
    else {
        itunes_component.log_alert(
            "Failed to connect to iTunes pipe. Error: {}",
            connection.error().system_error
        );
    }

    ac_itunes.shutdown();
    itunes_component.log_info("program terminated");


    return 0;
}

} // namespace ac::itunes_app::runtime
