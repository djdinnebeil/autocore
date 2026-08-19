/**
 * \file console_writer.ixx
 * \brief Provides synchronized component output to standard streams.
 */
export module auto_core.component:console_writer;

import std;

namespace ac::component_detail {

    /** \brief Serializes and flushes component output to standard streams. */
    class ConsoleWriter {
    public:
        /** \brief Writes to stdout and optionally appends a newline. */
        void write(std::string_view message, bool newline = true);
        /** \brief Writes to stderr and optionally appends a newline. */
        void write_error(std::string_view message, bool newline = true);

    private:
        void write_to(
            std::ostream& stream,
            std::string_view message,
            bool newline
        );

        std::mutex mutex_;
    };

} // namespace ac::component_detail
