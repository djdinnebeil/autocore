#include "catch_amalgamated.hpp"
#include "../editors/components/components_list_publish.hpp"

#include <cstdint>
#include <fstream>
#include <iterator>
#include <mutex>
#include <string>
#include <thread>

namespace publish = ac::components_list_publish;

namespace {

struct TempRoot {
    std::filesystem::path path;

    TempRoot()
        : path(
            std::filesystem::temp_directory_path() /
            ("auto_core_list_lock_" + std::to_string(GetCurrentProcessId()) +
             "_" + std::to_string(reinterpret_cast<std::uintptr_t>(this)))
        ) {
        std::error_code error;
        std::filesystem::remove_all(path, error);
        std::filesystem::create_directories(path);
    }

    ~TempRoot() {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
};

struct Event {
    HANDLE handle {nullptr};

    Event()
        : handle(CreateEventW(nullptr, TRUE, FALSE, nullptr)) {
    }

    ~Event() {
        if (handle != nullptr) {
            CloseHandle(handle);
        }
    }

    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;
};

void signal_pending(void* raw) {
    SetEvent(static_cast<HANDLE>(raw));
}

}

TEST_CASE(
    "List lock path follows the supplied installation root",
    "[components-list][lock]"
) {
    const auto root = std::filesystem::path {R"(C:\Auto Core Install)"};
    const auto lock = publish::list_lock_file(root);
    const auto expected =
        root / "data" / "auto_core" / "components.list.lock";

    CHECK(lock == expected);
    CHECK(lock.is_absolute());

    const auto previous = std::filesystem::current_path();
    struct RestoreWorkingDirectory {
        std::filesystem::path previous;
        ~RestoreWorkingDirectory() {
            std::error_code error;
            std::filesystem::current_path(previous, error);
        }
    } restore {previous};

    std::filesystem::current_path(std::filesystem::temp_directory_path());
    CHECK(publish::list_lock_file(root) == expected);
}

TEST_CASE(
    "Acquire creates a missing lock directory and keeps the lock file",
    "[components-list][lock]"
) {
    TempRoot root;
    const auto lock_path = publish::list_lock_file(root.path);
    std::string error;

    REQUIRE_FALSE(std::filesystem::exists(lock_path.parent_path()));
    {
        publish::ListUpdateLock lock;
        REQUIRE(lock.acquire(lock_path, error));
        CHECK(error.empty());
    }

    REQUIRE(std::filesystem::is_directory(lock_path.parent_path()));
    REQUIRE(std::filesystem::is_regular_file(lock_path));
    {
        std::ofstream marker(lock_path, std::ios::binary | std::ios::trunc);
        REQUIRE(marker);
        marker << 'm';
    }

    {
        publish::ListUpdateLock lock;
        REQUIRE(lock.acquire(lock_path, error));
    }

    std::ifstream marker(lock_path, std::ios::binary);
    REQUIRE(marker);
    const std::string bytes {
        std::istreambuf_iterator<char> {marker},
        std::istreambuf_iterator<char> {}
    };
    CHECK(bytes == "m");
    CHECK(std::filesystem::is_regular_file(lock_path));
}

TEST_CASE(
    "Directory creation failure does not open the lock file",
    "[components-list][lock]"
) {
    TempRoot root;
    const auto occupied = root.path / "data" / "auto_core";
    std::filesystem::create_directories(occupied.parent_path());
    {
        std::ofstream file(occupied, std::ios::binary | std::ios::trunc);
        REQUIRE(file);
        file << 'x';
    }

    const auto lock_path = publish::list_lock_file(root.path);
    publish::ListUpdateLock lock;
    std::string error;
    REQUIRE_FALSE(lock.acquire(lock_path, error));
    CHECK(error.find("Failed to create") != std::string::npos);
    CHECK_FALSE(std::filesystem::exists(lock_path));
    CHECK(std::filesystem::is_regular_file(occupied));
}

TEST_CASE(
    "A held list lock blocks the next acquire until release",
    "[components-list][lock]"
) {
    TempRoot root;
    const auto lock_path = publish::list_lock_file(root.path);
    std::string error;
    publish::ListUpdateLock held;
    REQUIRE(held.acquire(lock_path, error));

    Event pending;
    Event finished;
    REQUIRE(pending.handle != nullptr);
    REQUIRE(finished.handle != nullptr);

    std::mutex result_mutex;
    bool waiter_ok = false;
    std::string waiter_error;
    std::thread waiter([&] {
        publish::ListUpdateLock waiting;
        std::string local_error;
        const bool ok = waiting.acquire(
            lock_path,
            local_error,
            signal_pending,
            pending.handle
        );
        {
            std::lock_guard guard(result_mutex);
            waiter_ok = ok;
            waiter_error = std::move(local_error);
        }
        SetEvent(finished.handle);
    });

    struct ReleaseAndJoin {
        publish::ListUpdateLock& held;
        std::thread& waiter;

        ~ReleaseAndJoin() {
            held.release();
            if (!waiter.joinable()) {
                return;
            }
            const HANDLE thread = waiter.native_handle();
            if (WaitForSingleObject(thread, 5000) != WAIT_OBJECT_0) {
                TerminateThread(thread, 1);
                if (WaitForSingleObject(thread, 1000) != WAIT_OBJECT_0) {
                    waiter.detach();
                    return;
                }
            }
            waiter.join();
        }
    } release_and_join {held, waiter};

    const HANDLE ready[] {pending.handle, finished.handle};
    const DWORD signaled = WaitForMultipleObjects(2, ready, FALSE, 5000);
    REQUIRE(signaled == WAIT_OBJECT_0);
    REQUIRE(WaitForSingleObject(finished.handle, 0) == WAIT_TIMEOUT);

    held.release();
    REQUIRE(WaitForSingleObject(finished.handle, 5000) == WAIT_OBJECT_0);
    {
        std::lock_guard guard(result_mutex);
        REQUIRE(waiter_ok);
        CHECK(waiter_error.empty());
    }
    CHECK(std::filesystem::is_regular_file(lock_path));
}
