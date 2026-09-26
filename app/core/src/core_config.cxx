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

        constexpr CoreSettings default_settings {
            .warn_without_winkey_mapping = true
        };

        struct Data {
            std::optional<CoreSettings> settings;
            std::string report;

            Data() {
                const auto path =
                    ac::paths::config_directory() / "auto_core.ini";
                const auto document = ac::ini::read(path);
                if (!document) {
                    settings = default_settings;
                    std::error_code exists_error;
                    const bool present =
                        std::filesystem::exists(path, exists_error);
                    if (present && !exists_error) {
                        report =
                            "config/auto_core.ini is malformed. Run "
                            "auto_core_config.exe to generate a valid file. "
                            "Using built-in defaults; the file will not be "
                            "created.";
                    }
                    else {
                        report =
                            "config/auto_core.ini is missing. Run "
                            "auto_core_config.exe to generate it. Using "
                            "built-in defaults; the file will not be created.";
                    }
                    return;
                }

                const auto raw = document->find(
                    "auto_core",
                    "warn_without_winkey_mapping"
                );
                if (!raw) {
                    settings = default_settings;
                    report =
                        "config/auto_core.ini is missing [auto_core] "
                        "warn_without_winkey_mapping. Run auto_core_config.exe "
                        "to repair it. Using built-in defaults; the file will "
                        "not be rewritten.";
                    return;
                }

                const auto resolved = detail::resolve({
                    .warn_without_winkey_mapping = raw
                });
                settings = CoreSettings {
                    .warn_without_winkey_mapping =
                        resolved.warn_without_winkey_mapping
                };
                if (*raw != "true" && *raw != "false") {
                    report =
                        "config/auto_core.ini [auto_core] "
                        "warn_without_winkey_mapping is invalid. Run "
                        "auto_core_config.exe to repair it. Using the "
                        "default; the file will not be rewritten.";
                }
            }
        };

        const Data& data() {
            static const Data value;
            return value;
        }

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

    void initialize_core_settings() {
        (void)data();
    }

    std::string_view core_settings_report() noexcept {
        return data().report;
    }

    const CoreSettings& core_settings() noexcept {
        const Data& value = data();
        if (!value.settings) {
            std::terminate();
        }
        return *value.settings;
    }

} // namespace ac::config
