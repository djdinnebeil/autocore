module itunes_c;

import std;
import auto_core.ini;
import auto_core.paths;

void iTunes::set_config() {
    const std::filesystem::path itunes_config_path =
        ac::paths::config_directory() / "itunes.ini";

    const auto document = ac::ini::read(itunes_config_path);
    if (!document) {
        return;
    }

    if (const auto value = document->find("itunes", "start_automatically")) {
        auto_start = *value == "true";
    }

    if (const auto value = document->find("itunes", "tab_end")) {
        int parsed_tab_end {};
        const auto [end, error] = std::from_chars(
            value->data(),
            value->data() + value->size(),
            parsed_tab_end
        );
        if (error == std::errc {} && end == value->data() + value->size()) {
            tab_end = parsed_tab_end;
        }
    }
}
