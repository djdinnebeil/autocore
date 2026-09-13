/**
 * \file console_writer.ixx
 * \brief Provides synchronized component output to standard streams.
 */
export module auto_core.core.component:console_writer;

import std;

namespace ac::component_detail {

    /**
     * \brief Serializes component output to standard streams.
     *
     * Each write is locked and flushed so console output stays ordered and
     * visible immediately.
     */
    class ConsoleWriter {
    public:
        /** \brief Writes to stdout, optionally appends a newline, and flushes. */
        void write(std::string_view message, bool newline = true);
        /** \brief Writes to stderr, optionally appends a newline, and flushes. */
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
