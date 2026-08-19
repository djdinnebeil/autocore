module;

#include <cpr/cpr.h>

module cloud;

import std;
import journal_component;

namespace {
constexpr std::string_view firebase_url =
    "https://auto-core-cloud-default-rtdb.firebaseio.com/star.json";

std::string remove_outer_quotes(const std::string& input) {
    if (input.size() >= 2 && input.front() == '"' && input.back() == '"') {
        return input.substr(1, input.size() - 2);
    }
    return input;
}

std::string quote(const std::string& input) {
    return std::format("\"{}\"", input);
}
} // namespace

void get_string_from_firebase() {
    const auto response = cpr::Get(
        cpr::Url {firebase_url},
        cpr::ConnectTimeout {2000},
        cpr::Timeout {5000}
    );
    if (response.error.code != cpr::ErrorCode::OK) {
        journal_component().print(
            "HTTP GET request failed: {}",
            response.error.message
        );
        return;
    }
    if (response.status_code != cpr::status::HTTP_OK) {
        journal_component().print(
            "HTTP GET request failed with status: {}",
            response.status_code
        );
        return;
    }
    journal_component().logg_and_logg(
        "Retrieved data: {}",
        remove_outer_quotes(response.text)
    );
}

void update_string_in_firebase(const std::string& value) {
    const auto response = cpr::Put(
        cpr::Url {firebase_url},
        cpr::Body {quote(value)},
        cpr::Header {{"Content-Type", "application/json"}},
        cpr::ConnectTimeout {2000},
        cpr::Timeout {5000}
    );
    if (response.error.code != cpr::ErrorCode::OK) {
        journal_component().print(
            "HTTP PUT request failed: {}",
            response.error.message
        );
        return;
    }
    if (response.status_code != cpr::status::HTTP_OK) {
        journal_component().print(
            "HTTP PUT request failed with status: {}",
            response.status_code
        );
        return;
    }
    journal_component().logg_and_logg(
        "Updated data: {}",
        remove_outer_quotes(response.text)
    );
}
