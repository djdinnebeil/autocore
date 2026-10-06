/**
 * \file journal_cloud_client.ixx
 * \brief `journal_ac.exe` process owner for `journal_cloud.exe --serve`.
 */
export module journal_cloud_client;

import std;

export namespace journal::cloud {

[[nodiscard]] std::expected<void, std::string> start_service();
[[nodiscard]] bool service_running();
[[nodiscard]] std::expected<void, std::string> push(
    std::string_view name,
    int count
);
void shutdown_service();

} // namespace journal::cloud
