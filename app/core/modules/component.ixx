/**
 * \file component.ixx
 * \brief Defines component-scoped output for Auto Core.
 */
module;

#include "ac_api.hpp"

export module auto_core.component;

import std;
import auto_core.clock;
import auto_core.formatting;

export namespace ac {
    /** \brief Selects the destinations for a component message. */
    enum class OutputRoute {
        /** Write only to the component's local file log. */
        component,
        /** Write to the component log and queue for the central logger. */
        component_and_main,
        /** Write to the component log, central logger, and stdout. */
        component_main_and_console
    };

    /**
     * \brief Coordinates local logging, central logging, console output, and
     * clipboard-based text insertion for one named component.
     *
     * Output routing is synchronized per component. Narrow text is UTF-8;
     * wide text is converted to UTF-8 before logging or console output.
     */
    class Component {
    public:
        /**
         * \brief Starts a named component session and opens its local log.
         * \param name The component name used for log directories and events.
         */
        AC_API explicit Component(std::string_view name);
        AC_API ~Component() noexcept;

        Component(const Component&) = delete;
        Component& operator=(const Component&) = delete;
        Component(Component&&) = delete;
        Component& operator=(Component&&) = delete;

        /**
         * \brief Returns the local date and time captured when this component
         * was constructed.
         *
         * The returned value remains unchanged for the component's lifetime.
         */
        [[nodiscard]]
        AC_API const clock::DateTime& session_start() const noexcept;

        /**
         * \brief Starts asynchronous central logging when globally enabled.
         *
         * Connection failure is recorded in the component log and stderr.
         */
        AC_API void connect_to_logger();
        /** \brief Queues a central-logger shutdown request when connected. */
        [[nodiscard]] AC_API bool request_logger_shutdown();

        /**
         * \brief Writes UTF-8 text to the selected destinations.
         * \param message The message bytes.
         * \param route The destinations that receive the message.
         * \param newline Whether to append a newline at each destination.
         */
        AC_API void write(std::string_view message, OutputRoute route,
            bool newline = true);
        /** \brief Converts Unicode text to UTF-8 and routes it as `write()`. */
        AC_API void write(std::wstring_view message, OutputRoute route,
            bool newline = true);

        /**
         * \name Output convenience methods
         *
         * `logg` writes to the component log. `logg_and_logg` also queues for
         * the central logger. `logg_and_print` and `print` additionally write
         * to stdout. Names containing `nl` suppress the trailing newline.
         * Narrow, wide, single-character, and formatted UTF-8 overloads are
         * provided. Formatting failures are reported to every destination.
         * \{
         */
        void logg(std::string_view message) { write(message, OutputRoute::component); }
        void loggnl(std::string_view message) { write(message, OutputRoute::component, false); }
        void logg_and_logg(std::string_view message) { write(message, OutputRoute::component_and_main); }
        void loggnl_and_loggnl(std::string_view message) { write(message, OutputRoute::component_and_main, false); }
        void logg_and_print(std::string_view message) { write(message, OutputRoute::component_main_and_console); }
        void loggnl_and_printnl(std::string_view message) { write(message, OutputRoute::component_main_and_console, false); }
        void print(std::string_view message) { write(message, OutputRoute::component_main_and_console); }
        void printnl(std::string_view message) { write(message, OutputRoute::component_main_and_console, false); }

        void logg(std::wstring_view message) { write(message, OutputRoute::component); }
        void loggnl(std::wstring_view message) { write(message, OutputRoute::component, false); }
        void logg_and_logg(std::wstring_view message) { write(message, OutputRoute::component_and_main); }
        void loggnl_and_loggnl(std::wstring_view message) { write(message, OutputRoute::component_and_main, false); }
        void logg_and_print(std::wstring_view message) { write(message, OutputRoute::component_main_and_console); }
        void loggnl_and_printnl(std::wstring_view message) { write(message, OutputRoute::component_main_and_console, false); }
        void print(std::wstring_view message) { write(message, OutputRoute::component_main_and_console); }
        void printnl(std::wstring_view message) { write(message, OutputRoute::component_main_and_console, false); }

        template<typename Character>
            requires (std::same_as<Character, char> || std::same_as<Character, wchar_t>)
        void logg(Character message) { logg(std::basic_string_view<Character> {&message, 1}); }

        template<typename Character>
            requires (std::same_as<Character, char> || std::same_as<Character, wchar_t>)
        void loggnl(Character message) { loggnl(std::basic_string_view<Character> {&message, 1}); }

        template<typename Character>
            requires (std::same_as<Character, char> || std::same_as<Character, wchar_t>)
        void print(Character message) { print(std::basic_string_view<Character> {&message, 1}); }

        template<typename Character>
            requires (std::same_as<Character, char> || std::same_as<Character, wchar_t>)
        void printnl(Character message) { printnl(std::basic_string_view<Character> {&message, 1}); }

