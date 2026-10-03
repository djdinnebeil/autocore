module;

#include "itunes_config_detail.hpp"
#include "../shared/library_format_detail.hpp"
#include "../shared/song_template_detail.hpp"

module itunes_client;

import std;
import auto_core.core.ini;
import auto_core.core.paths;
import itunes_component;

namespace {

void load_song_format(itunes_client& client) {
    const auto loaded = itunes::song::detail::load_song_format_file(
        client.data_directory / "song.format"
    );
    if (loaded.invalid) {
        itunes_component.log_print("{}", loaded.error);
    }
    client.song_format = loaded.compiled;
}

void load_library_format(itunes_client& client) {
    const auto loaded = itunes::library_format::detail::load_library_format(
        client.data_directory / "library.format"
    );
    if (loaded.error) {
        itunes_component.log_print("{}", *loaded.error);
    }
    client.library_format = loaded.format;
}

} // namespace

void itunes_client::set_config() {
    const std::filesystem::path itunes_config_path =
        ac::paths::config_directory() / "itunes.ini";

    const auto document = ac::ini::read(itunes_config_path);
    std::optional<std::string_view> stored_directory;
    if (!document) {
        std::error_code exists_error;
        const bool present =
            std::filesystem::exists(itunes_config_path, exists_error);
        itunes_component.report_ini_unavailable(present && !exists_error);
    }
    else {
        const auto raw_auto_start = document->find("itunes", "auto_start");
        const auto settings = itunes::config::detail::resolve(
            {
                .directory = document->find("itunes", "directory"),
                .auto_start = raw_auto_start
            },
            {
                .auto_start = auto_start
            }
        );

        auto_start = settings.auto_start;
        stored_directory = document->find("itunes", "directory");

        itunes_component.log_main(
            "itunes.ini {}: auto_start raw={} resolved={}",
            itunes_config_path,
            raw_auto_start.value_or("absent"),
            auto_start ? "on" : "off"
        );
    }

    data_directory = itunes::config::detail::resolve_directory(
        stored_directory,
        ac::paths::installation_root()
    );
    load_song_format(*this);
    load_library_format(*this);
}
