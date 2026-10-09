#include "../shared/server_data_detail.hpp"

import std;
import auto_core.core.ini;
import auto_core.core.paths;
import component_settings;
import auto_core.core.shell;

namespace {

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

bool file_missing(const std::filesystem::path& path) {
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    return error || !present;
}

bool scalars_incomplete() {
    const auto ini_path = ac::paths::config_directory() / "server.ini";
    const auto document = ac::ini::read(ini_path);
    if (!document) {
        return true;
    }

    std::optional<std::string> stored;
    if (const auto value = document->find("server", "directory")) {
        const auto text = trim(*value);
        if (!text.empty()) {
            stored = std::string {text};
        }
    }
    std::optional<std::string_view> view;
    if (stored) {
        view = *stored;
    }
    const auto directory = server::data::resolve_directory(
        view,
        ac::paths::installation_root()
    );
    return file_missing(directory / "port.id") ||
        file_missing(directory / "document_root.id");
}

} // namespace

int main() {
    ac::shell::set_process_app_user_model_id();
    const ac::component_settings::DelegatedAction actions[] {
        {
            .label = "Edit port and document root",
            .executable = "server_editor.exe",
        },
        {
            .label = "Create default site files",
            .executable = "server_builder.exe",
            .arguments = {"--seed"},
        },
    };
    return ac::component_settings::run("server", actions, scalars_incomplete);
}
