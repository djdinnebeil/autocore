/**
 * \file version_check.cxx
 * \brief Retrieves version.txt and prints when the published version is newer.
 */
module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winhttp.h>

module auto_core.main.version_check;

import std;
import auto_core.core.ini;
import auto_core.core.paths;
import auto_core.core.thread;
import auto_core.main.application;
import auto_core.main.defaults;
import auto_core.version;

namespace {

    constexpr DWORD timeout_ms = 2000;
    constexpr DWORD maximum_body_bytes = 64;
    constexpr wchar_t version_host[] = L"raw.githubusercontent.com";
    constexpr wchar_t version_path[] =
        L"/djdinnebeil/autocore/main/version.txt";

    struct Version {
        unsigned long major = 0;
        unsigned long minor = 0;
        std::optional<unsigned long> patch;

        // An empty patch is a finalized MAJOR.MINOR release. It is newer than
        // every MAJOR.MINOR.PATCH release with the same major and minor.
        [[nodiscard]]
        friend std::strong_ordering operator<=>(
            const Version& left,
            const Version& right
        ) noexcept {
            if (const auto major = left.major <=> right.major; major != 0) {
                return major;
            }
            if (const auto minor = left.minor <=> right.minor; minor != 0) {
                return minor;
            }
            if (!left.patch.has_value() && !right.patch.has_value()) {
                return std::strong_ordering::equal;
            }
            if (!left.patch.has_value()) {
                return std::strong_ordering::greater;
            }
            if (!right.patch.has_value()) {
                return std::strong_ordering::less;
            }
            return *left.patch <=> *right.patch;
        }
    };

    struct InstalledVersion {
        Version version;
        std::string text;
        bool check_for_updates = true;
    };

    struct InternetHandle {
        HINTERNET handle = nullptr;

        ~InternetHandle() {
            if (handle != nullptr) {
                WinHttpCloseHandle(handle);
            }
        }

        InternetHandle() = default;
        InternetHandle(const InternetHandle&) = delete;
        InternetHandle& operator=(const InternetHandle&) = delete;
    };

    [[nodiscard]]
    std::string_view trim(std::string_view text) {
        while (!text.empty() &&
               std::isspace(static_cast<unsigned char>(text.front()))) {
            text.remove_prefix(1);
        }
        while (!text.empty() &&
               std::isspace(static_cast<unsigned char>(text.back()))) {
            text.remove_suffix(1);
        }
        return text;
    }

    [[nodiscard]]
    std::optional<Version> parse_version(const std::string_view text) {
        const std::string_view trimmed = trim(text);
        if (trimmed.empty()) {
            return std::nullopt;
        }

        Version version;
        const char* cursor = trimmed.data();
        const char* const end = trimmed.data() + trimmed.size();
        const auto read_number = [&](unsigned long& part) {
            if (cursor == end ||
                !std::isdigit(static_cast<unsigned char>(*cursor))) {
                return false;
            }
            const auto parsed = std::from_chars(cursor, end, part);
            if (parsed.ec != std::errc {}) {
                return false;
            }
            cursor = parsed.ptr;
            return true;
        };

        if (!read_number(version.major) ||
            cursor == end ||
            *cursor != '.') {
            return std::nullopt;
        }
        ++cursor;
        if (!read_number(version.minor)) {
            return std::nullopt;
        }
        if (cursor == end) {
            return version;
        }
        if (*cursor != '.') {
            return std::nullopt;
        }
        ++cursor;
        unsigned long patch = 0;
        if (!read_number(patch) || cursor != end) {
            return std::nullopt;
        }
        version.patch = patch;
        return version;
    }

    [[nodiscard]]
    bool enabled_value(
        const std::optional<std::string_view> value,
        const bool fallback
    ) {
        if (!value) {
            return fallback;
        }
        if (*value == "off") {
            return false;
        }
        if (*value == "on") {
            return true;
        }
        return fallback;
    }

