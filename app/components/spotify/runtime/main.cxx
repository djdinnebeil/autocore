/**
\file main.cxx
\brief Entry point for the Spotify component of Auto Core.

This file initializes the Spotify component, sets up the command map for handling
various commands through named pipes, and manages the main loop for processing
pipe commands. It also handles logging initialization and shutdown.
 */
import std;
import spotify_component;
import spotify_client;
import spotify_monitor;
import spotify_db_client;
import spotify_sqlite;
import auto_core.core.pipes;
import auto_core.core.shell;
import spotify_protocol;
import command_registry;
import spotify_registry;
import spotify_pipe;
import auto_core.taskbar;
import component_protocol;
import auto_core.core.ini;
import auto_core.core.paths;
import <Windows.h>;

/**
 * \brief Ends the Spotify process and performs necessary cleanup.
 */
void end_spotify() {
    spotify_component.log_info("shutdown signal received");
    stop_spotify_monitor();
}

namespace {

void print_database_error(const std::string& message) {
    spotify_component.log_alert("{}", message);
    std::cout << message << '\n';
}

bool database_exists() {
    std::error_code error;
    return std::filesystem::exists(spotify::sqlite::file_path(), error) && !error;
}

void create_database() {
    auto store = spotify::sqlite::open();
    if (!store) {
        print_database_error(store.error());
        return;
    }
    spotify_component.log_alert("Created history.db.");
}

void select_all() {
    auto store = spotify::sqlite::open();
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

int run_database_seed() {
    spotify_component.log_info("spotify_ac.exe started");
    const auto ini_path = ac::paths::component_config_file("spotify");
    if (!ac::ini::read(ini_path)) {
        spotify_component.log_alert(
            "config/components/spotify.ini is missing or unreadable. spotify_ac.exe --config owns it."
        );
        return 1;
    }
    std::error_code error;
    const bool exists = std::filesystem::exists(spotify::sqlite::file_path(), error);
    if (error) {
        spotify_component.log_alert(
            "Failed to inspect {}: {}",
            spotify::sqlite::file_path().string(),
            error.message()
        );
        return 1;
    }
    if (exists) {
        spotify_component.log_debug("Seed skipped; history.db already exists");
        return 0;
    }
    auto store = spotify::sqlite::open();
    if (!store) {
        print_database_error(store.error());
        return 1;
    }
    spotify_component.log_alert("Created history.db.");
    return 0;
}

int run_database_menu() {
    spotify_component.log_info("spotify_ac.exe started");
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
    try {
        const auto registry = create_spotify_command_registry();
        for (const std::string& line : registry.catalog_lines()) {
            std::cout << line << '\n';
        }
    }
    catch (const std::exception& error) {
        std::cerr << "Spotify keymap command export failed: "
                  << error.what() << '\n';
        return 1;
    }
    catch (...) {
        std::cerr << "Spotify keymap command export failed.\n";
        return 1;
    }
    std::cout.flush();
    return std::cout ? 0 : 1;
}

} // namespace

/**
 * \brief Main function for the Spotify process.
 * Initializes logging, sets up the command map, starts the Spotify song thread,
 * and processes pipe commands.
 *
 * \return Exit code of the process.
 */
namespace ac::spotify_app::runtime {

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
    const auto registry = create_spotify_command_registry();
    int exit_code = 0;
    log_init();

    if (const auto database = spotify::db::start_service(); !database) {
        spotify_component.log_debug("{}", database.error());
    }
    struct DatabaseServiceGuard {
        ~DatabaseServiceGuard() { spotify::db::shutdown_service(); }
    } database_service_guard;

    start_spotify_device_requests();
    struct DeviceRequestGuard {
        ~DeviceRequestGuard() { stop_spotify_device_requests(); }
    } device_request_guard;

    {
        const auto ini_path = ac::paths::component_config_file("spotify");
        if (!ac::ini::read(ini_path)) {
            std::error_code exists_error;
            const bool present =
                std::filesystem::exists(ini_path, exists_error);
            spotify_component.report_ini_unavailable(present && !exists_error);
        }
    }

    if (!ac_spotify.song_format_error.empty()) {
        spotify_component.log_alert("{}", ac_spotify.song_format_error);
        spotify_component.log_alert("Using the compiled Spotify song format.");
    }

    if (!ac::taskbar::connect()) {
        spotify_component.log_alert(
            "Unable to receive the native taskbar snapshot from Auto Core."
        );
    }
    struct TaskbarConnectionGuard {
        ~TaskbarConnectionGuard() { ac::taskbar::disconnect(); }
    } taskbar_connection_guard;

    start_spotify_monitor();

    ac::pipes::Pipe ac_spotify_pipe;
    auto connection = ac::pipes::connect_to_pipe_server(
        ac::protocol::component::pipe_name("spotify")
    );

    if (connection) {
        ac_spotify_pipe = std::move(*connection);
        ac::pipes::CommandDispatcher dispatcher;
        bool protocol_failed = false;
        register_spotify_pipe_commands(
            dispatcher,
            ac_spotify_pipe,
            registry,
            {
                .shutdown = &end_spotify,
                .play_pause = &spotify_play_pause,
                .next_song = &spotify_next_song,
                .print_songs = &spotify_print_songs,
                .get_queue = &spotify_get_queue,
                .update_component = &update_spotify_component,
                .switch_player = &spotify_switch_player,
                .download_album_cover = &spotify_download_album_cover,
                .unknown_named = [](const std::string_view name) {
                    spotify_component.log_alert("Unknown Spotify command: {}", name);
                }
            },
            protocol_failed
        );
        if (const auto hello = ac::pipes::send_string(
                ac_spotify_pipe,
                ac::protocol::component::make_hello(
                    registry.hello_lines()
                )
            ); !hello) {
            spotify_component.log_alert(
                "Failed to send Spotify hello. Error: {}",
                hello.error().system_error
            );
            exit_code = 1;
        }
        else if (const auto result = dispatcher.process(ac_spotify_pipe);
            !result) {
            spotify_component.log_alert(
                "Spotify pipe failed. Error: {}",
                result.error().system_error
            );
        }
        if (protocol_failed) {
            exit_code = 1;
        }
    }
    else {
        spotify_component.log_alert(
            "Failed to connect to Spotify pipe. Error: {}",
            connection.error().system_error
        );
    }

    stop_spotify_monitor();

    spotify_component.log_info("program terminated");

    return exit_code;
}

} // namespace ac::spotify_app::runtime
