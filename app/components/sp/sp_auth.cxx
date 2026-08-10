/**
\file sp_auth.cxx
\brief Implements Spotify credentials and token management.
*/
module sp_c;

import std;
import thread;
import sp_x;

import <json.hpp>;
import <cpr/cpr.h>;
import <chrono>;

using std::stoll;
using namespace cpr;

json parse(const std::string& s);

/**
 * \brief Encodes a std::string in Base64 format.
 *
 * \param input The std::string to encode.
 * \return The Base64-encoded std::string.
 */
std::string base64_encode(const std::string& input) {
    constexpr std::string_view base64_characters =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string output;
    int value = 0;
    int bit_count = -6;

    for (const std::uint8_t character : input) {
        value = (value << 8) + character;
        bit_count += 8;

        while (bit_count >= 0) {
            output.push_back(
                base64_characters[(value >> bit_count) & 0x3F]
            );

            bit_count -= 6;
        }
    }

    if (bit_count > -6) {
        output.push_back(
            base64_characters[
                ((value << 8) >> (bit_count + 8)) & 0x3F
            ]
        );
    }

    while (output.size() % 4 != 0) {
        output.push_back('=');
    }

    return output;
}


/**
 * \brief Retrieves the current Unix timestamp (seconds since epoch).
 * \return The current Unix timestamp as a time_t.
 */
time_t get_unix_timestamp() {
    return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}
/**
 * \brief Retrieves Spotify credentials.
 */
void Spotify::get_credentials() {
    std::ifstream rc(codes_path);
    std::getline(rc, client_id);
    std::getline(rc, client_secret);
    std::string credentials = client_id + ":" + client_secret;
    credentials_64 = base64_encode(credentials);
    rc.close();
}
/**
 * \brief Extracts Spotify tokens.
 */
void Spotify::extract_tokens() {
    get_credentials();
    get_devices();

    std::ifstream rc(tokens_path);
    std::getline(rc, access_token);
    std::getline(rc, refresh_token);

    std::string time_str;

    std::getline(rc, time_str);
    start_timestamp = std::stoll(time_str);

    std::getline(rc, time_str);
    refresh_token_expiration = std::stoll(time_str);

    rc.close();

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

        sp_component.logg_and_print(
            "Spotify refresh-token expiration is missing or invalid."
        );

        return;
    }

    const time_t current_time = get_unix_timestamp();

    if (current_time >= refresh_token_expiration) {
        reauthorization_required = true;

        sp_component.logg_and_print(
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

        sp_component.logg_and_print(
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
        sp_component.logg("Warning: System time appears to have gone backward. Forcing token refresh.");
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

    check_refresh_token_expiration();

    if (reauthorization_required) {
        sp_component.logg_and_print(
            "Spotify reauthorization required - run sp_oauth.exe to clear"
        );
        return false;
    }
    if (!check_timerate()) {
        authorization_header = "Bearer " + access_token;
        return true;
    }
    Response response = Post(Url {"https://accounts.spotify.com/api/token"},
        Header {{"Content-Type", "application/x-www-form-urlencoded"},
                    {"Authorization", "Basic " + credentials_64}},
        Payload {{"grant_type", "refresh_token"},
                     {"refresh_token", refresh_token}});

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
            std::ofstream rc(tokens_path);
            if (rc.is_open()) {
                std::ostringstream os;
                os << access_token << '\n'
                    << refresh_token << '\n'
                    << start_timestamp << '\n'
                    << refresh_token_expiration;
                rc << os.str();
                rc.close();
            }
        }
        authorization_header = "Bearer " + access_token;
        sp_component.logg_and_logg("refresh_tokens() - tokens refreshed");
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

                std::ofstream rc(tokens_path);

                if (rc.is_open()) {
                    rc << access_token << '\n'
                        << refresh_token << '\n'
                        << start_timestamp << '\n'
                        << refresh_token_expiration;
                }

                sp_component.logg_and_print(
                    "Spotify refresh token is no longer valid. "
                    "Spotify reauthorization required - run sp_oauth.exe to clear"
                );

                return false;
            }
        }
        catch (const json::exception& e) {
            sp_component.logg_and_print(
                "Failed to parse Spotify token error response: {}",
                e.what()
            );
        }

        sp_component.logg_and_print(
            "Spotify token request failed: Status Code {} - {}",
            response.status_code,
            response.text
        );

        return false;
    }
    else if (response.status_code == 429) {
        sp_component.logg_and_print("Error: Rate Limit Reached\nStatus Code {} - {}", response.status_code, response.text);
        return false;
    }
    else {
        sp_component.logg_and_print("Error: \nStatus Code {} - {}", response.status_code, response.text);
        return false;
    }
}

