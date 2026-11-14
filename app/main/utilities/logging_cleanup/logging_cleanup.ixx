/**
 * \file logging_cleanup.ixx
 * \brief In-process log cleanup for auto_core.exe.
 *
 * Retention uses an elapsed UTC cutoff. Size cleanup then deletes oldest
 * files first. Logs named with the current local date are kept.
 *
 * Directory, retention, and size come from the process-lifetime logging
 * configuration. `ac::logging::config::resolve()` reads `logging_policy.ini`
 * once for the host process, so edits apply after the host restarts.
 */
export module auto_core.utility.logging_cleanup;

import std;

export namespace ac::logging_cleanup {

    enum class Status {
        finished,
        failed,
        busy,
        incomplete
    };

    struct Request {
        bool dry_run = false;
    };

    struct Summary {
        bool available = false;
        bool dry_run = false;
        int retention_deleted = 0;
        int size_deleted = 0;
        int projected_retention = 0;
        int projected_size = 0;
        bool blocked_by_today = false;
        std::uint64_t remaining_bytes = 0;
    };

    struct Result {
        Status status = Status::finished;
        std::string detail;
        Summary summary;
    };

    /**
     * \brief Deletes expired logs under the resolved logging directory.
     *
     * `request.dry_run` reports the same decisions without deleting.
     * `Status::finished` means the pass completed. `Status::failed` means
     * the directory could not be read or the cleanup lock could not be
     * created. `Status::busy` means another cleanup holds the lock.
     * `Status::incomplete` means a delete or rescan failed.
     */
    Result run(const Request& request);

}
