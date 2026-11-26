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
     * The file is named `<date>_<file>.log` under the supplied directory.
     * The file name defaults to the component name. `ac::Component`
     * supplies `components/<name>/` unless the caller selected a shared
     * daily log. A shared file keeps the component
     * name on every record. An unfinished line stays in memory until its
     * newline. Construction opens the daily file and writes a session
     * header. Each `write` checks the supplied event date and may roll the
     * open file. Writers of one resolved path share
     * `Local\AutoCore.component.log.<path>`. File failures are reported
     * through `auto_core.core.error`; later writes retry the open.
     */
    class ComponentLogger {
    public:
        /**
         * \brief Opens the component's session log.
         * \param component_name The name used in records and, by default, the filename.
         * \param directory The directory that owns the log file.
         * \param session_start The stable start time for this session.
         * \param file_name The filename stem. Empty uses `component_name`.
         */
        ComponentLogger(
            std::string_view component_name,
            std::filesystem::path directory,
            const ac::clock::DateTime& session_start,
            std::string_view file_name = {}
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
         * the message opens a persisted line. A finished line is one
         * `WriteFile`. Records ending in a newline are flushed immediately.
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
        /**
         * \brief Flushes bytes already written to the current file.
         *
         * An unfinished line remains in memory and is not published.
         */
        void flush();

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };

} // namespace ac::component_detail
