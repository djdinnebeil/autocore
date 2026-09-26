import std;
import auto_core.core.component;
import auto_core.core.paths;
import itunes_defaults;
import components_editor_request;

import <iostream>;

namespace {

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

bool write_bytes(
    ac::Component& log,
    const std::filesystem::path& path,
    std::string_view contents
) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        log.log_print(
            "Failed to create config directory: {}",
            error.message()
        );
        return false;
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        log.log_print("Failed to create {}", path.string());
        return false;
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    output.close();
    return static_cast<bool>(output);
}

} // namespace

int main(int argc, char* argv[]) {
    ac::Component itunes_config {"itunes_config"};
    itunes_config.log_main("itunes_config.exe started");

    const bool initialize =
        ac::config::components_request::is_initialize_run(argc, argv);
    const auto path = ac::paths::config_directory() / "itunes.ini";
    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        itunes_config.log_print(
            "config/itunes.ini already exists.\n"
            "auto_start default: {}\n"
            "tab_end default: {}",
            itunes::defaults::auto_start ? "true" : "false",
            itunes::defaults::tab_end
        );
        return ac::config::components_request::run_component_update("itunes");
    }

    if (initialize) {
        if (!write_bytes(itunes_config, path, itunes::defaults::ini_text)) {
            return 1;
        }
        itunes_config.log_print("Wrote {}", path.string());
        return ac::config::components_request::run_component_update("itunes");
    }

    std::cout
        << "iTunes auto_start ["
        << (itunes::defaults::auto_start ? "true" : "false")
        << "]: ";
    std::string auto_start_input;
    if (!std::getline(std::cin, auto_start_input)) {
        itunes_config.log_print("Failed to read auto_start.");
        return 1;
    }
    auto auto_start = itunes::defaults::auto_start;
    const auto auto_start_value = trim(auto_start_input);
    if (auto_start_value == "true") {
        auto_start = true;
    }
    else if (auto_start_value == "false") {
        auto_start = false;
    }
    else if (!auto_start_value.empty()) {
        std::cout << "Enter true or false. Using default.\n";
    }

    std::cout << "iTunes tab_end [" << itunes::defaults::tab_end << "]: ";
    std::string tab_input;
    if (!std::getline(std::cin, tab_input)) {
        itunes_config.log_print("Failed to read tab_end.");
        return 1;
    }
    int tab_end = itunes::defaults::tab_end;
    const auto tab_value = trim(tab_input);
    if (!tab_value.empty()) {
        int parsed = 0;
        const auto result = std::from_chars(
            tab_value.data(),
            tab_value.data() + tab_value.size(),
            parsed
        );
        if (result.ec == std::errc {} &&
            result.ptr == tab_value.data() + tab_value.size() &&
            parsed >= 0) {
            tab_end = parsed;
        }
        else {
            std::cout << "Invalid tab_end. Using default.\n";
        }
    }

    const std::string contents =
        "[itunes]\nauto_start = " +
        std::string {auto_start ? "true" : "false"} +
        "\ntab_end = " +
        std::to_string(tab_end) +
        "\n";
    if (!write_bytes(itunes_config, path, contents)) {
        return 1;
    }
    itunes_config.log_print("Wrote {}", path.string());
    return ac::config::components_request::run_component_update("itunes");
}
