/**
 * \file spotify_http.ixx
 * \brief Defines Spotify HTTP response classification.
 */
export module spotify_http;

export namespace spotify_http {
    /** Returns true when an HTTP status represents a successful response. */
    constexpr bool is_success_status(const int status_code) noexcept {
        return status_code >= 200 && status_code < 300;
    }
}
