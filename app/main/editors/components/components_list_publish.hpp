/**
 * \file components_list_publish.hpp
 * \brief Lock and publish `components.list` for auto_core_components.exe.
 *
 * The lock file is stable coordination state. Publication writes a new
 * temporary file and then calls `ReplaceFileW` or `MoveFileExW`.
 * `REPLACEFILE_WRITE_THROUGH` is not used; Microsoft documents it as
 * unsupported. Flushing the temporary file is not a power-loss guarantee.
 */
#pragma once

#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

namespace ac::components_list_publish {

    struct PublishResult {
        bool ok {false};
        std::string message;
    };

    /**
     * \brief `<installation root>/data/auto_core/components.list.lock`.
     *
     * The directory is not created. `installation_root` is used as given.
     */
    [[nodiscard]]
    inline std::filesystem::path list_lock_file(
        const std::filesystem::path& installation_root
    ) {
        return installation_root / "data" / "auto_core" / "components.list.lock";
    }

    /**
     * \brief Called when an exclusive lock request is already pending.
     *
     * `acquire` invokes this before it waits. The lock is not held yet.
     */
    using LockPendingCallback = void (*)(void* context);

    /**
     * \brief Exclusive byte-range lock on `data/auto_core/components.list.lock`.
     *
     * A missing parent directory is created. Failure to create it returns
     * before the lock file is opened. The file is created if missing and is
     * never deleted or replaced. It is not opened with delete sharing.
     * `acquire` does not return until the exclusive range is held. Release
     * closes the handle.
     */
    class ListUpdateLock {
    public:
        ListUpdateLock() = default;
        ListUpdateLock(const ListUpdateLock&) = delete;
        ListUpdateLock& operator=(const ListUpdateLock&) = delete;

        ~ListUpdateLock() {
            release();
        }

        [[nodiscard]]
        bool acquire(
            const std::filesystem::path& lock_path,
            std::string& error,
            LockPendingCallback on_pending = nullptr,
            void* on_pending_context = nullptr
        ) {
            const auto directory = lock_path.parent_path();
            std::error_code directory_error;
            std::filesystem::create_directories(directory, directory_error);
            if (directory_error) {
                error =
                    "Failed to create " + directory.string() + " (" +
                    directory_error.message() + ").";
                return false;
            }

            handle_ = CreateFileW(
                lock_path.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                nullptr
            );
            if (handle_ == INVALID_HANDLE_VALUE) {
                error =
                    "Failed to open " + lock_path.string() +
                    " (error " + std::to_string(GetLastError()) + ").";
                return false;
            }

            const HANDLE pending_event = CreateEventW(
                nullptr,
                TRUE,
                FALSE,
                nullptr
            );
            if (pending_event == nullptr) {
                error =
                    "Failed to lock " + lock_path.string() +
                    " (error " + std::to_string(GetLastError()) + ").";
                CloseHandle(handle_);
                handle_ = INVALID_HANDLE_VALUE;
                return false;
            }

            OVERLAPPED overlapped {};
            overlapped.hEvent = pending_event;
            if (!LockFileEx(
                    handle_,
                    LOCKFILE_EXCLUSIVE_LOCK,
                    0,
                    1,
                    0,
                    &overlapped
                )) {
                const DWORD lock_error = GetLastError();
                if (lock_error != ERROR_IO_PENDING) {
                    error =
                        "Failed to lock " + lock_path.string() +
                        " (error " + std::to_string(lock_error) + ").";
                    CloseHandle(pending_event);
                    CloseHandle(handle_);
                    handle_ = INVALID_HANDLE_VALUE;
                    return false;
                }
                if (on_pending != nullptr) {
                    on_pending(on_pending_context);
                }
                const DWORD waited = WaitForSingleObject(pending_event, INFINITE);
                DWORD transferred = 0;
                const BOOL completed = GetOverlappedResult(
                    handle_,
                    &overlapped,
                    &transferred,
                    FALSE
                );
                if (waited != WAIT_OBJECT_0 || !completed) {
                    const DWORD wait_error = GetLastError();
                    CancelIoEx(handle_, &overlapped);
                    GetOverlappedResult(
                        handle_,
                        &overlapped,
                        &transferred,
                        TRUE
                    );
                    error =
                        "Failed to lock " + lock_path.string() +
                        " (error " +
                        std::to_string(
                            waited == WAIT_OBJECT_0 ? wait_error : waited
                        ) +
                        ").";
                    CloseHandle(pending_event);
                    CloseHandle(handle_);
                    handle_ = INVALID_HANDLE_VALUE;
                    return false;
                }
            }

            CloseHandle(pending_event);
            locked_ = true;
            return true;
        }

        void release() noexcept {
            if (locked_ && handle_ != INVALID_HANDLE_VALUE) {
                OVERLAPPED overlapped {};
                UnlockFileEx(handle_, 0, 1, 0, &overlapped);
                locked_ = false;
            }
            if (handle_ != INVALID_HANDLE_VALUE) {
                CloseHandle(handle_);
                handle_ = INVALID_HANDLE_VALUE;
            }
        }