    [[nodiscard]]
    InstalledVersion read_installed_version() {
        const auto fallback = parse_version(
            ac::version::auto_core_version
        );
        InstalledVersion installed {
            .version = *fallback,
            .text = std::string {ac::version::auto_core_version},
            .check_for_updates = ac::main::defaults::auto_core_check_for_updates,
        };

        const auto document = ac::ini::read(
            ac::paths::config_directory() / "auto_core.ini"
        );
        if (!document) {
            return installed;
        }

        installed.check_for_updates = enabled_value(
            document->find("auto_core", "check_for_updates"),
            installed.check_for_updates
        );
        return installed;
    }

    void skip(const std::string_view reason) {
        auto_core.log_debug("Version check skipped: {}", reason);
    }

    [[nodiscard]]
    std::optional<std::string> fetch_remote_version() {
        InternetHandle session;
        session.handle = WinHttpOpen(
            L"Auto Core",
            WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0
        );
        if (session.handle == nullptr ||
            !WinHttpSetTimeouts(
                session.handle,
                timeout_ms,
                timeout_ms,
                timeout_ms,
                timeout_ms
            )) {
            return std::nullopt;
        }

        InternetHandle connection;
        connection.handle = WinHttpConnect(
            session.handle,
            version_host,
            INTERNET_DEFAULT_HTTPS_PORT,
            0
        );
        if (connection.handle == nullptr) {
            return std::nullopt;
        }

        InternetHandle request;
        request.handle = WinHttpOpenRequest(
            connection.handle,
            L"GET",
            version_path,
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE
        );
        if (request.handle == nullptr ||
            !WinHttpSendRequest(
                request.handle,
                WINHTTP_NO_ADDITIONAL_HEADERS,
                0,
                WINHTTP_NO_REQUEST_DATA,
                0,
                0,
                0
            ) ||
            !WinHttpReceiveResponse(request.handle, nullptr)) {
            return std::nullopt;
        }

        DWORD status = 0;
        DWORD status_size = sizeof(status);
        if (!WinHttpQueryHeaders(
                request.handle,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX,
                &status,
                &status_size,
                WINHTTP_NO_HEADER_INDEX
            ) ||
            status != HTTP_STATUS_OK) {
            return std::nullopt;
        }

        std::string body;
        while (body.size() < maximum_body_bytes) {
            DWORD available = 0;
            if (!WinHttpQueryDataAvailable(request.handle, &available)) {
                return std::nullopt;
            }
            if (available == 0) {
                return body;
            }
            const DWORD room =
                maximum_body_bytes - static_cast<DWORD>(body.size());
            if (available > room) {
                return std::nullopt;
            }
            const auto offset = body.size();
            body.resize(offset + available);
            DWORD read = 0;
            if (!WinHttpReadData(
                    request.handle,
                    body.data() + offset,
                    available,
                    &read
                )) {
                return std::nullopt;
            }
            body.resize(offset + read);
            if (read == 0) {
                return body;
            }
        }
        return body;
    }

    void check_for_newer_version(const InstalledVersion& installed) {
        const auto body = fetch_remote_version();
        if (!body) {
            skip("request failed");
            return;
        }

        const std::string_view remote_text = trim(*body);
        const auto remote = parse_version(remote_text);
        if (!remote) {
            skip("unrecognized version text");
            return;
        }
        if (*remote <= installed.version) {
            return;
        }

        auto_core.print(
            "[Auto Core] Update available: {} -> {}",
            installed.text,
            remote_text
        );
    }

} // namespace

std::jthread start_version_check() {
    const InstalledVersion installed = read_installed_version();
    if (!installed.check_for_updates) {
        return {};
    }

    return std::jthread {[installed] {
        ac::thread::run_with_exception_handling(
            [installed] { check_for_newer_version(installed); },
            auto_core
        );
    }};
}
