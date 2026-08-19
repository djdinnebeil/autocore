module app_config;

import std;
import auto_core.encoding;
import auto_core.ini;
import auto_core.paths;

namespace app_config {
    std::wstring_view program_title() {
        static const std::wstring title = [] {
            const auto document = ac::ini::read(
                ac::paths::config_directory() / "app.ini"
            );
            if (!document) {
                return std::wstring {L"Auto Core"};
            }

            const auto configured =
                document->find("app", "program_title");
            return configured && !configured->empty()
                ? ac::encoding::to_utf16(*configured)
                : std::wstring {L"Auto Core"};
        }();

        return title;
    }
}
