/**
\file spotify_auth.cxx
\brief Implements Spotify credentials and token management.
*/
module spotify_client;

import std;
import auto_core.core.paths;
import auto_core.core.thread;
import spotify_component;
import spotify_defaults;
import spotify_application_data;
import spotify_token_store;

import <Windows.h>;
import <json.hpp>;
import <cpr/cpr.h>;
import <chrono>;

using std::stoll;
using namespace cpr;

json parse(const std::string& s);

/**
 * \brief Retrieves the current Unix timestamp (seconds since epoch).
 * \return The current Unix timestamp as a time_t.
 */
time_t get_unix_timestamp() {
    return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

void Spotify::load_client_id() {
    if (const auto id = spotify::data::load_client_id()) {
        client_id = *id;
    }
    else {
        client_id.clear();
    }
}

void Spotify::load_devices_from_disk() {
    devices.clear();
    for (const spotify::data::Device& device : spotify::data::load_devices()) {
        if (!device.id.empty()) {
            devices.emplace_back(device.name, device.id);
        }
    }
}

void Spotify::apply_token_state(const spotify::tokens::TokenState& state) {
    access_token = state.access_token;
    refresh_token = state.refresh_token;
    start_timestamp = state.authorized_at;
    refresh_token_expiration = state.refresh_expires_at;
    tokens_extracted = true;
    reauthorization_required = false;
    reauthorization_warning_logged = false;
    authorization_header = "Bearer " + access_token;
}

void Spotify::load_tokens_from_disk() {
    if (const auto state = spotify::tokens::load()) {
        apply_token_state(*state);
        return;
    }
    tokens_extracted = false;
    access_token.clear();
    refresh_token.clear();
}

std::string Spotify::get_device_code(std::string_view name) const {
    for (const auto& [device_name, device_id] : devices) {
        if (device_name == name) {
            return device_id;
        }
    }

    return {};
}

std::string ascii_lower(std::string text) {
    for (char& character : text) {
        character = static_cast<char>(
            std::tolower(static_cast<unsigned char>(character))
        );
    }

    return text;
}

bool device_name_equals_ignore_case(
    std::string_view left,
    std::string_view right
) {
    if (left.size() != right.size()) {
        return false;
    }

    for (std::size_t i = 0; i < left.size(); ++i) {
        if (
            std::tolower(static_cast<unsigned char>(left[i])) !=
            std::tolower(static_cast<unsigned char>(right[i]))
        ) {
            return false;
        }
    }

    return true;
}

std::string Spotify::first_configured_device_id() const {
    for (const char* preferred : {"desktop", "laptop"}) {
        for (const auto& [device_name, device_id] : devices) {
            if (
                !device_id.empty() &&
                device_name_equals_ignore_case(device_name, preferred)
            ) {
                return device_id;
            }
        }
    }

    for (const auto& [device_name, device_id] : devices) {
        if (!device_id.empty()) {
            return device_id;
        }
    }

    return {};
}

/**
 * \brief Extracts Spotify tokens.
 */
void Spotify::extract_tokens() {
    load_tokens_from_disk();
}

void Spotify::check_refresh_token_expiration() {
    if (reauthorization_required) {
        return;
    }

    constexpr time_t reauthorization_buffer =
        7 * 24 * 60 * 60;

    if (refresh_token_expiration <= 0) {
        reauthorization_required = true;

        spotify_component.log_print(
            "Spotify refresh-token expiration is missing or invalid."
        );

        return;
    }

    const time_t current_time = get_unix_timestamp();

    if (current_time >= refresh_token_expiration) {
        reauthorization_required = true;

        spotify_component.log_print(
            "Spotify refresh token has expired."
        );

        return;
    }

    const time_t reauthorization_warning_time =
        refresh_token_expiration - reauthorization_buffer;

    if (
        current_time >= reauthorization_warning_time &&
        !reauthorization_warning_logged
        ) {
        reauthorization_warning_logged = true;

        constexpr time_t seconds_per_day =
            24 * 60 * 60;

        const time_t seconds_remaining =
            refresh_token_expiration - current_time;

        const time_t days_remaining =
            (seconds_remaining + seconds_per_day - 1) /
            seconds_per_day;

        spotify_component.log_print(
            "Spotify refresh token will expire in "
            + std::to_string(days_remaining)
            + (days_remaining == 1 ? " day." : " days.")
        );
    }
}

/**
 * \brief Checks if the timerate has been reached.
 * \return True if timerate has been reached, false otherwise.
 */
bool Spotify::check_timerate() {
    time_t current_time = get_unix_timestamp();

    // Handle case where system time went backward
    if (current_time < start_timestamp) {
        spotify_component.log("Warning: System time appears to have gone backward. Forcing token refresh.");
        return true;
    }

    time_t elapsed_seconds = current_time - start_timestamp;
    return elapsed_seconds >= static_cast<time_t>(timerate) * 60;
}
namespace {

std::wstring quote_argument(std::wstring_view value) {
    std::wstring quoted;
    quoted.reserve(value.size() + 2);
    quoted.push_back(L'"');
    quoted.append(value);
    quoted.push_back(L'"');
    return quoted;
}

} // namespace

void Spotify::launch_spotify_oauth() {
    const auto executable_path =
        ac::paths::bin_directory() / "spotify_oauth.exe";
    std::error_code error;
    const bool present = std::filesystem::exists(executable_path, error);
    if (error || !present) {
        spotify_component.log_print(
            "Missing {}.",
            executable_path.string()
        );
        return;
    }

    std::wstring command_line = quote_argument(executable_path.wstring());
    STARTUPINFOW startup_info {};
    startup_info.cb = sizeof(startup_info);
    PROCESS_INFORMATION process_info {};
    if (!::CreateProcessW(
            executable_path.c_str(),
            command_line.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NEW_CONSOLE,
            nullptr,
            executable_path.parent_path().c_str(),
            &startup_info,
            &process_info
        )) {
        spotify_component.log_print(
            "Unable to start {}.",
            executable_path.string()
        );
        return;
    }

    ::CloseHandle(process_info.hThread);
    ::WaitForSingleObject(process_info.hProcess, INFINITE);
    DWORD exit_code = 1;
    if (!::GetExitCodeProcess(process_info.hProcess, &exit_code)) {
        ::CloseHandle(process_info.hProcess);
        spotify_component.log_print(
            "Unable to read the exit code from spotify_oauth.exe."
        );
        return;
    }
    ::CloseHandle(process_info.hProcess);
    const int code = static_cast<int>(exit_code);
    if (code != 0) {
        spotify_component.log_print("spotify_oauth.exe exited {}.", code);
    }
}

void Spotify::report_authorization_required() {
    if (authorization_required_logged) {
        return;
    }
    authorization_required_logged = true;
    spotify_component.log_print("Spotify authorization required.");
}

/**
 * \brief Runs one token-refresh decision without launching OAuth.
 */
Spotify::TokenRefreshAttempt Spotify::attempt_token_refresh() {
    if (!tokens_extracted) {
        report_authorization_required();
        return TokenRefreshAttempt::interactive;
    }

    check_refresh_token_expiration();

    if (reauthorization_required) {
        report_authorization_required();
        return TokenRefreshAttempt::interactive;
    }
    if (!check_timerate()) {
        authorization_header = "Bearer " + access_token;
        return TokenRefreshAttempt::ready;
    }

    spotify::tokens::TokenSession file_lock;
    if (!file_lock) {
        spotify_component.log_print("Unable to coordinate Spotify token state.");
        return TokenRefreshAttempt::transient_failure;
    }

    load_tokens_from_disk();
    if (!tokens_extracted) {
        report_authorization_required();
        return TokenRefreshAttempt::interactive;
    }
    reauthorization_required = false;
    check_refresh_token_expiration();
    if (reauthorization_required) {
        report_authorization_required();
        return TokenRefreshAttempt::interactive;
    }
    if (!check_timerate()) {
        authorization_header = "Bearer " + access_token;
        return TokenRefreshAttempt::ready;
    }

    load_client_id();
    if (client_id.empty()) {
        report_authorization_required();
        return TokenRefreshAttempt::interactive;
    }

    Response response = Post(Url {"https://accounts.spotify.com/api/token"},
        Header {{"Content-Type", "application/x-www-form-urlencoded"}},
        Payload {{"grant_type", "refresh_token"},
                     {"refresh_token", refresh_token},
                     {"client_id", client_id}});

    if (response.status_code == 200) {
        auto response_json = parse(response.text);
        if (response_json.contains("access_token")) {
            access_token = response_json["access_token"];
            if (
                response_json.contains("refresh_token") &&
                response_json["refresh_token"].is_string() &&
                !response_json["refresh_token"].get_ref<const std::string&>().empty()
                ) {
                refresh_token = response_json["refresh_token"];
            }
            start_timestamp = get_unix_timestamp();
            spotify::tokens::TokenState state;
            state.access_token = access_token;
            state.refresh_token = refresh_token;
            state.authorized_at = start_timestamp;
            state.refresh_expires_at = refresh_token_expiration;
            if (!file_lock.store(state)) {
                spotify_component.log_print("Unable to write tokens.map.");
                return TokenRefreshAttempt::transient_failure;
            }
        }
        authorization_header = "Bearer " + access_token;
        authorization_required_logged = false;
        spotify_component.log_main("refresh_tokens() - tokens refreshed");
        return TokenRefreshAttempt::ready;
    }
    else if (response.status_code == 400) {
        try {
            const json response_json = parse(response.text);

            if (
                response_json.contains("error") &&
                response_json["error"] == "invalid_grant"
                ) {
                reauthorization_required = true;
                report_authorization_required();
                return TokenRefreshAttempt::interactive;
            }
        }
        catch (const json::exception& e) {
            spotify_component.log_print(
                "Failed to parse Spotify token error response: {}",
                e.what()
            );
        }

        spotify_component.log_print(
            "Spotify token request failed: Status Code {} - {}",
            response.status_code,
            response.text
        );

        return TokenRefreshAttempt::transient_failure;
    }
    else if (response.status_code == 429) {
        spotify_component.log_print("Error: Rate Limit Reached\nStatus Code {} - {}", response.status_code, response.text);
        return TokenRefreshAttempt::transient_failure;
    }
    else {
        spotify_component.log_print("Error: \nStatus Code {} - {}", response.status_code, response.text);
        return TokenRefreshAttempt::transient_failure;
    }
}

/**
 * \brief Refreshes Spotify tokens.
 * \return True if tokens were refreshed, false otherwise.
 */
bool Spotify::refresh_tokens() {
    std::lock_guard<std::mutex> lock(refresh_token_mutex);

    for (int pass = 0; pass < 2; ++pass) {
        if (!tokens_extracted && !token_cache_reread) {
            token_cache_reread = true;
            load_tokens_from_disk();
        }
        const auto attempt = attempt_token_refresh();
        if (attempt == TokenRefreshAttempt::ready) {
            oauth_auto_launch_attempted = false;
            authorization_required_logged = false;
            return true;
        }
        if (attempt == TokenRefreshAttempt::transient_failure) {
            return false;
        }
        if (spotify::defaults::should_auto_launch_oauth(
                spotify::defaults::AuthorizationCondition::interactive_required,
                auto_launch_oauth,
                oauth_auto_launch_attempted
            )) {
            oauth_auto_launch_attempted = true;
            launch_spotify_oauth();
            load_tokens_from_disk();
            token_cache_reread = true;
            if (tokens_extracted) {
                authorization_required_logged = false;
            }
            continue;
        }
        return false;
    }
    return false;
}

