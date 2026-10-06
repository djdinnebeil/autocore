/**
 * \file process.ixx
 * \brief Job containment, owner-process handles, and breakaway shell launch.
 *
 * Hosted components are placed in a per-process job. Long-lived helpers wait
 * on an inherited owner handle. User-facing shell launches go through
 * `auto_core_shell_launcher.exe`, which is created outside the component job.
 */
module;

#include "ac_api.hpp"

export module auto_core.core.process;

import std;

export namespace ac::process {

    struct Error {
        unsigned long system_error {};
    };

    [[nodiscard]] AC_API std::wstring component_job_name(
        unsigned long process_id
    );

    [[nodiscard]] AC_API bool in_component_job() noexcept;

    [[nodiscard]] AC_API std::expected<void*, Error> create_component_job(
        unsigned long process_id
    );

    [[nodiscard]] AC_API std::expected<std::size_t, Error> job_active_count(
        void* job
    ) noexcept;

    AC_API bool terminate_job(void* job, unsigned int exit_code) noexcept;

    AC_API bool try_clear_kill_on_job_close(void* job) noexcept;

    /**
     * \brief Assigns a suspended process to `job` and resumes it.
     *
     * On assignment or resume failure the process is terminated while it is
     * still suspended, or immediately after a failed resume, and `thread` is
     * closed. The caller still owns `process`.
     */
    [[nodiscard]] AC_API std::expected<void, Error> resume_in_job(
        void* job,
        void* process,
        void*& thread
    ) noexcept;

    [[nodiscard]] AC_API std::expected<void*, Error> create_process_with_owner(
        const std::filesystem::path& executable,
        std::wstring command_line,
        const std::filesystem::path& working_directory
    );

    [[nodiscard]] AC_API std::expected<void*, Error> parse_owner_handle(
        std::string_view text
    );

    /**
     * \brief Watches an owner process from the serve thread.
     *
     * Construct this on the thread blocked in synchronous pipe I/O. Owner
     * death runs `on_owner_exit` and cancels that thread's synchronous I/O.
     * It does not call `ExitProcess`.
     */
    class OwnerWatch {
    public:
        AC_API OwnerWatch() noexcept;
        AC_API OwnerWatch(void* owner, std::function<void()> on_owner_exit);
        AC_API ~OwnerWatch();

        OwnerWatch(const OwnerWatch&) = delete;
        OwnerWatch& operator=(const OwnerWatch&) = delete;

        AC_API OwnerWatch(OwnerWatch&& other) noexcept;
        AC_API OwnerWatch& operator=(OwnerWatch&& other) noexcept;

        [[nodiscard]] AC_API bool active() const noexcept;

    private:
        struct State;
        State* state_ {nullptr};
    };

    enum class ShellLaunchFailure {
        launcher_missing,
        breakaway_denied,
        create_failed,
        launcher_failed,
    };

    struct ShellLaunchError {
        ShellLaunchFailure reason {ShellLaunchFailure::create_failed};
        unsigned long system_error {};
    };

    [[nodiscard]] AC_API std::expected<void, ShellLaunchError>
        shell_launch_outside_job(
            std::wstring_view verb,
            std::wstring_view file,
            std::wstring_view parameters,
            std::wstring_view directory
        );

    enum class Handoff {
        transferred,
        already_empty,
        failed,
    };

    struct HandoffResult {
        Handoff status {Handoff::failed};
        unsigned long system_error {};
        /** \brief Newly opened process handle. The caller closes it. */
        void* target {nullptr};
    };

    [[nodiscard]] AC_API HandoffResult handoff_job(
        void* job,
        void* root_process
    ) noexcept;

} // namespace ac::process
