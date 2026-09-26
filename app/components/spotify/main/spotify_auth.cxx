/**
\file spotify_auth.cxx
\brief Implements Spotify credentials and token management.
*/
module spotify_client;

import std;
import auto_core.core.ini;
import auto_core.core.thread;
import spotify_component;

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

std::string format_spotify_config_ini(
    const std::string& client_id,
    const std::vector<std::pair<std::string, std::string>>& devices
) {
    std::ostringstream output;
    output << "[auth]\nclient_id = " << client_id << '\n';

    if (!devices.empty()) {
        output << "\n[devices]\n";
        for (const auto& [name, id] : devices) {
            output << name << " = " << id << '\n';
        }
    }

    return output.str();
}

std::string format_spotify_tokens_ini(
    const std::string& access_token,
    const std::string& refresh_token,
    time_t authorized_at,
    time_t refresh_expires_at
) {
    std::ostringstream output;
    output
        << "; Machine-local. Do not copy this file.\n"
        << "[tokens]\n"
        << "access_token = " << access_token << '\n'
        << "refresh_token = " << refresh_token << '\n'
        << "authorized_at = " << authorized_at << '\n'
        << "refresh_expires_at = " << refresh_expires_at << '\n';
    return output.str();
}

void Spotify::load_config() {
    client_id.clear();
    devices.clear();

    const auto document = ac::ini::read(config_path);
    if (!document) {
        return;
    }

    if (const auto id = document->find("auth", "client_id")) {
        client_id = std::string {*id};
    }

    devices = document->settings("devices");
}

void Spotify::save_config() {
    std::ofstream output(config_path);
    if (!output.is_open()) {
        return;
    }

    output << format_spotify_config_ini(client_id, devices);
}

void Spotify::save_tokens() {
    std::ofstream output(tokens_path);
    if (!output.is_open()) {
        return;
    }

    output << format_spotify_tokens_ini(
        access_token,
        refresh_token,
        start_timestamp,
        refresh_token_expiration
    );
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
    load_config();

    const auto document = ac::ini::read(tokens_path);
    if (!document) {
        tokens_extracted = false;
        return;
    }

    const auto loaded_access = document->find("tokens", "access_token");
    const auto loaded_refresh = document->find("tokens", "refresh_token");
    const auto authorized_at = document->find("tokens", "authorized_at");
    const auto refresh_expires_at =
        document->find("tokens", "refresh_expires_at");

    if (
        !loaded_access ||
        !loaded_refresh ||
        !authorized_at ||
        !refresh_expires_at ||
        loaded_access->empty() ||
        loaded_refresh->empty()
    ) {
        tokens_extracted = false;
        return;
    }

    access_token = std::string {*loaded_access};
    refresh_token = std::string {*loaded_refresh};

    try {
        start_timestamp = stoll(std::string {*authorized_at});
        refresh_token_expiration = stoll(std::string {*refresh_expires_at});
    }
    catch (const std::exception&) {
        tokens_extracted = false;
        return;
    }

    tokens_extracted = true;
    reauthorization_required = false;
    reauthorization_warning_logged = false;
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
/**
 * \brief Refreshes Spotify tokens.
 * \return True if tokens were refreshed, false otherwise.
 */
bool Spotify::refresh_tokens() {
    std::lock_guard<std::mutex> lock(refresh_token_mutex);

    if (
        !tokens_extracted ||
        reauthorization_required
    ) {
        extract_tokens();
    }

    if (!tokens_extracted) {
        spotify_component.log_print(
            "Spotify reauthorization required - run spotify_oauth.exe to clear"
        );
        return false;
    }

    check_refresh_token_expiration();

    if (reauthorization_required) {
        spotify_component.log_print(
            "Spotify reauthorization required - run spotify_oauth.exe to clear"
        );
        return false;
    }
    if (!check_timerate()) {
        authorization_header = "Bearer " + access_token;
        return true;
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
            save_tokens();
        }
        authorization_header = "Bearer " + access_token;
        spotify_component.log_main("refresh_tokens() - tokens refreshed");
        return true;
    }
    else if (response.status_code == 400) {
        try {
            const json response_json = parse(response.text);

            if (
                response_json.contains("error") &&
                response_json["error"] == "invalid_grant"
                ) {
                reauthorization_required = true;
                refresh_token_expiration = 0;
                save_tokens();

                spotify_component.log_print(
                    "Spotify refresh token is no longer valid. "
                    "Spotify reauthorization required - run spotify_oauth.exe to clear"
                );

                return false;
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

        return false;
    }
    else if (response.status_code == 429) {
        spotify_component.log_print("Error: Rate Limit Reached\nStatus Code {} - {}", response.status_code, response.text);
        return false;
    }
    else {
        spotify_component.log_print("Error: \nStatus Code {} - {}", response.status_code, response.text);
        return false;
    }
}

