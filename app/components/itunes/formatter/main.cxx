/**
 * \file main.cxx
 * \brief Edits `<itunes directory>/song.format` and `library.format`.
 *
 * This program is the sole writer of both files. It reads `itunes.ini`
 * and does not write it. `--seed` and `--init` require a readable INI.
 * The no-argument menu still launches `itunes_config.exe --init` when it is missing.
 */
module;

#include "../main/itunes_config_detail.hpp"
#include "../shared/itunes_metadata_detail.hpp"
#include "../shared/library_format_detail.hpp"
#include "../shared/song_template_detail.hpp"

export module itunes_formatter_main;

import std;

import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.encoding;
import auto_core.core.ini;
import auto_core.core.paths;

import <Windows.h>;
import <iostream>;

namespace {

ac::Component itunes_formatter {
    "itunes_formatter",
    ac::logging::config::LoggingScope {"itunes"}
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
        ac::paths::bin_directory() / "itunes_config.exe";
    std::error_code error;
    const bool present = std::filesystem::exists(executable_path, error);
    if (error || !present) {
        itunes_formatter.log_print("Missing {}.", executable_path.string());
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
        itunes_formatter.log_print(
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
        itunes_formatter.log_print(
            "Unable to read the exit code from itunes_config.exe."
        );
        return 1;
    }
    ::CloseHandle(process_info.hProcess);
    const int code = static_cast<int>(exit_code);
    if (code != 0) {
        itunes_formatter.log_print("itunes_config.exe exited {}.", code);
    }
    return code;
}

bool ini_readable(const std::filesystem::path& path) {
    return ac::ini::read(path).has_value();
}

int ensure_itunes_ini(const std::filesystem::path& path) {
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error) {
        itunes_formatter.log_print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return 1;
    }
    if (present) {
        if (!ini_readable(path)) {
            itunes_formatter.log_print(
                "Failed to read {}. Song format was not edited.",
                path.string()
            );
            return 1;
        }
        return 0;
    }

    itunes_formatter.log_print(
        "config/itunes.ini is missing. Running itunes_config.exe --init."
    );
    if (launch_config_init() != 0) {
        itunes_formatter.log_print(
            "itunes.ini was not initialized. Song format was not edited."
        );
        return 1;
    }

    error.clear();
    const bool created = std::filesystem::exists(path, error);
    if (error || !created || !ini_readable(path)) {
        itunes_formatter.log_print(
            "config/itunes.ini is still unavailable. Song format was not edited."
        );
        return 1;
    }
    return 0;
}

std::filesystem::path itunes_data_directory() {
    const auto path = ac::paths::config_directory() / "itunes.ini";
    const auto document = ac::ini::read(path);
    std::optional<std::string_view> stored;
    if (document) {
        stored = document->find("itunes", "directory");
    }
    return itunes::config::detail::resolve_directory(
        stored,
        ac::paths::installation_root()
    );
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
    if (size > itunes::song::detail::max_template_bytes) {
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

std::string preview_line(const itunes::song::detail::Compiled& compiled) {
    const std::wstring preview = itunes::song::detail::apply(
        compiled,
        [](const std::string_view token) -> std::wstring {
            if (token == "name") {
                return L"Song";
            }
            if (token == "artist") {
                return L"Artist";
            }
            if (token == "album") {
                return L"Album";
            }
            if (token == "duration") {
                return L"3:42";
            }
            return {};
        }
    );
    return ac::encoding::to_utf8(preview);
}

void print_tokens() {
    std::cout << "Tokens:";
    for (const auto& spec : itunes::metadata::detail::catalog) {
        std::cout << " {" << spec.token << '}';
    }
    std::cout << '\n';
}

bool replace_format_file(
    const std::filesystem::path& path,
    std::string_view contents
) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        itunes_formatter.log_print(
            "Failed to create {}: {}",
            path.parent_path().string(),
            error.message()
        );
        return false;
    }

    const auto temporary_path = std::filesystem::path {path.wstring() + L".new"};
    {
        std::ofstream output(temporary_path, std::ios::binary | std::ios::trunc);
        if (!output) {
            itunes_formatter.log_print("Failed to create {}", path.string());
            return false;
        }
        output.write(
            contents.data(),
            static_cast<std::streamsize>(contents.size())
        );
        output.flush();
        if (!output) {
            output.close();
            ::DeleteFileW(temporary_path.c_str());
            itunes_formatter.log_print("Failed to write {}", path.string());
            return false;
        }
    }

    if (!::MoveFileExW(
            temporary_path.c_str(),
            path.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
        )) {
        ::DeleteFileW(temporary_path.c_str());
        itunes_formatter.log_print("Failed to replace {}", path.string());
        return false;
    }
    itunes_formatter.log_print("Wrote {}", path.string());
    return true;
}

int format_song(const std::filesystem::path& directory) {
    const auto format_path = directory / "song.format";
    std::error_code error;
    const bool existed = std::filesystem::exists(format_path, error);
    if (error) {
        itunes_formatter.log_print(
            "Failed to inspect {}: {}",
            format_path.string(),
            error.message()
        );
        return 1;
    }

    std::string current {itunes::song::detail::default_template};
    bool current_matches_file = false;
    if (existed) {
        std::string read_error;
        const auto bytes = read_bytes(format_path, read_error);
        if (!bytes) {
            itunes_formatter.log_print("{}", read_error);
        }
        else if (const auto accepted = itunes::song::detail::accepted_song_format(*bytes)) {
            current = *accepted;
            current_matches_file = true;
        }
        else {
            const auto compiled = itunes::song::detail::compile_utf8(*bytes);
            itunes_formatter.log_print(
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
    const auto current_compiled = itunes::song::detail::compile_utf8(current);
    if (current_compiled.ok) {
        std::cout << "Sample: " << preview_line(current_compiled.compiled) << '\n';
    }
    else {
        std::cout << "Current format is invalid.\n";
        std::cout
            << "Sample: "
            << preview_line(itunes::song::detail::default_compiled())
            << '\n';
    }

    while (true) {
        std::cout << "Format [" << current << "]: ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            itunes_formatter.log("Input ended.");
            return 0;
        }
        const auto value = trim(line);
        if (value == "cancel") {
            itunes_formatter.log("Song format edit cancelled.");
            return 0;
        }

        const std::string candidate = value.empty()
            ? current
            : std::string {value};
        const auto compiled = itunes::song::detail::compile_utf8(candidate);
        if (!compiled.ok) {
            std::cout << compiled.error << '\n';
            continue;
        }
        const auto accepted = itunes::song::detail::accepted_song_format(candidate);
        if (!accepted) {
            std::cout << "Song format is invalid.\n";
            continue;
        }
        if (current_matches_file && *accepted == current) {
            itunes_formatter.log_print(
                "Left existing {} unchanged.",
                format_path.string()
            );
            return 0;
        }
        if (!replace_format_file(format_path, *accepted)) {
            return 1;
        }
        return 0;
    }
}

int configure_library_columns(const std::filesystem::path& directory) {
    const auto format_path = directory / "library.format";
    const auto loaded =
        itunes::library_format::detail::load_library_format(format_path);
    if (loaded.error) {
        itunes_formatter.log_print("{}", *loaded.error);
    }

    std::error_code error;
    const bool present = std::filesystem::exists(format_path, error);
    if (error) {
        itunes_formatter.log_print(
            "Failed to inspect {}: {}",
            format_path.string(),
            error.message()
        );
        return 1;
    }

    const auto& shown = loaded.format;
    std::string chosen_text;
    itunes::library_format::detail::CompiledLibraryFormat chosen_compiled;
    while (true) {
        std::cout << "Format [" << shown.format_text << "]: ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            itunes_formatter.log("Input ended.");
            return 0;
        }
        const auto value = trim(line);
        if (value == "cancel") {
            itunes_formatter.log("Library column edit cancelled.");
            return 0;
        }

        const std::string candidate = value.empty()
            ? shown.format_text
            : std::string {value};
        const auto compiled =
            itunes::library_format::detail::compile_library_format(candidate);
        if (!compiled) {
            std::cout
                << "Enter a format such as [column] or column - column.\n";
            continue;
        }
        chosen_text = candidate;
        chosen_compiled = *compiled;
        break;
    }

    int chosen_count = shown.column_count;
    while (true) {
        std::cout << "Column count [" << shown.column_count << "]: ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            itunes_formatter.log("Input ended.");
            return 0;
        }
        const auto value = trim(line);
        if (value == "cancel") {
            itunes_formatter.log("Library column edit cancelled.");
            return 0;
        }
        if (value.empty()) {
            break;
        }
        const auto parsed =
            itunes::library_format::detail::parse_column_count(value);
        if (!parsed) {
            std::cout << "Enter a positive whole number.\n";
            continue;
        }
        chosen_count = *parsed;
        break;
    }

    if (present &&
        !loaded.error &&
        chosen_text == shown.format_text &&
        chosen_count == shown.column_count) {
        itunes_formatter.log_print(
            "Left existing {} unchanged.",
            format_path.string()
        );
        return 0;
    }

    itunes::library_format::detail::LibraryFormat accepted;
    accepted.format_text = std::move(chosen_text);
    accepted.compiled = std::move(chosen_compiled);
    accepted.column_count = chosen_count;
    if (!replace_format_file(
            format_path,
            itunes::library_format::detail::library_format_text(accepted)
        )) {
        return 1;
    }
    return 0;
}

std::optional<bool> file_present(const std::filesystem::path& path) {
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error) {
        itunes_formatter.log_print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return std::nullopt;
    }
    return present;
}

