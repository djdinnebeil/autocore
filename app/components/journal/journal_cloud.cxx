module;

#include <cpr/cpr.h>

module journal_cloud;

import std;
import auto_core.core.thread;
import journal_component;
import journal_database;

namespace {

std::string remove_outer_quotes(const std::string& input) {
    if (input.size() >= 2 && input.front() == '"' && input.back() == '"') {
        return input.substr(1, input.size() - 2);
    }
    return input;
}

std::string quote(const std::string& input) {
    return std::format("\"{}\"", input);
}

std::optional<std::string> load_firebase_url() {
    const auto url = journal_database::firebase_url();
    if (!url) {
        journal_component().logg_and_logg("{}", url.error());
        return std::nullopt;
    }
    return *url;
}

void put_string(const std::string& url, const std::string& value) {
    const auto response = cpr::Put(
        cpr::Url {url},
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

void get_string(const std::string& url) {
    const auto response = cpr::Get(
        cpr::Url {url},
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

} // namespace

void get_string_from_firebase() {
    const auto url = load_firebase_url();
    if (!url) {
        return;
    }
    std::thread worker([url = *url] {
        ac::thread::run_with_exception_handling(
            [url] { get_string(url); },
            journal_component()
        );
    });
    worker.detach();
}

void update_string_in_firebase(const std::string& value) {
    const auto url = load_firebase_url();
    if (!url) {
        return;
    }
    std::thread worker([url = *url, value] {
        ac::thread::run_with_exception_handling(
            [url, value] { put_string(url, value); },
            journal_component()
        );
    });
    worker.detach();
}
