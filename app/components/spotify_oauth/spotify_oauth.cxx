module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <bcrypt.h>
#include <shellapi.h>

module spotify_oauth;

import std;
import auto_core.core.ini;

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "bcrypt.lib")

import <cpr/cpr.h>;
import <json.hpp>;
import <CivetServer.h>;


namespace {

    constexpr std::string_view account_url {
        "https://accounts.spotify.com"
    };

    constexpr std::string_view token_endpoint {
        "https://accounts.spotify.com/api/token"
    };

    constexpr std::string_view callback_path {
        "/callback"
    };

    constexpr std::string_view pkce_unreserved {
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789-._~"
    };

    constexpr std::size_t code_verifier_length = 64;
    constexpr std::size_t state_length = 32;

    const std::array spotify_scopes {
        std::string_view {"user-follow-read"},
        std::string_view {"ugc-image-upload"},
        std::string_view {"user-read-playback-state"},
        std::string_view {"user-modify-playback-state"},
        std::string_view {"user-read-currently-playing"},
        std::string_view {"user-read-private"},
        std::string_view {"user-read-email"},
        std::string_view {"user-follow-modify"},
        std::string_view {"user-library-modify"},
        std::string_view {"user-library-read"},
        std::string_view {"streaming"},
        std::string_view {"app-remote-control"},
        std::string_view {"user-read-playback-position"},
        std::string_view {"user-top-read"},
        std::string_view {"user-read-recently-played"},
        std::string_view {"playlist-modify-private"},
        std::string_view {"playlist-read-collaborative"},
        std::string_view {"playlist-read-private"},
        std::string_view {"playlist-modify-public"}
    };

    struct SpotifyAuthorizationSession {
        std::string redirect_uri;
        std::string code_verifier;
        std::string state;
        std::string access_token;
        std::string refresh_token;

        SpotifyAuthorizationStatus status {
            SpotifyAuthorizationStatus::invalid_callback
        };

        std::string message;

        std::atomic<bool> complete {false};
    };

    std::string join_scopes() {
        std::string result;

        for (const std::string_view scope : spotify_scopes) {
            if (!result.empty()) {
                result += ' ';
            }

            result += scope;
        }

        return result;
    }

    std::string base64url_encode(std::string_view input) {
        constexpr std::string_view characters {
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789-_"
        };

        std::string output;

        int value = 0;
        int bit_position = -6;

        for (const unsigned char character : input) {
            value = (value << 8) + character;
            bit_position += 8;

            while (bit_position >= 0) {
                output.push_back(
                    characters[
                        (value >> bit_position) & 0x3F
                    ]
                );

                bit_position -= 6;
            }
        }

        if (bit_position > -6) {
            output.push_back(
                characters[
                    ((value << 8) >> (bit_position + 8)) & 0x3F
                ]
            );
        }

        return output;
    }

    bool fill_random_bytes(unsigned char* bytes, std::size_t size) {
        const NTSTATUS status = BCryptGenRandom(
            nullptr,
            bytes,
            static_cast<ULONG>(size),
            BCRYPT_USE_SYSTEM_PREFERRED_RNG
        );

        return BCRYPT_SUCCESS(status);
    }

    bool generate_unreserved_string(
        std::size_t length,
        std::string& output
    ) {
        output.clear();
        output.reserve(length);

        constexpr std::size_t charset_size = pkce_unreserved.size();
        constexpr unsigned char reject_threshold =
            static_cast<unsigned char>(
                (256 / charset_size) * charset_size
            );

        std::array<unsigned char, 128> buffer {};

        while (output.size() < length) {
            if (!fill_random_bytes(buffer.data(), buffer.size())) {
                output.clear();
                return false;
            }

            for (const unsigned char byte : buffer) {
                if (byte >= reject_threshold) {
                    continue;
                }

                output.push_back(
                    pkce_unreserved[byte % charset_size]
                );

                if (output.size() == length) {
                    break;
                }
            }
        }

        return true;
    }

