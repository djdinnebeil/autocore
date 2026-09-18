module;

#include "config_defaults.hpp"
#include "core_config_detail.hpp"

module auto_core.core.config;

import std;
import auto_core.core.ini;
import auto_core.core.paths;

import <iostream>;

namespace ac::config {

    namespace {

        bool write_if_missing(
            const std::filesystem::path& path,
            const std::string_view contents
        ) {
            std::error_code ec;
            if (std::filesystem::exists(path, ec)) {
                return true;
            }
            if (ec) {
                std::cerr
                    << "Failed to inspect "
                    << path.string()
                    << ": "
                    << ec.message()
                    << '\n';
                return false;
            }

            std::ofstream output(path, std::ios::binary);
            if (!output) {
                std::cerr << "Failed to create " << path.string() << '\n';
                return false;
            }

            output.write(
                contents.data(),
                static_cast<std::streamsize>(contents.size())
            );
            output.close();
            if (!output) {
                std::cerr << "Failed to write " << path.string() << '\n';
                return false;
            }
            return true;
        }

        std::string journal_ini_for_directory(std::string_view directory) {
            const std::string value =
                directory.empty() ? std::string {"journal"} : std::string {directory};
            if (value == "journal") {
                return std::string {detail::journal_ini};
            }
            return std::string {
                "[journal]\n"
                "directory = "
            } + value +
                "\n"
                "series =\n"
                "remote_sync = disable\n"
                "\n"
                "[timestamp]\n"
                "day_rollover_hour = 0\n";
        }

        std::string taskbar_ini_for_directory(std::string_view directory) {
            const std::string value =
                directory.empty() ? std::string {"taskbar"} : std::string {directory};
            if (value == "taskbar") {
                return std::string {detail::taskbar_ini};
            }
            return std::string {
                "[taskbar]\n"
                "directory = "
            } + value +
                "\n"
                "mode = live\n";
        }

        bool write_file(
            const std::filesystem::path& path,
            const std::string_view contents
        ) {
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            if (!output) {
                std::cerr << "Failed to create " << path.string() << '\n';
                return false;
            }

            output.write(
                contents.data(),
                static_cast<std::streamsize>(contents.size())
            );
            output.close();
            if (!output) {
                std::cerr << "Failed to write " << path.string() << '\n';
                return false;
            }
            return true;
        }

        bool create_config_directory() {
            std::error_code ec;
            std::filesystem::create_directories(
                ac::paths::config_directory(),
                ec
            );
            if (ec) {
                std::cerr
                    << "Failed to create config directory: "
                    << ec.message()
                    << '\n';
                return false;
            }
            return true;
        }

        int parse_server_port(std::string_view value) {
            int parsed_port = 0;
            const auto result = std::from_chars(
                value.data(),
                value.data() + value.size(),
                parsed_port
            );
            if (result.ec == std::errc {} &&
                result.ptr == value.data() + value.size() &&
                parsed_port >= 1 &&
                parsed_port <= 65535) {
                return parsed_port;
            }
            return 8585;
        }

        int stored_server_port() {
            const auto document = ac::ini::read(
                ac::paths::config_directory() / "server.ini"
            );
            if (!document) {
                return 8585;
            }
            if (const auto value = document->find("server", "port")) {
                return parse_server_port(*value);
            }
            return 8585;
        }

        std::string stored_server_document_root() {
            const auto document = ac::ini::read(
                ac::paths::config_directory() / "server.ini"
            );
            if (!document) {
                return "server";
            }
            if (const auto value = document->find("server", "document_root")) {
                if (!value->empty()) {
                    return std::string {*value};
                }
            }
            return "server";
        }

        std::string server_ini_for(
            std::string_view document_root,
            int port
        ) {
            const std::string root =
                document_root.empty()
                    ? std::string {"server"}
                    : std::string {document_root};
            if (root == "server" && port == 8585) {
                return std::string {detail::server_ini};
            }
            return std::string {
                "[server]\n"
                "port = "
            } + std::to_string(port) +
                "\n"
                "document_root = " +
                root +
                "\n";
        }

        std::string writer_ini_for_directories(
            std::string_view directory,
            std::string_view notes_directory
        ) {
            const std::string value =
                directory.empty() ? std::string {"writer"} : std::string {directory};
            const std::string notes =
                notes_directory.empty()
                    ? std::string {"notes"}
                    : std::string {notes_directory};
            if (value == "writer" && notes == "notes") {
                return std::string {detail::writer_ini};
            }
            return std::string {
                "[writer]\n"
                "directory = "
            } + value +
                "\n"
                "notes_directory = " +
                notes +
                "\n";
        }

        constexpr CoreSettings default_settings {
            .warn_without_winkey_mapping = true
        };

        struct Data {
            std::optional<CoreSettings> settings;

