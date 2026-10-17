/**
 * \file component_logger.ixx
 * \brief Owns the local file log for one Auto Core component.
 */
export module auto_core.core.component:logger;

import std;
import auto_core.core.clock;

namespace ac::component_detail {

    /**
     * \brief Owns the synchronized daily log for one component.
     *
     * The file is named `<date>_<component>.log` under the supplied
     * directory (`components/<name>/` from `ac::Component`). Construction
     * opens the file and appends a session header. Each `write` checks the
     * supplied event date and may roll the open file. File failures are
     * reported through `auto_core.core.error`; later writes no-op until a
     * roll retries the open.
     */
    class ComponentLogger {
    public:
        /**
         * \brief Opens the component's session log.
         * \param component_name The name used in records and the filename.
         * \param directory The directory that owns the log file.
         * \param session_start The stable start time for this session.
         */
        ComponentLogger(
            std::string_view component_name,
            std::filesystem::path directory,
            const ac::clock::DateTime& session_start
        );
        ~ComponentLogger() noexcept;

        ComponentLogger(const ComponentLogger&) = delete;
        ComponentLogger& operator=(const ComponentLogger&) = delete;
        ComponentLogger(ComponentLogger&&) = delete;
        ComponentLogger& operator=(ComponentLogger&&) = delete;

        /**
         * \brief Appends a message, rolling to today's file when needed.
         *
         * Compares the local calendar date with the open file and rolls when
         * they differ. `event_type` is written after the timestamp whenever
         * the message opens a persisted line. Records ending in a newline are
         * flushed immediately.
         */
        void write(
            std::string_view timestamp,
            std::string_view date_iso,
            std::string_view event_type,
            std::string_view message,
            bool newline = true
        );
        /** \brief Rolls the log file when the local calendar date changed. */
        void update_file();
        /** \brief Flushes the current file when it is open. */
        void flush();

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };

} // namespace ac::component_detail