bool itunes_ini_readable() {
    const auto path = ac::paths::config_directory() / "itunes.ini";
    if (ac::ini::read(path)) {
        return true;
    }
    itunes_formatter.log_print(
        "config/itunes.ini is missing or unreadable. itunes_config.exe owns it."
    );
    return false;
}

std::optional<std::string> read_init_line(
    std::string_view label,
    std::string_view shown
) {
    std::cout << label << " [" << shown << "]: ";
    std::string line;
    if (!std::getline(std::cin, line)) {
        itunes_formatter.log("Input ended.");
        return std::nullopt;
    }
    const auto value = trim(line);
    if (value == "cancel") {
        itunes_formatter.log("Initialization cancelled.");
        return std::nullopt;
    }
    if (value.empty()) {
        return std::string {shown};
    }
    return std::string {value};
}

int seed_formats(const std::filesystem::path& directory) {
    const auto library_path = directory / "library.format";
    const auto library_present = file_present(library_path);
    if (!library_present) {
        return 1;
    }
    if (*library_present) {
        itunes_formatter.log_print("Seed skipped; library.format already exists");
    }
    else {
        const itunes::library_format::detail::LibraryFormat format;
        if (!replace_format_file(
                library_path,
                itunes::library_format::detail::library_format_text(format)
            )) {
            return 1;
        }
    }

    const auto song_path = directory / "song.format";
    const auto song_present = file_present(song_path);
    if (!song_present) {
        return 1;
    }
    if (*song_present) {
        itunes_formatter.log_print("Seed skipped; song.format already exists");
        return 0;
    }
    const auto accepted = itunes::song::detail::accepted_song_format(
        itunes::song::detail::default_template
    );
    if (!accepted) {
        itunes_formatter.log_print("The default song format is invalid.");
        return 1;
    }
    if (!replace_format_file(song_path, *accepted)) {
        return 1;
    }
    return 0;
}

