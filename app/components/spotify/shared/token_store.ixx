/**
 * \file token_store.ixx
 * \brief `tokens.map` parse, serialize, and coordinated replace.
 *
 * Link this module only into `spotify_oauth.exe`, `spotify_ac.exe`, and tests.
 * The editor does not read or write token state.
 */
export module spotify_token_store;

import std;
import spotify_data_directory;

import <Windows.h>;

export namespace spotify::tokens {

inline constexpr std::wstring_view mutex_name = L"Local\\AutoCoreSpotifyTokens";

struct TokenState {
    std::string access_token;
    std::string refresh_token;
    std::time_t authorized_at {};
    std::time_t refresh_expires_at {};
};

[[nodiscard]] inline std::filesystem::path path() {
    return spotify::data_directory() / "tokens.map";
}

[[nodiscard]] inline std::string_view trim_ws(std::string_view value) noexcept {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

/**
 * \brief Parses a complete token file.
 * \return Empty when any required field is missing, empty, or not a timestamp.
 */
[[nodiscard]] inline std::optional<TokenState> parse(std::string_view text) {
    std::string access_token;
    std::string refresh_token;
    std::optional<std::time_t> authorized_at;
    std::optional<std::time_t> refresh_expires_at;
    bool malformed = false;

    std::size_t cursor = 0;
    while (cursor <= text.size()) {
        const auto end = text.find('\n', cursor);
        auto line = text.substr(
            cursor,
            end == std::string_view::npos ? std::string_view::npos : end - cursor
        );
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }
        cursor = end == std::string_view::npos ? text.size() + 1 : end + 1;
        if (trim_ws(line).empty()) {
            if (end == std::string_view::npos) {
                break;
            }
            continue;
        }
        const auto split = line.find('=');
        if (split == std::string_view::npos) {
            malformed = true;
            break;
        }
        const auto key = trim_ws(line.substr(0, split));
        const auto value = std::string {trim_ws(line.substr(split + 1))};
        if (key == "access_token") {
            access_token = value;
        }
        else if (key == "refresh_token") {
            refresh_token = value;
        }
        else if (key == "authorized_at" || key == "refresh_expires_at") {
            if (value.empty()) {
                malformed = true;
                break;
            }
            std::time_t parsed = 0;
            const auto result = std::from_chars(
                value.data(),
                value.data() + value.size(),
                parsed
            );
            if (result.ec != std::errc {} ||
                result.ptr != value.data() + value.size()) {
                malformed = true;
                break;
            }
            if (key == "authorized_at") {
                authorized_at = parsed;
            }
            else {
                refresh_expires_at = parsed;
            }
        }
        if (end == std::string_view::npos) {
            break;
        }
    }

    if (malformed ||
        access_token.empty() ||
        refresh_token.empty() ||
        !authorized_at ||
        !refresh_expires_at) {
        return std::nullopt;
    }
    return TokenState {
        std::move(access_token),
        std::move(refresh_token),
        *authorized_at,
        *refresh_expires_at
    };
}

[[nodiscard]] inline std::string serialize(const TokenState& state) {
    std::ostringstream output;
    output
        << "access_token = " << state.access_token << '\n'
        << "refresh_token = " << state.refresh_token << '\n'
        << "authorized_at = " << state.authorized_at << '\n'
        << "refresh_expires_at = " << state.refresh_expires_at << '\n';
    return output.str();
}

[[nodiscard]] inline std::optional<TokenState> load() {
    std::ifstream input(path(), std::ios::binary);
    if (!input) {
        return std::nullopt;
    }
    std::ostringstream contents;
    contents << input.rdbuf();
    return parse(contents.str());
}

[[nodiscard]] inline bool replace_file(std::string_view contents) {
    const auto destination = path();
    std::error_code error;
    std::filesystem::create_directories(destination.parent_path(), error);
    if (error) {
        return false;
    }
    const auto temporary = destination.native() + L".new";
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) {
            return false;
        }
        output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        output.flush();
        output.close();
        if (!output) {
            std::filesystem::remove(temporary, error);
            return false;
        }
    }
    if (!MoveFileExW(
            temporary.c_str(),
            destination.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
        )) {
        std::filesystem::remove(temporary, error);
        return false;
    }
    return true;
}

/**
 * \brief Holds `Local\AutoCoreSpotifyTokens` for one critical section.
 *
 * Runtime refresh rereads `tokens.map` through `load` while this lock is held,
 * then calls `store` only if a refresh is still required.
 */
class TokenSession {
public:
    TokenSession() {
        handle_ = CreateMutexW(nullptr, FALSE, mutex_name.data());
        if (handle_ == nullptr) {
            return;
        }
        const DWORD wait = WaitForSingleObject(handle_, INFINITE);
        held_ = wait == WAIT_OBJECT_0 || wait == WAIT_ABANDONED;
    }

    TokenSession(const TokenSession&) = delete;
    TokenSession& operator=(const TokenSession&) = delete;

    ~TokenSession() {
        if (held_) {
            ReleaseMutex(handle_);
        }
        if (handle_ != nullptr) {
            CloseHandle(handle_);
        }
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return held_;
    }

    [[nodiscard]] bool store(const TokenState& state) const {
        if (!held_) {
            return false;
        }
        return replace_file(serialize(state));
    }

private:
    HANDLE handle_ = nullptr;
    bool held_ = false;
};

} // namespace spotify::tokens
