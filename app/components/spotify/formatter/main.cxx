/**
 * \file main.cxx
 * \brief Edits `<spotify directory>/song.format` for `spotify_formatter.exe`.
 *
 * This program is the sole writer of `song.format`. It reads `spotify.ini`
 * and does not write it. `--seed` and `--init` require a readable INI.
 * The no-argument menu still launches `spotify_config.exe --init` when it is missing.
 */
import std;

import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.ini;
import auto_core.core.paths;
import spotify_data_directory;
import spotify_song_catalog;
import spotify_song_template;

import <Windows.h>;
import <iostream>;
import auto_core.core.shell;

namespace {

ac::Component spotify_formatter {
    "spotify_formatter",
    ac::logging::config::LoggingScope {"spotify"}
};

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

std::wstring quote_argument(std::wstring_view value) {
    std::wstring quoted;
    quoted.reserve(value.size() + 2);
    quoted.push_back(L'"');
    quoted.append(value);
    quoted.push_back(L'"');
    return quoted;
}

int launch_config_init() {
    const auto executable_path =
        ac::paths::bin_directory() / "spotify_config.exe";
    std::error_code error;
    const bool present = std::filesystem::exists(executable_path, error);
    if (error || !present) {
        spotify_formatter.log_print("Missing {}.", executable_path.string());
        return 1;
    }

    std::wstring command_line =
        quote_argument(executable_path.wstring()) + L" \"--init\"";
    STARTUPINFOW startup_info {};
    startup_info.cb = sizeof(startup_info);
    PROCESS_INFORMATION process_info {};
    if (!::CreateProcessW(
            executable_path.c_str(),
            command_line.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            executable_path.parent_path().c_str(),
            &startup_info,
            &process_info
        )) {
        spotify_formatter.log_print(
            "Unable to start {}.",
            executable_path.string()
        );
        return 1;
    }

    ::CloseHandle(process_info.hThread);
    ::WaitForSingleObject(process_info.hProcess, INFINITE);
    DWORD exit_code = 1;
    if (!::GetExitCodeProcess(process_info.hProcess, &exit_code)) {
        ::CloseHandle(process_info.hProcess);
        spotify_formatter.log_print(
            "Unable to read the exit code from spotify_config.exe."
        );
        return 1;
    }
    ::CloseHandle(process_info.hProcess);
    const int code = static_cast<int>(exit_code);
    if (code != 0) {
        spotify_formatter.log_print("spotify_config.exe exited {}.", code);
    }
    return code;
}

bool ini_readable(const std::filesystem::path& path) {
    return ac::ini::read(path).has_value();
}

int ensure_spotify_ini(const std::filesystem::path& path) {
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error) {
        spotify_formatter.log_print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return 1;
    }
    if (present) {
        if (!ini_readable(path)) {
            spotify_formatter.log_print(
                "Failed to read {}. Song format was not edited.",
                path.string()
            );
            return 1;
        }
        return 0;
    }

    spotify_formatter.log_print(
        "config/spotify.ini is missing. Running spotify_config.exe --init."
    );
    if (launch_config_init() != 0) {
        spotify_formatter.log_print(
            "spotify.ini was not initialized. Song format was not edited."
        );
        return 1;
    }

    error.clear();
    const bool created = std::filesystem::exists(path, error);
    if (error || !created || !ini_readable(path)) {
        spotify_formatter.log_print(
            "config/spotify.ini is still unavailable. Song format was not edited."
        );
        return 1;
    }
    return 0;
}

int require_readable_ini() {
    const auto path = ac::paths::config_directory() / "spotify.ini";
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error) {
        spotify_formatter.log_print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return 1;
    }
    if (!present || !ini_readable(path)) {
        spotify_formatter.log_print(
            "config/spotify.ini is missing or unreadable. spotify_config.exe owns it."
        );
        return 1;
    }
    return 0;
}

std::optional<std::string> read_bytes(
    const std::filesystem::path& path,
    std::string& error
) {
    std::error_code code;
    const auto size = std::filesystem::file_size(path, code);
    if (code) {
        error = "Failed to read " + path.string() + ".";
        return std::nullopt;
    }
    if (size > spotify::song::max_template_bytes) {
        error = "Song format is larger than 4096 bytes.";
        return std::nullopt;
    }
    std::ifstream input(path, std::ios::binary);
    std::string bytes(static_cast<std::size_t>(size), '\0');
    if (size > 0) {
        input.read(bytes.data(), static_cast<std::streamsize>(size));
    }
    if (!input || input.gcount() != static_cast<std::streamsize>(size)) {
        error = "Failed to read " + path.string() + ".";
        return std::nullopt;
    }
    return bytes;
}

std::string preview_line(const spotify::song::Compiled& compiled) {
    return spotify::song::apply(compiled, spotify::catalog::preview_value);
}

void print_tokens() {
    std::cout << "Tokens:";
    for (const std::string_view token : spotify::catalog::tokens) {
        std::cout << " {" << token << '}';
    }
    std::cout << '\n';
}

