#include "server_data_detail.hpp"

#include <charconv>

namespace server::data {

    namespace {

        [[nodiscard]] std::string_view trim(std::string_view value) noexcept {
            const auto first = value.find_first_not_of(" \t\r\n");
            if (first == std::string_view::npos) {
                return {};
            }
            const auto last = value.find_last_not_of(" \t\r\n");
            return value.substr(first, last - first + 1);
        }

        [[nodiscard]] bool stays_inside(
            const std::filesystem::path& data_directory,
            const std::filesystem::path& candidate
        ) {
            const auto relative =
                candidate.lexically_normal().lexically_relative(
                    data_directory.lexically_normal()
                );
            if (relative.empty()) {
                return false;
            }
            for (const auto& part : relative) {
                if (part == "..") {
                    return false;
                }
            }
            return true;
        }

    } // namespace

    std::optional<int> parse_port(std::string_view text) {
        const auto value = trim(text);
        if (value.empty()) {
            return std::nullopt;
        }
        if (value.find('\n') != std::string_view::npos ||
            value.find('\r') != std::string_view::npos) {
            return std::nullopt;
        }

        int parsed = 0;
        const auto result = std::from_chars(
            value.data(),
            value.data() + value.size(),
            parsed
        );
        if (result.ec != std::errc {} ||
            result.ptr != value.data() + value.size() ||
            parsed < 1 ||
            parsed > 65535) {
            return std::nullopt;
        }
        return parsed;
    }

    std::optional<std::string> parse_document_root(std::string_view text) {
        const auto value = trim(text);
        if (value.empty()) {
            return std::nullopt;
        }
        if (value.find('\n') != std::string_view::npos ||
            value.find('\r') != std::string_view::npos) {
            return std::nullopt;
        }
        return std::string {value};
    }

    bool document_root_is_absolute(std::string_view stored) {
        try {
            return std::filesystem::path {std::string {stored}}.is_absolute();
        }
        catch (...) {
            return false;
        }
    }

    std::filesystem::path resolve_directory(
        const std::optional<std::string_view>& stored,
        const std::filesystem::path& installation_root
    ) {
        const auto fallback =
            (installation_root / "components" / "server").lexically_normal();
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

    std::filesystem::path resolve_document_root(
        std::string_view stored,
        const std::filesystem::path& data_directory
    ) {
        std::filesystem::path configured {std::string {stored}};
        if (configured.is_absolute()) {
            return configured.lexically_normal();
        }
        return (data_directory / configured).lexically_normal();
    }

    std::optional<std::filesystem::path> builder_document_root(
        std::string_view stored,
        const std::filesystem::path& data_directory
    ) {
        const auto parsed = parse_document_root(stored);
        if (!parsed) {
            return std::nullopt;
        }

        try {
            std::filesystem::path configured {*parsed};
            if (configured.empty() || configured.is_absolute()) {
                return std::nullopt;
            }
            const auto resolved = (data_directory / configured).lexically_normal();
            if (!stays_inside(data_directory, resolved)) {
                return std::nullopt;
            }
            return resolved;
        }
        catch (...) {
            return std::nullopt;
        }
    }

    DocumentRootStartup document_root_startup(bool directory_exists) noexcept {
        if (directory_exists) {
            return DocumentRootStartup::serve;
        }
        return DocumentRootStartup::fail;
    }

} // namespace server::data