int init_library(const std::filesystem::path& directory) {
    const auto path = directory / "library.format";
    const auto present = file_present(path);
    if (!present) {
        return 1;
    }
    if (*present) {
        itunes_formatter.log_print(
            "Initialization skipped; library.format already exists"
        );
        return 0;
    }

    std::string chosen_text;
    itunes::library_format::detail::CompiledLibraryFormat chosen_compiled;
    while (true) {
        const auto line = read_init_line(
            "format",
            itunes::library_format::detail::default_library_format
        );
        if (!line) {
            return 1;
        }
        const auto compiled =
            itunes::library_format::detail::compile_library_format(*line);
        if (!compiled) {
            std::cout
                << "Enter a format such as [column] or column - column.\n";
            continue;
        }
        chosen_text = *line;
        chosen_compiled = *compiled;
        break;
    }

    int chosen_count = itunes::library_format::detail::default_column_count;
    while (true) {
        const auto line = read_init_line(
            "column_count",
            std::to_string(itunes::library_format::detail::default_column_count)
        );
        if (!line) {
            return 1;
        }
        const auto parsed =
            itunes::library_format::detail::parse_column_count(*line);
        if (!parsed) {
            std::cout << "Enter a positive whole number.\n";
            continue;
        }
        chosen_count = *parsed;
        break;
    }

    itunes::library_format::detail::LibraryFormat accepted;
    accepted.format_text = std::move(chosen_text);
    accepted.compiled = std::move(chosen_compiled);
    accepted.column_count = chosen_count;
    if (!replace_format_file(
            path,
            itunes::library_format::detail::library_format_text(accepted)
        )) {
        return 1;
    }
    return 0;
}

