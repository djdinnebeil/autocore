module;

#include "itunes_config_detail.hpp"

module itunes_client;

import std;
import auto_core.core.ini;
import auto_core.core.paths;
import itunes_component;

void itunes_client::set_config() {
    const std::filesystem::path itunes_config_path =
        ac::paths::config_directory() / "itunes.ini";

    const auto document = ac::ini::read(itunes_config_path);
    if (!document) {
        std::error_code exists_error;
        const bool present =
            std::filesystem::exists(itunes_config_path, exists_error);
        itunes_component.report_ini_unavailable(present && !exists_error);
        return;
    }

    const auto raw_auto_start = document->find("itunes", "auto_start");
    const auto settings = itunes::config::detail::resolve(
        {
            .auto_start = raw_auto_start,
            .tab_end = document->find("itunes", "tab_end")
        },
        {
            .auto_start = auto_start,
            .tab_end = tab_end
        }
    );

    auto_start = settings.auto_start;
    tab_end = settings.tab_end;

    itunes_component.log_main(
        "itunes.ini {}: auto_start raw={} resolved={}",
        itunes_config_path,
        raw_auto_start.value_or("absent"),
        auto_start
    );
}
