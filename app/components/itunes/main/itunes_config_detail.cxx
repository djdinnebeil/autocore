#include "itunes_config_detail.hpp"

#include <charconv>

namespace itunes::config::detail {

    namespace {
        std::optional<bool> parse_bool(
            const std::optional<std::string_view> value
        ) noexcept {
            if (value == "true") {
                return true;
            }
            if (value == "false") {
                return false;
            }
            return std::nullopt;
        }
    }

    Settings resolve(const RawSettings& raw, Settings defaults) {
        if (const auto value = parse_bool(raw.auto_start)) {
            defaults.auto_start = *value;
        }

        if (raw.tab_end) {
            int parsed_tab_end {};
            const auto parsed = std::from_chars(
                raw.tab_end->data(),
                raw.tab_end->data() + raw.tab_end->size(),
                parsed_tab_end
            );

            if (parsed.ec == std::errc {} &&
                parsed.ptr == raw.tab_end->data() + raw.tab_end->size()) {
                defaults.tab_end = parsed_tab_end;
            }
        }

        return defaults;
    }

} // namespace itunes::config::detail
