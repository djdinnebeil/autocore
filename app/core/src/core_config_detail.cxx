#include "core_config_detail.hpp"

namespace ac::config::detail {

    Settings resolve(const RawSettings& raw) {
        return Settings {
            .warn_without_winkey_mapping =
                raw.warn_without_winkey_mapping != "false"
        };
    }

} // namespace ac::config::detail
