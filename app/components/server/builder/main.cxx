/**
 * \file main.cxx
 * \brief Creates missing Server starter files for `server_builder.exe`.
 *
 * `--seed` is the only successful invocation. It resolves the Server data
 * directory from `config/server.ini`, then reads `document_root.id`.
 * `index.html` and `styles.css` are created independently when each file
 * is missing. An existing file is left unchanged. Starter files are written
 * only for a relative document root that stays inside the data directory.
 * An absolute document root is left untouched.
 */
#include "../shared/server_data_detail.hpp"

import std;

import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.ini;
import auto_core.core.paths;
import server_default_site;

import <iostream>;
import auto_core.core.shell;

namespace {

ac::Component server_builder {
    "server_builder",
    ac::logging::config::LoggingScope {"server"}
};

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

enum class DirectoryLoad {
    ready,
    unreadable
};

struct DataDirectory {
    DirectoryLoad load {DirectoryLoad::unreadable};
    std::filesystem::path path;
};

DataDirectory load_data_directory() {
    const auto ini_path = ac::paths::config_directory() / "server.ini";
    std::error_code error;
    const bool present = std::filesystem::exists(ini_path, error);
    if (error) {
        server_builder.log_print(
            "Failed to inspect {}: {}",
            ini_path.string(),
            error.message()
        );
        return {};
    }
    if (!present) {
        return {
            DirectoryLoad::ready,
            server::data::resolve_directory(
                std::nullopt,
                ac::paths::installation_root()
            )
        };
    }

    const auto document = ac::ini::read(ini_path);
    if (!document) {
        server_builder.log_print(
            "Failed to read {}. Site files were not written.",
            ini_path.string()
        );
        return {};
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
    return {
        DirectoryLoad::ready,
        server::data::resolve_directory(view, ac::paths::installation_root())
    };
}

bool write_if_missing(
    const std::filesystem::path& path,
    const std::string_view contents
) {
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error) {
        server_builder.log_print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return false;
    }
    if (present) {
        server_builder.log_print("Left existing {} unchanged.", path.string());
        return true;
    }

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        server_builder.log_print("Failed to create {}", path.string());
        return false;
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    output.close();
    if (!output) {
        server_builder.log_print("Failed to write {}", path.string());
        return false;
    }
    server_builder.log_print("Created {}", path.string());
    return true;
}

int seed_site() {
    const auto directory = load_data_directory();
    if (directory.load != DirectoryLoad::ready) {
        return 1;
    }

    const auto scalar_path = directory.path / "document_root.id";
    std::error_code error;
    const bool present = std::filesystem::exists(scalar_path, error);
    if (error || !present) {
        server_builder.log_print(
            "Server document_root.id is missing or invalid. Run server_editor.exe to configure Server."
        );
        return 1;
    }

    std::ifstream input(scalar_path, std::ios::binary);
    if (!input) {
        server_builder.log_print(
            "Server document_root.id is missing or invalid. Run server_editor.exe to configure Server."
        );
        return 1;
    }
    std::ostringstream contents;
    contents << input.rdbuf();
    const auto stored = server::data::parse_document_root(contents.str());
    if (!stored) {
        server_builder.log_print(
            "Server document_root.id is missing or invalid. Run server_editor.exe to configure Server."
        );
        return 1;
    }

    if (server::data::document_root_is_absolute(*stored)) {
        server_builder.log_print(
            "document_root.id is absolute. server_builder.exe does not modify that directory."
        );
        return 1;
    }

    const auto document_root = server::data::builder_document_root(
        *stored,
        directory.path
    );
    if (!document_root) {
        server_builder.log_print(
            "document_root.id resolves outside the Server data directory. Starter files were not written."
        );
        return 1;
    }

    std::filesystem::create_directories(*document_root, error);
    if (error) {
        server_builder.log_print(
            "Failed to create {}: {}",
            document_root->string(),
            error.message()
        );
        return 1;
    }

    const bool index_written = write_if_missing(
        *document_root / "index.html",
        server::default_site::index_html
    );
    const bool styles_written = write_if_missing(
        *document_root / "styles.css",
        server::default_site::styles_css
    );
    return index_written && styles_written ? 0 : 1;
}

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    server_builder.log_main("server_builder.exe started");
    if (argc != 2 || std::string_view {argv[1]} != "--seed") {
        std::cerr << "Usage: server_builder.exe --seed\n";
        return 1;
    }
    return seed_site();
}