    bool sha256_digest(
        std::string_view input,
        std::array<unsigned char, 32>& digest
    ) {
        BCRYPT_ALG_HANDLE algorithm = nullptr;

        NTSTATUS status = BCryptOpenAlgorithmProvider(
            &algorithm,
            BCRYPT_SHA256_ALGORITHM,
            nullptr,
            0
        );

        if (!BCRYPT_SUCCESS(status)) {
            return false;
        }

        std::vector<unsigned char> buffer(
            input.begin(),
            input.end()
        );

        status = BCryptHash(
            algorithm,
            nullptr,
            0,
            buffer.data(),
            static_cast<ULONG>(buffer.size()),
            digest.data(),
            static_cast<ULONG>(digest.size())
        );

        BCryptCloseAlgorithmProvider(algorithm, 0);

        return BCRYPT_SUCCESS(status);
    }

    bool create_code_challenge(
        std::string_view code_verifier,
        std::string& code_challenge
    ) {
        std::array<unsigned char, 32> digest {};

        if (!sha256_digest(code_verifier, digest)) {
            return false;
        }

        code_challenge = base64url_encode(
            std::string_view {
                reinterpret_cast<const char*>(digest.data()),
                digest.size()
            }
        );

        return true;
    }

    bool create_pkce_session(
        SpotifyAuthorizationSession& session,
        std::string& code_challenge
    ) {
        if (!generate_unreserved_string(
            code_verifier_length,
            session.code_verifier
        )) {
            return false;
        }

        if (!generate_unreserved_string(
            state_length,
            session.state
        )) {
            return false;
        }

        return create_code_challenge(
            session.code_verifier,
            code_challenge
        );
    }

    std::optional<std::string> get_query_parameter(
        std::string_view query,
        std::string_view parameter
    ) {
        const std::string key =
            std::string(parameter) + '=';

        const std::size_t key_position =
            query.find(key);

        if (key_position == std::string_view::npos) {
            return std::nullopt;
        }

        const std::size_t value_start =
            key_position + key.size();

        const std::size_t value_end =
            query.find('&', value_start);

        return std::string {
            query.substr(
                value_start,
                value_end == std::string_view::npos
                    ? std::string_view::npos
                    : value_end - value_start
            )
        };
    }

    std::string build_redirect_uri(
        const SpotifyOAuthConfig& config
    ) {
        return
            "http://127.0.0.1:"
            + config.port_number
            + std::string(callback_path);
    }

    std::string build_authorization_link(
        const SpotifyOAuthConfig& config,
        std::string_view redirect_uri,
        std::string_view code_challenge,
        std::string_view state
    ) {
        const cpr::Parameters parameters {
            {"client_id", config.client_id},
            {"response_type", "code"},
            {"redirect_uri", std::string(redirect_uri)},
            {"scope", join_scopes()},
            {"code_challenge_method", "S256"},
            {"code_challenge", std::string(code_challenge)},
            {"state", std::string(state)}
        };

        const cpr::CurlHolder curl_holder;

        return std::string(account_url)
            + "/authorize?"
            + parameters.GetContent(curl_holder);
    }

    bool open_authorization_link(
        const std::string& authorization_link
    ) {
        const std::wstring wide_link {
            authorization_link.begin(),
            authorization_link.end()
        };

        const HINSTANCE result = ShellExecuteW(
            nullptr,
            L"open",
            wide_link.c_str(),
            nullptr,
            nullptr,
            SW_SHOWNORMAL
        );

        return reinterpret_cast<std::intptr_t>(result) > 32;
    }

    bool ensure_data_directory(
        const SpotifyOAuthConfig& config,
        std::string& error_message
    ) {
        const std::filesystem::path token_directory =
            config.token_path.parent_path();

        if (token_directory.empty()) {
            return true;
        }

        std::error_code error;

        std::filesystem::create_directories(
            token_directory,
            error
        );

        if (error) {
            error_message =
                "Unable to create token directory: " +
                error.message();

            return false;
        }

        return true;
    }

