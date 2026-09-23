/**
 * \file itunes_runtime.ixx
 * \brief Testable coordination boundaries for iTunes component commands.
 */
export module itunes_runtime;

import std;

export namespace itunes::runtime {

    /**
     * \brief Runs synchronous tasks on one dedicated worker thread.
     *
     * Nested calls made by the worker execute directly to avoid self-deadlock.
     * Stopping drains already-queued work and rejects later submissions.
     */
    class serial_executor {
    public:
        serial_executor();
        ~serial_executor() noexcept;

        serial_executor(const serial_executor&) = delete;
        serial_executor& operator=(const serial_executor&) = delete;
        serial_executor(serial_executor&&) = delete;
        serial_executor& operator=(serial_executor&&) = delete;

        /**
         * Runs `function` on the worker thread and waits for the result.
         * Nested calls made by the worker execute directly.
         */
        template<typename Function>
        decltype(auto) invoke(Function&& function) {
            if (is_worker_thread()) {
                return std::invoke(std::forward<Function>(function));
            }

            using result_type = std::invoke_result_t<Function>;
            auto task = std::make_shared<std::packaged_task<result_type()>>(
                std::forward<Function>(function)
            );
            auto result = task->get_future();
            enqueue([task = std::move(task)] {
                (*task)();
            });
            return result.get();
        }

        /** Drains queued work and rejects later submissions. */
        void stop() noexcept;

    private:
        class impl;
        std::unique_ptr<impl> impl_;

        void enqueue(std::function<void()> task);
        [[nodiscard]] bool is_worker_thread() const noexcept;
    };

    /** COM/player operations used by command helpers. */
    class automation {
    public:
        virtual ~automation() = default;

        virtual void play_pause() = 0;
        virtual void next_song() = 0;
        virtual void prev_song() = 0;
        virtual void stop_song() = 0;
        [[nodiscard]] virtual std::wstring get_current_track() = 0;
        [[nodiscard]] virtual bool has_current_track() const noexcept = 0;
        [[nodiscard]] virtual std::filesystem::path remove_current_track() = 0;
    };

    /** COM initialization, readiness, and shutdown. */
    class lifecycle {
    public:
        virtual ~lifecycle() = default;
        [[nodiscard]] virtual bool initialize_attempt() = 0;
        [[nodiscard]] virtual bool is_initialized() const noexcept = 0;
        virtual void shutdown() noexcept = 0;
    };

    /** Bounded COM initialization retry. Defaults: 9 attempts, 250 ms apart. */
    struct retry_policy {
        std::size_t maximum_attempts = 9;
        std::chrono::milliseconds delay = std::chrono::milliseconds {250};
    };

    using retry_waiter = std::function<void(std::chrono::milliseconds)>;

    class playback_monitor {
    public:
        virtual ~playback_monitor() = default;
        virtual void playback_state_changed() = 0;
    };

    class file_recycler {
    public:
        virtual ~file_recycler() = default;
        [[nodiscard]] virtual bool recycle(
            const std::filesystem::path& path
        ) = 0;
    };

    enum class removal_status {
        no_current_track,
        recycled,
        recycle_failed
    };

    struct removal_result {
        removal_status status;
        std::filesystem::path path;
    };

    void play_pause(automation& player);
    void previous_song(automation& player);
    void next_song(automation& player, playback_monitor& monitor);
    [[nodiscard]] std::wstring stop_song(automation& player);
    [[nodiscard]] removal_result remove_song(
        automation& player,
        file_recycler& recycler
    );
    /**
     * Retries `initialize_attempt` up to `policy.maximum_attempts` times.
     * Nested worker calls run inline to avoid self-deadlock.
     */
    [[nodiscard]] bool initialize_with_retry(
        lifecycle& target,
        retry_policy policy,
        const retry_waiter& wait
    );

} // namespace itunes::runtime