    private:
        HANDLE handle_ {INVALID_HANDLE_VALUE};
        bool locked_ {false};
    };

    [[nodiscard]]
    inline PublishResult fail_before_publish(
        const std::filesystem::path& destination,
        const std::filesystem::path& temporary,
        const std::string_view action,
        const DWORD error
    ) {
        PublishResult result;
        const bool removed = DeleteFileW(temporary.c_str()) != 0;
        result.message =
            std::string {action} + " " + temporary.string() +
            " (error " + std::to_string(error) + "). " +
            destination.string() + " was not modified.";
        if (!removed) {
            result.message +=
                " The temporary file could not be removed (error " +
                std::to_string(GetLastError()) + ").";
        }
        return result;
    }

    [[nodiscard]]
    inline PublishResult fail_after_publish(
        const std::filesystem::path& destination,
        const std::filesystem::path& temporary,
        const DWORD error
    ) {
        PublishResult result;
        result.message =
            "Failed to publish " + destination.string() +
            " (error " + std::to_string(error) + "). The prepared file is " +
            temporary.string() +
            ". It was kept. The result for " + destination.string() +
            " is uncertain.";
        return result;
    }

    /**
     * \brief Writes `contents` to a new temporary file and publishes it.
     *
     * An existing destination uses `ReplaceFileW` with no backup and flags 0.
     * A missing destination uses `MoveFileExW` with flags 0. A failure before
     * that call removes the unfinished temporary file. A publication failure
     * keeps the temporary file, reports both paths, and does not claim the
     * destination pathname is unchanged.
     */
    [[nodiscard]]
    inline PublishResult publish_list(
        const std::filesystem::path& destination,
        const std::string_view contents
    ) {
        const auto directory = destination.parent_path();
        const DWORD pid = GetCurrentProcessId();
        HANDLE handle = INVALID_HANDLE_VALUE;
        std::filesystem::path temporary;
        for (unsigned attempt = 0; attempt < 16; ++attempt) {
            temporary =
                directory /
                ("components.list." + std::to_string(pid) + "." +
                    std::to_string(attempt) + ".tmp");
            handle = CreateFileW(
                temporary.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                0,
                nullptr,
                CREATE_NEW,
                FILE_ATTRIBUTE_NORMAL,
                nullptr
            );
            if (handle != INVALID_HANDLE_VALUE) {
                break;
            }
            if (GetLastError() != ERROR_FILE_EXISTS) {
                PublishResult result;
                result.message =
                    "Failed to create a temporary file for " +
                    destination.string() + " (error " +
                    std::to_string(GetLastError()) + "). " +
                    destination.string() + " was not modified.";
                return result;
            }
        }
        if (handle == INVALID_HANDLE_VALUE) {
            PublishResult result;
            result.message =
                "Failed to create a temporary file for " +
                destination.string() + ". " + destination.string() +
                " was not modified.";
            return result;
        }

        std::size_t written_total = 0;
        while (written_total < contents.size()) {
            const auto chunk = static_cast<DWORD>(std::min<std::size_t>(
                contents.size() - written_total,
                static_cast<std::size_t>(1u << 20)
            ));
            DWORD written = 0;
            if (!WriteFile(
                    handle,
                    contents.data() + written_total,
                    chunk,
                    &written,
                    nullptr
                ) ||
                written != chunk) {
                const DWORD error = GetLastError();
                CloseHandle(handle);
                return fail_before_publish(
                    destination,
                    temporary,
                    "Failed to write",
                    error == 0 ? ERROR_WRITE_FAULT : error
                );
            }
            written_total += written;
        }

        if (!FlushFileBuffers(handle)) {
            const DWORD error = GetLastError();
            CloseHandle(handle);
            return fail_before_publish(
                destination,
                temporary,
                "Failed to flush",
                error
            );
        }
        if (!CloseHandle(handle)) {
            return fail_before_publish(
                destination,
                temporary,
                "Failed to close",
                GetLastError()
            );
        }

        const DWORD attributes = GetFileAttributesW(destination.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES) {
            const DWORD error = GetLastError();
            if (error != ERROR_FILE_NOT_FOUND &&
                error != ERROR_PATH_NOT_FOUND) {
                return fail_after_publish(destination, temporary, error);
            }
            if (!MoveFileExW(temporary.c_str(), destination.c_str(), 0)) {
                return fail_after_publish(
                    destination,
                    temporary,
                    GetLastError()
                );
            }
        }
        else if (!ReplaceFileW(
                     destination.c_str(),
                     temporary.c_str(),
                     nullptr,
                     0,
                     nullptr,
                     nullptr
                 )) {
            return fail_after_publish(destination, temporary, GetLastError());
        }

        PublishResult result;
        result.ok = true;
        return result;
    }

} // namespace ac::components_list_publish
