module itunes_runtime;

namespace itunes::runtime {

    namespace {
        thread_local const serial_executor* active_executor = nullptr;
    }

    class serial_executor::impl {
    public:
        explicit impl(const serial_executor* owner)
            : worker([this, owner] {
                active_executor = owner;
                run();
                active_executor = nullptr;
            }) {
        }

        void run() {
            std::unique_lock lock {mutex};
            while (true) {
                condition.wait(lock, [this] {
                    return stopping || !tasks.empty();
                });

                if (stopping && tasks.empty()) {
                    return;
                }

                auto task = std::move(tasks.front());
                tasks.pop_front();
                lock.unlock();
                task();
                lock.lock();
            }
        }

        std::mutex mutex;
        std::condition_variable condition;
        std::deque<std::function<void()>> tasks;
        bool stopping = false;
        std::thread worker;
    };

    serial_executor::serial_executor()
        : impl_(std::make_unique<impl>(this)) {
    }

    serial_executor::~serial_executor() noexcept {
        stop();
    }

    void serial_executor::enqueue(std::function<void()> task) {
        {
            const std::scoped_lock lock {impl_->mutex};
            if (impl_->stopping) {
                throw std::logic_error("Serial executor has stopped");
            }
            impl_->tasks.push_back(std::move(task));
        }
        impl_->condition.notify_one();
    }

    bool serial_executor::is_worker_thread() const noexcept {
        return active_executor == this;
    }

    void serial_executor::stop() noexcept {
        if (!impl_) {
            return;
        }

        {
            const std::scoped_lock lock {impl_->mutex};
            impl_->stopping = true;
        }
        impl_->condition.notify_one();

        if (impl_->worker.joinable() && !is_worker_thread()) {
            impl_->worker.join();
        }
    }

    void play_pause(automation& player) {
        player.play_pause();
        static_cast<void>(player.get_current_track());
    }

    void previous_song(automation& player) {
        player.prev_song();
        static_cast<void>(player.get_current_track());
    }

    void next_song(automation& player, playback_monitor& monitor) {
        player.next_song();
        monitor.playback_state_changed();
    }

    std::wstring stop_song(automation& player) {
        player.stop_song();
        player.play_pause();
        player.play_pause();
        return player.get_current_track();
    }

    removal_result remove_song(
        automation& player,
        file_recycler& recycler
    ) {
        static_cast<void>(player.get_current_track());
        if (!player.has_current_track()) {
            return {.status = removal_status::no_current_track};
        }

        const std::filesystem::path path = player.remove_current_track();
        return {
            .status = recycler.recycle(path)
                ? removal_status::recycled
                : removal_status::recycle_failed,
            .path = path
        };
    }

    bool initialize_with_retry(
        lifecycle& target,
        const retry_policy policy,
        const retry_waiter& wait
    ) {
        if (target.is_initialized()) {
            return true;
        }

        for (std::size_t attempt = 0;
             attempt < policy.maximum_attempts;
             ++attempt) {
            if (target.initialize_attempt()) {
                return true;
            }

            if (attempt + 1 < policy.maximum_attempts) {
                wait(policy.delay);
            }
        }

        return false;
    }

} // namespace itunes::runtime