    bool write_text_file(
        const std::filesystem::path& path,
        std::string_view contents,
        std::string& error_message
    ) {
        std::ofstream output_file(path);

        if (!output_file.is_open()) {
            error_message =
                "Unable to open file: " + path.string();

            return false;
        }

        output_file << contents;

        if (!output_file) {
            error_message =
                "An error occurred while writing: " +
                path.string();

            return false;
        }

        return true;
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

    void upsert_device(
        std::vector<std::pair<std::string, std::string>>& devices,
        std::string name,
        std::string id
    ) {
        name = ascii_lower(std::move(name));

        for (auto& [device_name, device_id] : devices) {
            if (device_name_equals_ignore_case(device_name, name)) {
                device_name = std::move(name);
                device_id = std::move(id);
                return;
            }
        }

        devices.emplace_back(std::move(name), std::move(id));
    }

    bool write_sp_config(
        const SpotifyOAuthConfig& config,
        const std::vector<std::pair<std::string, std::string>>& devices,
        std::string& error_message
    ) {
        std::ostringstream config_contents;
        config_contents
            << "[auth]\nclient_id = " << config.client_id << '\n';

        if (!devices.empty()) {
            config_contents << "\n[devices]\n";
            for (const auto& [name, id] : devices) {
                config_contents << name << " = " << id << '\n';
            }
        }

        return write_text_file(
            config.config_path,
            config_contents.str(),
            error_message
        );
    }

    std::vector<std::pair<std::string, std::string>> load_devices(
        const std::filesystem::path& config_path
    ) {
        if (const auto document = ac::ini::read(config_path)) {
            return document->settings("devices");
        }

        return {};
    }

    bool save_authorization_files(
        const SpotifyOAuthConfig& config,
        const SpotifyAuthorizationSession& session,
        std::string& error_message
    ) {
        if (!ensure_data_directory(config, error_message)) {
            return false;
        }

        const auto authorization_time =
            std::chrono::system_clock::now();

        const std::time_t authorization_timestamp =
            std::chrono::system_clock::to_time_t(
                authorization_time
            );

        const std::time_t refresh_token_expiration_timestamp =
            std::chrono::system_clock::to_time_t(
                authorization_time + std::chrono::days(179)
            );

        std::ostringstream token_contents;
        token_contents
            << "; Machine-local. Do not copy this file.\n"
            << "[tokens]\n"
            << "access_token = " << session.access_token << '\n'
            << "refresh_token = " << session.refresh_token << '\n'
            << "authorized_at = " << authorization_timestamp << '\n'
            << "refresh_expires_at = "
            << refresh_token_expiration_timestamp << '\n';

        if (!write_text_file(
            config.token_path,
            token_contents.str(),
            error_message
        )) {
            return false;
        }

        std::vector<std::pair<std::string, std::string>> devices =
            load_devices(config.config_path);

        return write_sp_config(config, devices, error_message);
    }

    bool exchange_authorization_code(
        const SpotifyOAuthConfig& config,
        SpotifyAuthorizationSession& session,
        std::string_view authorization_code,
        std::string& error_message
    ) {
        const cpr::Response response = cpr::Post(
            cpr::Url {
                std::string(token_endpoint)
            },
            cpr::Header {
                {
                    "Content-Type",
                    "application/x-www-form-urlencoded"
                }
            },
            cpr::Payload {
                {
                    "grant_type",
                    "authorization_code"
                },
                {
                    "code",
                    std::string(authorization_code)
                },
                {
                    "redirect_uri",
                    session.redirect_uri
                },
                {
                    "client_id",
                    config.client_id
                },
                {
                    "code_verifier",
                    session.code_verifier
                }
            }
        );

        if (response.status_code != 200) {
            error_message =
                "Spotify token request failed with status code "
                + std::to_string(response.status_code)
                + ". Response: "
                + response.text;

            return false;
        }

        try {
            const nlohmann::json response_json =
                nlohmann::json::parse(response.text);

            if (!response_json.contains("access_token")
                || !response_json["access_token"].is_string()) {
                error_message =
                    "Spotify did not return a valid access token.";

                return false;
            }

            if (!response_json.contains("refresh_token")
                || !response_json["refresh_token"].is_string()) {
                error_message =
                    "Spotify did not return a valid refresh token.";

                return false;
            }

            session.access_token =
                response_json["access_token"].get<std::string>();

            session.refresh_token =
                response_json["refresh_token"].get<std::string>();
        }
        catch (const nlohmann::json::exception& exception) {
            error_message =
                "Unable to parse Spotify token response: "
                + std::string(exception.what());

            return false;
        }

        return true;
    }

    void send_browser_response(
        mg_connection* connection,
        int status_code,
        std::string_view status_text,
        std::string_view message
    ) {
        mg_printf(
            connection,
            "HTTP/1.1 %d %.*s\r\n"
            "Content-Type: text/plain; charset=utf-8\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%.*s",
            status_code,
            static_cast<int>(status_text.size()),
            status_text.data(),
            static_cast<int>(message.size()),
            message.data()
        );
    }

    class SpotifyCallbackHandler final : public CivetHandler {
    public:
        SpotifyCallbackHandler(
            const SpotifyOAuthConfig& config,
            SpotifyAuthorizationSession& session
        )
            : config_(config),
            session_(session) {
        }

        bool handleGet(
            CivetServer*,
            mg_connection* connection
        ) override {
            const mg_request_info* request_info =
                mg_get_request_info(connection);

            const std::string_view query =
                request_info->query_string
                ? request_info->query_string
                : "";

            if (const auto error =
                get_query_parameter(query, "error")) {
                session_.status =
                    SpotifyAuthorizationStatus::authorization_denied;

                session_.message =
                    "Spotify authorization was denied: " + *error;

                send_browser_response(
                    connection,
                    200,
                    "OK",
                    "Spotify authorization was not completed."
                );

                finish();
                return true;
            }

            const auto returned_state =
                get_query_parameter(query, "state");

            if (
                !returned_state ||
                *returned_state != session_.state
            ) {
                session_.status =
                    SpotifyAuthorizationStatus::invalid_callback;

                session_.message =
                    "The Spotify callback state did not match.";

                send_browser_response(
                    connection,
                    400,
                    "Bad Request",
                    "The authorization callback was invalid."
                );

                finish();
                return true;
            }

            const auto authorization_code =
                get_query_parameter(query, "code");

            if (!authorization_code) {
                session_.status =
                    SpotifyAuthorizationStatus::invalid_callback;

                session_.message =
                    "The Spotify callback did not contain "
                    "an authorization code.";

                send_browser_response(
                    connection,
                    400,
                    "Bad Request",
                    "The authorization callback was invalid."
                );

                finish();
                return true;
            }

            std::cout << "Authorization code received.\n";

            std::string error_message;

            if (!exchange_authorization_code(
                config_,
                session_,
                *authorization_code,
                error_message
            )) {
                session_.status =
                    SpotifyAuthorizationStatus::token_exchange_failed;

                session_.message =
                    std::move(error_message);

                send_browser_response(
                    connection,
                    502,
                    "Bad Gateway",
                    "Spotify authorization failed while "
                    "retrieving tokens."
                );

                finish();
                return true;
            }

            if (!save_authorization_files(
                config_,
                session_,
                error_message
            )) {
                session_.status =
                    SpotifyAuthorizationStatus::token_save_failed;

                session_.message =
                    std::move(error_message);

                send_browser_response(
                    connection,
                    500,
                    "Internal Server Error",
                    "Authorization succeeded, but the tokens "
                    "could not be saved."
                );

                finish();
                return true;
            }

            session_.status =
                SpotifyAuthorizationStatus::success;

            session_.message =
                "Spotify authorization completed successfully.";

            send_browser_response(
                connection,
                200,
                "OK",
                "Authorization successful. "
                "You can close this page."
            );

            finish();
            return true;
        }

    private:
        const SpotifyOAuthConfig& config_;
        SpotifyAuthorizationSession& session_;

        void finish() {
            session_.complete.store(
                true,
                std::memory_order_release
            );
        }
    };

} // namespace

bool SpotifyOAuthConfig::is_valid() const noexcept {
    return !client_id.empty()
        && !port_number.empty();
}

bool SpotifyAuthorizationResult::succeeded() const noexcept {
    return status == SpotifyAuthorizationStatus::success;
}

SpotifyOAuth::SpotifyOAuth(
    SpotifyOAuthConfig config
)
    : config_(std::move(config)) {
}

SpotifyAuthorizationResult SpotifyOAuth::authorize() {
    if (!config_.is_valid()) {
        return {
            SpotifyAuthorizationStatus::invalid_configuration,
            "Client ID and port number cannot be empty."
        };
    }

    SpotifyAuthorizationSession session;
    session.redirect_uri =
        build_redirect_uri(config_);

    std::string code_challenge;

    if (!create_pkce_session(session, code_challenge)) {
        return {
            SpotifyAuthorizationStatus::invalid_configuration,
            "Unable to generate PKCE credentials."
        };
    }

    const std::string authorization_link =
        build_authorization_link(
            config_,
            session.redirect_uri,
            code_challenge,
            session.state
        );

    try {
        const char* server_options[] {
            "listening_ports",
            config_.port_number.c_str(),
            "document_root",
            ".",
            nullptr
        };

        CivetServer server(server_options);

        SpotifyCallbackHandler callback_handler(
            config_,
            session
        );

        server.addHandler(
            callback_path.data(),
            callback_handler
        );

        std::cout
            << "Server started on port "
            << config_.port_number
            << ".\n";

        if (!open_authorization_link(authorization_link)) {
            return {
                SpotifyAuthorizationStatus::browser_open_failed,
                "Unable to open the Spotify authorization link."
            };
        }

        std::cout
            << "Authorization link:\n"
            << authorization_link
            << '\n';

        while (!session.complete.load(
            std::memory_order_acquire
        )) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(100)
            );
        }
    }
    catch (const std::exception& exception) {
        return {
            SpotifyAuthorizationStatus::server_start_failed,
            "Unable to run the local authorization server: "
                + std::string(exception.what())
        };
    }

    return {
        session.status,
        std::move(session.message)
    };
}

