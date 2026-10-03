/**
 * \file journal_cloud_protocol.ixx
 * \brief Private `ac.journal.cloud.v1` contract between `journal_ac.exe` and
 * `journal_cloud.exe --serve`.
 *
 * This is not `ac.component.v1`. `journal_cloud.exe` is not a component.
 */
export module journal_cloud_protocol;

import std;

export namespace journal::cloud {

inline constexpr std::string_view protocol_id = "ac.journal.cloud.v1";
inline constexpr std::wstring_view pipe_name = L"ac_journal_cloud_pipe";

enum class Request : std::int32_t {
    push = 1,
    shutdown = 2,
};

[[nodiscard]] constexpr std::int32_t to_wire(const Request request) noexcept {
    return static_cast<std::int32_t>(request);
}

} // namespace journal::cloud
