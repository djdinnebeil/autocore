#include "itunes_config_detail.hpp"

namespace itunes::config::detail {

    namespace {

        [[nodiscard]] std::string_view trim(std::string_view value) noexcept {
            const auto first = value.find_first_not_of(" \t");
            if (first == std::string_view::npos) {
                return {};
            }
            const auto last = value.find_last_not_of(" \t");
            return value.substr(first, last - first + 1);
        }

        std::optional<bool> parse_bool(
            const std::optional<std::string_view> value
        ) noexcept {
            if (value == "on") {
                return true;
            }
            if (value == "off") {
                return false;
            }
            return std::nullopt;
        }

        [[nodiscard]] std::filesystem::path default_directory(
            const std::filesystem::path& installation_root
        ) {
            return (installation_root / "components" / "itunes").lexically_normal();
        }

    } // namespace

    Settings resolve(const RawSettings& raw, Settings defaults) {
        if (raw.directory) {
            const auto directory = trim(*raw.directory);
            if (!directory.empty()) {
                defaults.directory = std::string {directory};
            }
        }

        if (const auto value = parse_bool(raw.auto_start)) {
            defaults.auto_start = *value;
        }

        return defaults;
    }

    std::filesystem::path resolve_directory(
        const std::optional<std::string_view>& stored,
        const std::filesystem::path& installation_root
    ) {
        const auto fallback = default_directory(installation_root);
        if (!stored) {
            return fallback;
        }

        const auto text = trim(*stored);
        if (text.empty()) {
            return fallback;
        }

        try {
            std::filesystem::path resolved {std::string {text}};
            if (resolved.empty()) {
                return fallback;
            }
            if (resolved.is_relative()) {
                resolved = installation_root / resolved;
            }
            return resolved.lexically_normal();
        }
        catch (...) {
            return fallback;
        }
    }

} // namespace itunes::config::detail