SpotifyAuthorizationResult SpotifyOAuth::register_devices() {
    if (config_.client_id.empty()) {
        return {
            SpotifyAuthorizationStatus::invalid_configuration,
            "Client ID cannot be empty."
        };
    }

    const auto tokens = ac::ini::read(config_.token_path);
    if (!tokens) {
        return {
            SpotifyAuthorizationStatus::invalid_configuration,
            "Token file is missing. Generate new tokens first."
        };
    }

    const auto loaded_access = tokens->find("tokens", "access_token");
    const auto loaded_refresh = tokens->find("tokens", "refresh_token");
    const auto refresh_expires_at =
        tokens->find("tokens", "refresh_expires_at");

    if (
        !loaded_refresh ||
        loaded_refresh->empty()
    ) {
        return {
            SpotifyAuthorizationStatus::invalid_configuration,
            "Refresh token is missing. Generate new tokens first."
        };
    }

    std::string access_token =
        loaded_access ? std::string {*loaded_access} : std::string {};
    std::string refresh_token {*loaded_refresh};

    const cpr::Response refresh_response = cpr::Post(
        cpr::Url { std::string(token_endpoint) },
        cpr::Header {
            {
                "Content-Type",
                "application/x-www-form-urlencoded"
            }
        },
        cpr::Payload {
            {"grant_type", "refresh_token"},
            {"refresh_token", refresh_token},
            {"client_id", config_.client_id}
        }
    );

    if (refresh_response.status_code != 200) {
        try {
            const nlohmann::json refresh_json =
                nlohmann::json::parse(refresh_response.text);

            if (
                refresh_json.contains("error") &&
                refresh_json["error"] == "invalid_grant"
            ) {
                return {
                    SpotifyAuthorizationStatus::token_exchange_failed,
                    "Refresh token is no longer valid. "
                    "Generate new tokens first."
                };
            }
        }
        catch (const nlohmann::json::exception&) {
        }

        return {
            SpotifyAuthorizationStatus::token_exchange_failed,
            "Spotify token refresh failed with status code "
                + std::to_string(refresh_response.status_code)
                + ". Response: "
                + refresh_response.text
        };
    }

    try {
        const nlohmann::json refresh_json =
            nlohmann::json::parse(refresh_response.text);

        if (
            !refresh_json.contains("access_token") ||
            !refresh_json["access_token"].is_string()
        ) {
            return {
                SpotifyAuthorizationStatus::token_exchange_failed,
                "Spotify did not return a valid access token."
            };
        }

        access_token = refresh_json["access_token"].get<std::string>();

        if (
            refresh_json.contains("refresh_token") &&
            refresh_json["refresh_token"].is_string() &&
            !refresh_json["refresh_token"].get<std::string>().empty()
        ) {
            refresh_token =
                refresh_json["refresh_token"].get<std::string>();
        }
    }
    catch (const nlohmann::json::exception& exception) {
        return {
            SpotifyAuthorizationStatus::token_exchange_failed,
            "Unable to parse Spotify token response: "
                + std::string(exception.what())
        };
    }

    const std::time_t now = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now()
    );
    const std::string refresh_expires_text =
        refresh_expires_at && !refresh_expires_at->empty()
            ? std::string {*refresh_expires_at}
            : std::to_string(now);

    std::ostringstream token_contents;
    token_contents
        << "; Machine-local. Do not copy this file.\n"
        << "[tokens]\n"
        << "access_token = " << access_token << '\n'
        << "refresh_token = " << refresh_token << '\n'
        << "authorized_at = " << now << '\n'
        << "refresh_expires_at = " << refresh_expires_text << '\n';

    std::string error_message;

    if (!ensure_data_directory(config_, error_message)) {
        return {
            SpotifyAuthorizationStatus::token_save_failed,
            std::move(error_message)
        };
    }

    if (!write_text_file(
        config_.token_path,
        token_contents.str(),
        error_message
    )) {
        return {
            SpotifyAuthorizationStatus::token_save_failed,
            std::move(error_message)
        };
    }

    const cpr::Response devices_response = cpr::Get(
        cpr::Url { "https://api.spotify.com/v1/me/player/devices" },
        cpr::Header {
            {"Authorization", "Bearer " + access_token},
            {"Content-Type", "application/json"}
        }
    );

    if (devices_response.status_code != 200) {
        return {
            SpotifyAuthorizationStatus::device_request_failed,
            "Spotify device request failed with status code "
                + std::to_string(devices_response.status_code)
                + ". Response: "
                + devices_response.text
        };
    }

    std::vector<std::pair<std::string, std::string>> devices =
        load_devices(config_.config_path);

    try {
        const nlohmann::json devices_json =
            nlohmann::json::parse(devices_response.text);

        if (
            !devices_json.contains("devices") ||
            !devices_json["devices"].is_array()
        ) {
            return {
                SpotifyAuthorizationStatus::device_request_failed,
                "Spotify did not return a device list."
            };
        }

        for (const auto& device : devices_json["devices"]) {
            if (
                !device.contains("name") ||
                !device.contains("id") ||
                !device["name"].is_string() ||
                !device["id"].is_string()
            ) {
                continue;
            }

            upsert_device(
                devices,
                device["name"].get<std::string>(),
                device["id"].get<std::string>()
            );
        }
    }
    catch (const nlohmann::json::exception& exception) {
        return {
            SpotifyAuthorizationStatus::device_request_failed,
            "Unable to parse Spotify device response: "
                + std::string(exception.what())
        };
    }

    if (!write_sp_config(config_, devices, error_message)) {
        return {
            SpotifyAuthorizationStatus::token_save_failed,
            std::move(error_message)
        };
    }

    std::ostringstream summary;
    summary << "Registered Spotify devices:\n";

    if (devices.empty()) {
        summary << "(none available)\n";
    }
    else {
        for (const auto& [name, id] : devices) {
            summary << name << " = " << id << '\n';
        }
    }

    return {
        SpotifyAuthorizationStatus::success,
        summary.str()
    };
}
