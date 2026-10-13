module;

#include "core_config_detail.hpp"

module auto_core.core.config;

import std;
import auto_core.core.ini;
import auto_core.core.paths;

namespace ac::config {

    namespace {

        constexpr CoreSettings default_settings {
            .missing_winkey_mapping = true,
            .unmapped_key = true
        };

        void append_report(std::string& report, const std::string_view text) {
            if (!report.empty()) {
                report.push_back(' ');
            }
            report += text;
        }

        struct Data {
            std::optional<CoreSettings> settings;
            std::string report;

            Data() {
                const auto path =
                    ac::paths::config_directory() / "warnings.ini";
                const auto document = ac::ini::read(path);
                if (!document) {
                    settings = default_settings;
                    std::error_code exists_error;
                    const bool present =
                        std::filesystem::exists(path, exists_error);
                    if (present && !exists_error) {
                        report =
                            "config/warnings.ini is malformed. Run "
                            "warnings_config.exe to generate a valid file. "
                            "Using built-in defaults; the file will not be "
                            "created.";
                    }
                    else {
                        report =
                            "config/warnings.ini is missing. Run "
                            "warnings_config.exe to generate it. Using "
                            "built-in defaults; the file will not be created.";
                    }
                    return;
                }

                const auto winkey = document->find(
                    "warnings",
                    "missing_winkey_mapping"
                );
                const auto unmapped = document->find(
                    "warnings",
                    "unmapped_key"
                );
                const auto resolved = detail::resolve({
                    .missing_winkey_mapping = winkey,
                    .unmapped_key = unmapped
                });
                settings = CoreSettings {
                    .missing_winkey_mapping = resolved.missing_winkey_mapping,
                    .unmapped_key = resolved.unmapped_key
                };

                if (!winkey) {
                    append_report(
                        report,
                        "config/warnings.ini is missing [warnings] "
                        "missing_winkey_mapping. Run warnings_config.exe to "
                        "repair it. Using the default; the file will not be "
                        "rewritten."
                    );
                }
                else if (*winkey != "on" && *winkey != "off") {
                    append_report(
                        report,
                        "config/warnings.ini [warnings] "
                        "missing_winkey_mapping is invalid. Run "
                        "warnings_config.exe to repair it. Using the default; "
                        "the file will not be rewritten."
                    );
                }

                if (!unmapped) {
                    append_report(
                        report,
                        "config/warnings.ini is missing [warnings] "
                        "unmapped_key. Run warnings_config.exe to repair it. "
                        "Using the default; the file will not be rewritten."
                    );
                }
                else if (*unmapped != "on" && *unmapped != "off") {
                    append_report(
                        report,
                        "config/warnings.ini [warnings] unmapped_key is "
                        "invalid. Run warnings_config.exe to repair it. Using "
                        "the default; the file will not be rewritten."
                    );
                }
            }
        };

        const Data& data() {
            static const Data value;
            return value;
        }

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
