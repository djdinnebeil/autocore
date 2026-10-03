/**
 * \file journal_cloud_http.ixx
 * \brief Firebase HTTP used only by `journal_cloud.exe`.
 */
export module journal_cloud_http;

import std;

export namespace journal::cloud::http {

[[nodiscard]] std::expected<void, std::string> push(
    std::string_view url,
    std::string_view name,
    int count
);

} // namespace journal::cloud::http
