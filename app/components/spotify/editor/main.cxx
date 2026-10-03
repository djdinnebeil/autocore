import std;
import spotify_application_data;
import spotify_devices_protocol;
import auto_core.core.component;
import auto_core.core.ini;
import auto_core.core.logging.config;
import auto_core.core.paths;
import auto_core.core.pipes;

import <Windows.h>;
import auto_core.core.shell;

namespace {

ac::Component spotify_editor {
    "spotify_editor",
    ac::logging::config::LoggingScope {"spotify"}
};

std::string_view trim(std::string_view value) {
    return spotify::data::trim_ws(value);
}

bool replace_file(const std::filesystem::path& destination, std::string_view contents) {
    std::error_code error;
    std::filesystem::create_directories(destination.parent_path(), error);
    if (error) {
        spotify_editor.log_print(
            "Failed to create {}: {}",
            destination.parent_path().string(),
            error.message()
        );
        return false;
    }
    const auto temporary = destination.native() + L".new";
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) {
            spotify_editor.log_print("Failed to create {}.", destination.string());
            return false;
        }
        output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        output.flush();
        output.close();
        if (!output) {
            std::filesystem::remove(temporary, error);
            spotify_editor.log_print("Failed to write {}.", destination.string());
            return false;
        }
    }
    if (!MoveFileExW(
            temporary.c_str(),
            destination.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
        )) {
        std::filesystem::remove(temporary, error);
        spotify_editor.log_print("Failed to replace {}.", destination.string());
        return false;
    }
    return true;
}

int require_readable_ini() {
    const auto path = ac::paths::config_directory() / "spotify.ini";
    if (!ac::ini::read(path)) {
        spotify_editor.log_print(
            "config/spotify.ini is missing or unreadable. spotify_config.exe owns it."
        );
        return 1;
    }
    return 0;
}

int set_client_id(const bool allow_blank) {
    std::cout << "Enter Spotify client ID: ";
    std::string input;
    if (!std::getline(std::cin, input)) {
        spotify_editor.log_print("Failed to read the Spotify client ID.");
        return 1;
    }
    const auto chosen = trim(input);
    if (chosen.empty()) {
        if (!allow_blank) {
            spotify_editor.log_print("The Spotify client ID cannot be empty.");
            return 1;
        }
        if (spotify::data::load_client_id()) {
            spotify_editor.log_print(
                "Left existing {} unchanged.",
                spotify::data::client_id_path().string()
            );
            return 0;
        }
        if (!replace_file(spotify::data::client_id_path(), {})) {
            return 1;
        }
        spotify_editor.log_print("Spotify client ID left blank.");
        return 0;
    }
    if (!replace_file(spotify::data::client_id_path(), std::string {chosen})) {
        return 1;
    }
    spotify_editor.log_print("Wrote {}.", spotify::data::client_id_path().string());
    return 0;
}

int seed_client_id() {
    if (require_readable_ini() != 0) {
        return 1;
    }
    const auto path = spotify::data::client_id_path();
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error) {
        spotify_editor.log_print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return 1;
    }
    if (exists) {
        spotify_editor.log_print("Seed skipped; client.id already exists");
        return 0;
    }
    if (!replace_file(path, {})) {
        return 1;
    }
    spotify_editor.log_print("Created a blank client.id.");
    return 0;
}

int retrieve_devices() {
    auto pipe = ac::pipes::connect_to_pipe_server(
        std::wstring {spotify::devices::pipe_name}
    );
    if (!pipe) {
        spotify_editor.log_print("The Spotify component is not running.");
        return 1;
    }
    if (const auto sent = ac::pipes::send_string(*pipe, spotify::devices::protocol_id); !sent) {
        spotify_editor.log_print("The Spotify component is not running.");
        return 1;
    }
    auto hello = ac::pipes::read_string(*pipe);
    if (!hello || *hello != "ok") {
        spotify_editor.log_print("Spotify device discovery is unavailable.");
        return 1;
    }
    if (const auto sent = ac::pipes::send_pipe_command(
            *pipe,
            spotify::devices::to_wire(spotify::devices::Request::list_devices)
        );
        !sent) {
        spotify_editor.log_print("Spotify device discovery is unavailable.");
        return 1;
    }
    auto status = ac::pipes::read_string(*pipe);
    if (!status) {
        spotify_editor.log_print("Spotify device discovery is unavailable.");
        return 1;
    }
    auto body = ac::pipes::read_string(*pipe);
    if (!body) {
        spotify_editor.log_print("Spotify device discovery is unavailable.");
        return 1;
    }
    if (*status != "ok") {
        spotify_editor.log_print("{}", *body);
        return 1;
    }
    const auto devices = spotify::data::parse_devices(*body);
    if (!replace_file(
            spotify::data::devices_list_path(),
            spotify::data::serialize_devices(devices)
        )) {
        return 1;
    }
    spotify_editor.log_print("Wrote {}.", spotify::data::devices_list_path().string());
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    spotify_editor.log_main("spotify_editor.exe started");
    if (argc == 2 && std::string_view {argv[1]} == "--client-id") {
        return set_client_id(true);
    }
    if (argc == 2 && std::string_view {argv[1]} == "--seed") {
        return seed_client_id();
    }
    if (argc != 1) {
        std::cerr << "Usage: spotify_editor.exe [--client-id | --seed]\n";
        return 1;
    }

    while (true) {
        std::cout
            << "spotify_editor\n"
            << "\n"
            << "1. Set client ID\n"
            << "2. Retrieve current devices\n"
            << "3. Exit\n"
            << "Select option: ";
        std::string option;
        if (!std::getline(std::cin, option)) {
            return 0;
        }
        if (option == "1") {
            (void)set_client_id(false);
        }
        else if (option == "2") {
            (void)retrieve_devices();
        }
        else if (option == "3") {
            return 0;
        }
        else {
            spotify_editor.log_print("Unknown option.");
        }
    }
}
