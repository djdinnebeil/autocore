module auto_core.core.component;

import :logger;
import std;
import auto_core.core.clock;
import auto_core.core.error;

namespace {

    bool write_record(
        std::ostream& stream,
        bool& line_open,
        const std::string_view timestamp,
        const std::string_view message,
        const bool newline
    ) {
        if (!stream.good()) {
            return false;
        }

        std::size_t offset = 0;
        while (offset < message.size()) {
            if (!line_open) {
                stream << '[' << timestamp << "] ";
                line_open = true;
            }

            const std::size_t end = message.find('\n', offset);
            if (end == std::string_view::npos) {
                stream << message.substr(offset);
                offset = message.size();
            }
            else {
                stream << message.substr(offset, end - offset) << '\n';
                line_open = false;
                offset = end + 1;
            }
        }

        if (message.empty() && !line_open) {
            stream << '[' << timestamp << "] ";
            line_open = true;
        }

        if (newline && line_open) {
            stream << '\n';
            line_open = false;
        }

        if (newline) {
            stream.flush();
        }

        return stream.good();
    }

    void close_open_line(std::ofstream& stream, bool& line_open) {
        if (stream.is_open() && line_open) {
            stream << '\n';
            line_open = false;
        }
    }

} // namespace

namespace ac::component_detail {

    class ComponentLogger::Impl {
    public:
        Impl(
            const std::string_view component_name,
            std::filesystem::path log_directory,
            const ac::clock::DateTime& component_session_start
        )
            : session_start(component_session_start),
              name(component_name),
              directory(std::move(log_directory)) {
        }

        ac::clock::DateTime session_start;
        std::string name;
        std::filesystem::path directory;
        std::string logger_date;
        std::ofstream log_stream;
        std::ofstream main_log_stream;
        bool line_open = false;
        bool main_line_open = false;
        std::mutex mutex;

        std::filesystem::path log_path(const std::string_view date) const {
            return directory / (std::string {date} + "_" + name + ".log");
        }

        std::filesystem::path main_log_path(
            const std::string_view date
        ) const {
            return directory /
                (std::string {date} + "_" + name + ".main.log");
        }

        void write_session_marker(
            const std::string_view timestamp,
            const std::string_view marker
        ) {
            if (!log_stream.is_open()) {
                return;
            }
            (void)write_record(
                log_stream,
                line_open,
                timestamp,
                marker,
                true
            );
        }

        void open_files(const std::string_view date) {
            std::error_code error;
            std::filesystem::create_directories(directory, error);
            if (error) {
                ac::error::log(
                    "Failed to create component log directory: {} - {}",
                    directory,
                    error.message()
                );
                return;
            }

            const auto comprehensive_path = log_path(date);
            log_stream.open(comprehensive_path, std::ios::app);
            if (!log_stream.is_open()) {
                ac::error::log(
                    "Failed to open component log file: {}",
                    comprehensive_path
                );
                return;
            }

            const auto central_subset_path = main_log_path(date);
            main_log_stream.open(central_subset_path, std::ios::app);
            if (!main_log_stream.is_open()) {
                ac::error::log(
                    "Failed to open component main log file: {}",
                    central_subset_path
                );
            }
        }

        void open_files_unlocked(
            const std::string_view date,
            const std::string_view timestamp,
            const bool initial
        ) {
            open_files(date);
            if (!log_stream.is_open()) {
                return;
            }

            logger_date = date;

            const std::string marker = initial
                ? std::format(
                    "Session started for {} {}",
                    name,
                    ac::clock::format_datetime(session_start)
                )
                : std::format(
                    "--- Session continues from {} ---",
                    ac::clock::format_datetime(session_start)
                );

            write_session_marker(timestamp, marker);
        }

        void close_files() {
            close_open_line(log_stream, line_open);
            log_stream.close();
            close_open_line(main_log_stream, main_line_open);
            main_log_stream.close();
        }

        void update_files_unlocked(
            const std::string_view date,
            const std::string_view timestamp
        ) {
            if (logger_date == date) {
                return;
            }

            if (log_stream.is_open()) {
                close_open_line(log_stream, line_open);
                (void)write_record(
                    log_stream,
                    line_open,
                    timestamp,
                    "--- Session continues in next log file ---",
                    true
                );
            }

            close_files();
            open_files_unlocked(date, timestamp, false);
        }

        void close_files_unlocked() {
            const auto end = ac::clock::get_local_datetime();
            const std::string timestamp =
                ac::clock::format_log_timestamp(end);

            if (log_stream.is_open()) {
                close_open_line(log_stream, line_open);
                (void)write_record(
                    log_stream,
                    line_open,
                    timestamp,
                    std::format(
                        "Session ended for {} {} at {}",
                        name,
                        end.date_iso,
                        end.timestamp_with_seconds
                    ),
                    true
                );
                (void)write_record(
                    log_stream,
                    line_open,
                    timestamp,
                    "***",
                    true
                );
            }

            close_files();
        }
    };

    ComponentLogger::ComponentLogger(
        const std::string_view component_name,
        std::filesystem::path directory,
        const ac::clock::DateTime& session_start
    )
        : impl_(std::make_unique<Impl>(
            component_name,
            std::move(directory),
            session_start
        )) {
        std::scoped_lock lock(impl_->mutex);
        impl_->open_files_unlocked(
            session_start.date_iso,
            ac::clock::format_log_timestamp(session_start),
            true
        );
    }

    ComponentLogger::~ComponentLogger() noexcept {
        try {
            std::scoped_lock lock(impl_->mutex);
            impl_->close_files_unlocked();
        }
        catch (const std::exception& exception) {
            ac::error::log(
                "Unable to close component logs: {}",
                exception.what()
            );
        }
        catch (...) {
            ac::error::log(
                "Unable to close component logs: unknown error."
            );
        }
    }

    void ComponentLogger::write(
        const std::string_view timestamp,
        const std::string_view date_iso,
        const std::string_view message,
        const bool main_worthy,
        const bool newline
    ) {
        std::scoped_lock lock(impl_->mutex);
        impl_->update_files_unlocked(date_iso, timestamp);

        if (!impl_->log_stream.is_open()) {
            return;
        }

        const bool comprehensive_written = write_record(
            impl_->log_stream,
            impl_->line_open,
            timestamp,
            message,
            newline
        );

        if (
            main_worthy && comprehensive_written &&
            impl_->main_log_stream.is_open()
        ) {
            (void)write_record(
                impl_->main_log_stream,
                impl_->main_line_open,
                timestamp,
                message,
                newline
            );
        }
    }

    void ComponentLogger::update_file() {
        const auto now = ac::clock::get_local_datetime();
        std::scoped_lock lock(impl_->mutex);
        impl_->update_files_unlocked(
            now.date_iso,
            ac::clock::format_log_timestamp(now)
        );
    }

    void ComponentLogger::flush() {
        std::scoped_lock lock(impl_->mutex);

        if (impl_->log_stream.is_open()) {
            impl_->log_stream.flush();
        }
        if (impl_->main_log_stream.is_open()) {
            impl_->main_log_stream.flush();
        }
    }

} // namespace ac::component_detail
