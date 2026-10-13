#include "core_config_detail.hpp"

namespace ac::config::detail {

    Settings resolve(const RawSettings& raw) {
        return Settings {
            .missing_winkey_mapping =
                raw.missing_winkey_mapping != "off",
            .unmapped_key =
                raw.unmapped_key != "off"
        };
    }

} // namespace ac::config::detail