int init_song(const std::filesystem::path& directory) {
    const auto path = directory / "song.format";
    const auto present = file_present(path);
    if (!present) {
        return 1;
    }
    if (*present) {
        itunes_formatter.log_print(
            "Initialization skipped; song.format already exists"
        );
        return 0;
    }

    while (true) {
        const auto line = read_init_line(
            "Song format",
            itunes::song::detail::default_template
        );
        if (!line) {
            return 1;
        }
        const auto compiled = itunes::song::detail::compile_utf8(*line);
        if (!compiled.ok) {
            std::cout << compiled.error << '\n';
            continue;
        }
        const auto accepted = itunes::song::detail::accepted_song_format(*line);
        if (!accepted) {
            std::cout << "Song format is invalid.\n";
            continue;
        }
        if (!replace_format_file(path, *accepted)) {
            return 1;
        }
        return 0;
    }
}

int run_initialization(const bool seed) {
    if (!itunes_ini_readable()) {
        return 1;
    }
    const auto directory = itunes_data_directory();
    if (seed) {
        return seed_formats(directory);
    }
    if (init_library(directory) != 0) {
        return 1;
    }
    return init_song(directory);
}

} // namespace

export int run_itunes_formatter(int argc, char* argv[]) {
    itunes_formatter.log_main("itunes_formatter.exe started");

    const bool seed = argc == 2 && std::string_view {argv[1]} == "--seed";
    const bool init = argc == 2 && std::string_view {argv[1]} == "--init";
    if (argc > 2 || (argc == 2 && !seed && !init)) {
        std::cerr << "Usage: itunes_formatter.exe [--init | --seed]\n";
        return 1;
    }
    if (seed || init) {
        return run_initialization(seed);
    }

    const auto ini_path = ac::paths::config_directory() / "itunes.ini";
    if (ensure_itunes_ini(ini_path) != 0) {
        return 1;
    }

    const auto directory = itunes_data_directory();
    while (true) {
        std::cout
            << "\niTunes formatter\n"
            << "  1. Format song\n"
            << "  2. Configure library columns\n"
            << "  3. Exit\n"
            << "> ";
        std::string choice;
        if (!std::getline(std::cin, choice)) {
            return 0;
        }
        choice = std::string {trim(choice)};
        if (choice == "1") {
            return format_song(directory);
        }
        if (choice == "2") {
            return configure_library_columns(directory);
        }
        if (choice == "3" || choice == "q" || choice == "Q") {
            return 0;
        }
        std::cout << "Unknown option.\n";
    }
}
