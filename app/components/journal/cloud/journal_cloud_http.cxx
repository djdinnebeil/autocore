module;

#include <cpr/cpr.h>

module journal_cloud_http;

import std;
import journal_firebase;

namespace journal::cloud::http {

std::expected<void, std::string> push(
    const std::string_view url,
    const std::string_view name,
    const int count
) {
    const auto response = cpr::Put(
        cpr::Url {std::string {url}},
        cpr::Body {journal::firebase::payload(name, count)},
        cpr::Header {{"Content-Type", "application/json"}},
        cpr::ConnectTimeout {2000},
        cpr::Timeout {5000}
    );
    if (response.error.code != cpr::ErrorCode::OK) {
        return std::unexpected(std::format(
            "HTTP PUT request failed: {}",
            response.error.message
        ));
    }
    if (response.status_code != cpr::status::HTTP_OK) {
        return std::unexpected(std::format(
            "HTTP PUT request failed with status: {}",
            response.status_code
        ));
    }
    return {};
}

} // namespace journal::cloud::http
