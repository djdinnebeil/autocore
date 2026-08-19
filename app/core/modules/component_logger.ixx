/**
 * \file component_logger.ixx
 * \brief Owns the local file log for one Auto Core component.
 */
export module auto_core.component:logger;

import std;
import auto_core.clock;

namespace ac::component_detail {

    /**
     * \brief Owns a synchronized, daily file log for one component session.
     *
     * Files are named `<date>_<component>.log`. Construction appends a session
     * header, date changes write continuation markers, and destruction appends
     * a session footer. File failures are reported through `auto_core.error`.
     */
    class ComponentLogger {
    public:
        /**
         * \brief Opens the component's session log.
         * \param component_name The name used in records and the filename.
         * \param directory The directory that owns the log files.
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
         * Records ending in a newline are flushed immediately.
         */
        void write(std::string_view message, bool newline = true);
        /** \brief Rolls the log file when the local calendar date changed. */
        void update_file();
        /** \brief Flushes the current file when it is open. */
        void flush();

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };

} // namespace ac::component_detail