bool replace_song_format(
    const std::filesystem::path& path,
    std::string_view contents
) {
    if (!spotify::song::replace_song_format(path, contents)) {
        spotify_formatter.log_print("Failed to replace {}", path.string());
        return false;
    }
    spotify_formatter.log_print("Wrote {}", path.string());
    return true;
}

} // namespace

int edit_song_format() {
    const auto format_path = spotify::data_directory() / "song.format";
    std::error_code error;
    const bool existed = std::filesystem::exists(format_path, error);
    if (error) {
        spotify_formatter.log_print(
            "Failed to inspect {}: {}",
            format_path.string(),
            error.message()
        );
        return 1;
    }

    std::string current {spotify::song::default_template};
    bool current_matches_file = false;
    if (existed) {
        std::string read_error;
        const auto bytes = read_bytes(format_path, read_error);
        if (!bytes) {
            spotify_formatter.log_print("{}", read_error);
        }
        else if (const auto accepted = spotify::catalog::accepted_song_format(*bytes)) {
            current = *accepted;
            current_matches_file = true;
        }
        else {
            const auto compiled = spotify::catalog::compile_utf8(*bytes);
            spotify_formatter.log_print(
                "{}",
                compiled.error.empty()
                    ? "song.format is invalid."
                    : compiled.error
            );
            if (compiled.error != "Song format is not valid UTF-8." &&
                compiled.error != "Song format is larger than 4096 bytes.") {
                std::string_view text {*bytes};
                if (text.ends_with("\r\n")) {
                    text.remove_suffix(2);
                }
                else if (text.ends_with('\n') || text.ends_with('\r')) {
                    text.remove_suffix(1);
                }
                current = std::string {text};
                current_matches_file = true;
            }
        }
    }

    print_tokens();
    const auto current_compiled = spotify::catalog::compile_utf8(current);
    if (current_compiled.ok) {
        std::cout << "Sample: " << preview_line(current_compiled.compiled) << '\n';
    }
    else {
        std::cout << "Current format is invalid.\n";
        std::cout
            << "Sample: "
            << preview_line(spotify::catalog::default_compiled())
            << '\n';
    }

    while (true) {
        std::cout << "Format [" << current << "]: ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            spotify_formatter.log("Input ended.");
            return 0;
        }
        const auto value = trim(line);
        if (value == "cancel") {
            spotify_formatter.log("Song format edit cancelled.");
            return 0;
        }

        const std::string candidate = value.empty()
            ? current
            : std::string {value};
        const auto compiled = spotify::catalog::compile_utf8(candidate);
        if (!compiled.ok) {
            std::cout << compiled.error << '\n';
            continue;
        }
        const auto accepted = spotify::catalog::accepted_song_format(candidate);
        if (!accepted) {
            std::cout << "Song format is invalid.\n";
            continue;
        }
        if (current_matches_file && *accepted == current) {
            spotify_formatter.log_print(
                "Left existing {} unchanged.",
                format_path.string()
            );
            return 0;
        }
        if (!replace_song_format(format_path, *accepted)) {
            return 1;
        }
        return 0;
    }
}

int run_seed() {
    if (require_readable_ini() != 0) {
        return 1;
    }
    const auto format_path = spotify::data_directory() / "song.format";
    std::error_code error;
    const bool exists = std::filesystem::exists(format_path, error);
    if (error) {
        spotify_formatter.log_print(
            "Failed to inspect {}: {}",
            format_path.string(),
            error.message()
        );
        return 1;
    }
    if (exists) {
        spotify_formatter.log_print("Seed skipped; song.format already exists");
        return 0;
    }
    if (!replace_song_format(format_path, spotify::song::default_template)) {
        return 1;
    }
    return 0;
}

int run_init() {
    if (require_readable_ini() != 0) {
        return 1;
    }
    const auto format_path = spotify::data_directory() / "song.format";
    std::error_code error;
    const bool exists = std::filesystem::exists(format_path, error);
    if (error) {
        spotify_formatter.log_print(
            "Failed to inspect {}: {}",
            format_path.string(),
            error.message()
        );
        return 1;
    }
    if (exists) {
        spotify_formatter.log_print(
            "Initialization skipped; song.format already exists"
        );
        return 0;
    }
    return edit_song_format();
}

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    spotify_formatter.log_main("spotify_formatter.exe started");

    const bool seed = argc == 2 && std::string_view {argv[1]} == "--seed";
    const bool init = argc == 2 && std::string_view {argv[1]} == "--init";
    if (argc > 2 || (argc == 2 && !seed && !init)) {
        std::cerr << "Usage: spotify_formatter.exe [--init | --seed]\n";
        return 1;
    }
    if (seed) {
        return run_seed();
    }
    if (init) {
        return run_init();
    }

    const auto ini_path = ac::paths::config_directory() / "spotify.ini";
    if (ensure_spotify_ini(ini_path) != 0) {
        return 1;
    }
    return edit_song_format();
}