            Data() {
                seed_missing_config_files();
                seed_missing_journal_choices();

                const auto document = ac::ini::read(
                    ac::paths::config_directory() / "auto_core.ini"
                );
                if (!document) {
                    settings = default_settings;
                    return;
                }

                const auto resolved = detail::resolve({
                    .warn_without_winkey_mapping = document->find(
                        "auto_core", "warn_without_winkey_mapping"
                    )
                });
                settings = CoreSettings {
                    .warn_without_winkey_mapping =
                        resolved.warn_without_winkey_mapping
                };
            }
        };

        const Data& data() {
            static const Data value;
            return value;
        }

    }

    void seed_missing_config_files() {
        std::error_code ec;
        std::filesystem::create_directories(
            ac::paths::config_directory(),
            ec
        );
        if (ec) {
            std::cerr
                << "Failed to create config directory: "
                << ec.message()
                << '\n';
            return;
        }

        const auto& directory = ac::paths::config_directory();
        write_if_missing(directory / "auto_core.ini", detail::auto_core_ini);
        write_if_missing(directory / "logger.ini", detail::logger_ini);
        write_if_missing(directory / "server.ini", detail::server_ini);
        write_if_missing(
            directory / "crash_recovery.ini",
            detail::crash_recovery_ini
        );
        write_if_missing(directory / "shutdown.ini", detail::shutdown_ini);
        write_if_missing(directory / "itunes.ini", detail::itunes_ini);
        write_if_missing(directory / "journal.ini", detail::journal_ini);
        write_if_missing(directory / "taskbar.ini", detail::taskbar_ini);
        write_if_missing(directory / "keymap.ini", detail::keymap_ini);
        write_if_missing(directory / "spotify.ini", detail::spotify_ini);
        write_if_missing(directory / "writer.ini", detail::writer_ini);
        write_if_missing(directory / "components.list", detail::components_list);
    }

    void seed_missing_journal_choices() {
        std::error_code ec;
        std::filesystem::create_directories(
            ac::paths::journal_directory(),
            ec
        );
        if (ec) {
            std::cerr
                << "Failed to create journal directory: "
                << ec.message()
                << '\n';
            return;
        }

        write_if_missing(
            ac::paths::journal_directory() / "journal_choices.ini",
            detail::journal_choices_ini
        );
    }

    bool write_journal_ini_if_missing(std::string_view directory) {
        std::error_code ec;
        std::filesystem::create_directories(
            ac::paths::config_directory(),
            ec
        );
        if (ec) {
            std::cerr
                << "Failed to create config directory: "
                << ec.message()
                << '\n';
            return false;
        }

        const auto contents = journal_ini_for_directory(directory);
        return write_if_missing(
            ac::paths::config_directory() / "journal.ini",
            contents
        );
    }

    bool write_taskbar_ini_if_missing(std::string_view directory) {
        std::error_code ec;
        std::filesystem::create_directories(
            ac::paths::config_directory(),
            ec
        );
        if (ec) {
            std::cerr
                << "Failed to create config directory: "
                << ec.message()
                << '\n';
            return false;
        }

        const auto contents = taskbar_ini_for_directory(directory);
        return write_if_missing(
            ac::paths::config_directory() / "taskbar.ini",
            contents
        );
    }

    bool write_writer_ini_if_missing(
        std::string_view directory,
        std::string_view notes_directory
    ) {
        if (!create_config_directory()) {
            return false;
        }

        const auto contents = writer_ini_for_directories(
            directory,
            notes_directory
        );
        return write_if_missing(
            ac::paths::config_directory() / "writer.ini",
            contents
        );
    }

    bool write_server_ini_if_missing(
        std::string_view document_root,
        int port
    ) {
        if (port < 1 || port > 65535) {
            return false;
        }
        if (!create_config_directory()) {
            return false;
        }

        return write_if_missing(
            ac::paths::config_directory() / "server.ini",
            server_ini_for(document_root, port)
        );
    }

    bool write_server_ini_port(int port) {
        if (port < 1 || port > 65535) {
            return false;
        }
        if (!create_config_directory()) {
            return false;
        }

        return write_file(
            ac::paths::config_directory() / "server.ini",
            server_ini_for(stored_server_document_root(), port)
        );
    }

    bool write_server_ini_document_root(std::string_view document_root) {
        if (!create_config_directory()) {
            return false;
        }

        return write_file(
            ac::paths::config_directory() / "server.ini",
            server_ini_for(document_root, stored_server_port())
        );
    }

    void initialize_core_settings() {
        (void)data();
    }

    const CoreSettings& core_settings() noexcept {
        const Data& value = data();
        if (!value.settings) {
            std::terminate();
        }
        return *value.settings;
    }

} // namespace ac::config
