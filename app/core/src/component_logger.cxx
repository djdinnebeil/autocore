module auto_core.component;

import :logger;
import std;
import auto_core.clock;
import auto_core.error;

namespace {

    void write_to_stream(
        std::ostream& stream,
        const std::string_view message,
        const bool newline
    ) {
        stream << message;

        if (newline) {
            stream << '\n';
        }
    }

}

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
        std::mutex mutex;

        void open_file_unlocked() {
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

            const std::filesystem::path logger_path =
                directory / (session_start.date_iso + "_" + name + ".log");

            log_stream.open(logger_path, std::ios::app);

            if (!log_stream.is_open()) {
                ac::error::log(
                    "Failed to open component log file: {}",
                    logger_path
                );
                return;
            }

            logger_date = session_start.date_iso;
            log_stream
                << "Session started for "
                << name
                << " "
                << ac::clock::format_datetime(session_start)
                << '\n';
        }

        void update_file_unlocked() {
            const std::string current_date_iso = ac::clock::get_date_iso();

            if (logger_date == current_date_iso) {
                return;
            }

            if (log_stream.is_open()) {
                log_stream
                    << "--- Session continues in next log file ---\n";
                log_stream.close();
            }

            const std::filesystem::path logger_path =
                directory / (current_date_iso + "_" + name + ".log");

            log_stream.open(logger_path, std::ios::app);

            if (!log_stream.is_open()) {
                ac::error::log(
                    "Failed to open component log file: {}",
                    logger_path
                );
                return;
            }

            log_stream
                << "--- Session continues from "
                << ac::clock::format_datetime(session_start)
                << " ---\n";

            logger_date = current_date_iso;
        }

        void close_file_unlocked() {
            if (!log_stream.is_open()) {
                return;
            }

            const auto end = ac::clock::get_local_datetime();
            log_stream
                << "Session ended for "
                << name
                << " "
                << end.date_iso
                << " at "
                << end.timestamp_with_seconds
                << "\n***\n";
            log_stream.close();
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
        impl_->open_file_unlocked();
    }

    ComponentLogger::~ComponentLogger() noexcept {
        try {
            std::scoped_lock lock(impl_->mutex);
            impl_->close_file_unlocked();
        }
        catch (const std::exception& exception) {
            ac::error::log(
                "Unable to close component log: {}",
                exception.what()
            );
        }
        catch (...) {
            ac::error::log(
                "Unable to close component log: unknown error."
            );
        }
    }

    void ComponentLogger::write(
        const std::string_view message,
        const bool newline
    ) {
        std::scoped_lock lock(impl_->mutex);
        impl_->update_file_unlocked();

        if (!impl_->log_stream.is_open()) {
            return;
        }

        write_to_stream(impl_->log_stream, message, newline);

        if (newline) {
            impl_->log_stream.flush();
        }
    }

    void ComponentLogger::update_file() {
        std::scoped_lock lock(impl_->mutex);
        impl_->update_file_unlocked();
    }

    void ComponentLogger::flush() {
        std::scoped_lock lock(impl_->mutex);

        if (impl_->log_stream.is_open()) {
            impl_->log_stream.flush();
        }
    }

}