        template<typename... Args>
        void logg(const char* format_string, Args&&... args) {
            write_formatted(OutputRoute::component, true, format_string, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void loggnl(const char* format_string, Args&&... args) {
            write_formatted(OutputRoute::component, false, format_string, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void logg_and_logg(const char* format_string, Args&&... args) {
            write_formatted(OutputRoute::component_and_main, true, format_string, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void loggnl_and_loggnl(const char* format_string, Args&&... args) {
            write_formatted(OutputRoute::component_and_main, false, format_string, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void logg_and_print(const char* format_string, Args&&... args) {
            write_formatted(OutputRoute::component_main_and_console, true, format_string, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void loggnl_and_printnl(const char* format_string, Args&&... args) {
            write_formatted(OutputRoute::component_main_and_console, false, format_string, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void print(const char* format_string, Args&&... args) {
            write_formatted(OutputRoute::component_main_and_console, true, format_string, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void printnl(const char* format_string, Args&&... args) {
            write_formatted(OutputRoute::component_main_and_console, false, format_string, std::forward<Args>(args)...);
        }
        /** \} */

        /**
         * \brief Prints and inserts text, leaving it on the clipboard.
         *
         * This default operation never prompts. Use the explicitly named
         * preserving variant when previous clipboard contents should survive.
         */
        AC_API void print_and_insert(std::string_view message);
        AC_API void print_and_insert(std::wstring_view message);

        /**
         * \brief Replaces the clipboard with text and sends Ctrl+V.
         *
         * Failures are reported through all component output destinations.
         */
        AC_API void insert_text_replacing_clipboard(
            std::string_view message
        );
        AC_API void insert_text_replacing_clipboard(
            std::wstring_view message
        );
        /**
         * \brief Inserts text and restores a useful text representation of the
         * previous clipboard contents.
         *
         * Empty input is replaced by a newline. Legacy text is normalized to
         * Unicode, Explorer file objects become newline-separated paths, and
         * unsupported data is replaced by a newline with a diagnostic.
         */
        AC_API void insert_text_preserving_clipboard_text(
            std::string_view message
        );
        AC_API void insert_text_preserving_clipboard_text(
            std::wstring_view message
        );
        /**
         * \brief Inserts text into a previously captured foreground window.
         *
         * Use this overload when a console prompt temporarily owns focus. The
         * target remains authoritative through any clipboard warning prompt.
         */
        AC_API void insert_text_preserving_clipboard_text(
            void* target_window,
            std::wstring_view message
        );

        /**
         * \brief Reads Unicode clipboard text and reports failures.
         * \return The text, or no value after an error is reported.
         */
        [[nodiscard]]
        AC_API std::optional<std::wstring> get_clipboard_text();

        /**
         * \name Combined output and insertion methods
         *
         * These methods first write through `print` or `printnl`, then insert
         * using the explicitly named replacement or preservation policy.
         * \{
         */
        AC_API void print_and_insert_text_replacing_clipboard(
            std::string_view message
        );
        AC_API void print_and_insert_text_replacing_clipboard(
            std::wstring_view message
        );
        AC_API void printnl_and_insert_text_replacing_clipboard(
            std::string_view message
        );
        AC_API void printnl_and_insert_text_replacing_clipboard(
            std::wstring_view message
        );
        AC_API void print_and_insert_text_preserving_clipboard_text(
            std::string_view message
        );
        AC_API void print_and_insert_text_preserving_clipboard_text(
            std::wstring_view message
        );
        AC_API void printnl_and_insert_text_preserving_clipboard_text(
            std::string_view message
        );
        AC_API void printnl_and_insert_text_preserving_clipboard_text(
            std::wstring_view message
        );
        /** \} */

        template<typename... Args>
        void print_and_insert(const char* format_string, Args&&... args) {
            if (auto message = format_message(format_string, std::forward<Args>(args)...)) {
                print_and_insert(*message);
            }
        }

        /** \brief Rolls the local component log to today's file if needed. */
        AC_API void update_log_file();
        /** \brief Flushes buffered local component-log output. */
        AC_API void flush();

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;

        template<typename... Args>
        std::optional<std::string> format_message(const char* format_string, Args&&... args) {
            try {
                return ac::formatting::format(format_string, std::forward<Args>(args)...);
            }
            catch (const std::exception& exception) {
                write(std::string("Component format error: ") + exception.what(),
                    OutputRoute::component_main_and_console);
                return std::nullopt;
            }
        }

        template<typename... Args>
        void write_formatted(OutputRoute route, bool newline,
            const char* format_string, Args&&... args) {
            if (auto message = format_message(format_string, std::forward<Args>(args)...)) {
                write(*message, route, newline);
            }
        }

        void report_error(std::string_view message);
    };
} // namespace ac
