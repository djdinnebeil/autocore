/**
 * \file component.ixx
 * \brief Defines component-scoped output for Auto Core.
 */
module;

#include "ac_api.hpp"

export module auto_core.core.component;

import std;
import auto_core.core.clock;
import auto_core.core.formatting;
import auto_core.core.logging.config;

export namespace ac {
    /**
     * \brief Classifies one component message before policy filtering.
     *
     * `trace`, `debug`, `info`, and `alert` are diagnostic severities.
     * `print` is ordinary console output and is not a diagnostic severity.
     */
    enum class MessageClass : unsigned char {
        trace = 0,
        debug = 1,
        info = 2,
        alert = 3,
        print = 4
    };

    /**
     * \brief Coordinates local logging, console output, and
     * clipboard-based text insertion for one named component.
     *
     * Output routing is synchronized per component. Narrow text is UTF-8;
     * wide text is converted to UTF-8 before logging or console output.
     */
    class Component {
    public:
        /**
         * \brief Starts a named component session.
         *
         * Construction does not read logging configuration or open log
         * files. The first enabled file write opens `components/<name>/`.
         * \param name The component name used for `components/<name>/` log
         * directories and events.
         */
        AC_API explicit Component(std::string_view name);

        /**
         * \brief Starts a named session with an explicit logging scope.
         *
         * `name` is the log identity. `scope` selects the family INI and
         * fallback. Absence of this overload means no family INI lookup.
         */
        AC_API Component(
            std::string_view name,
            ac::logging::config::LoggingScope scope
        );
        /** \brief Closes the local component logs. */
        AC_API ~Component() noexcept;

        /**
         * \brief Returns the component name passed to the constructor.
         */
        [[nodiscard]] AC_API std::string_view name() const noexcept;

        /**
         * \brief Reports a missing or malformed `config/<name>.ini`.
         *
         * Uses `log_alert`. Directs the user to `<name>_config.exe`.
         * States that built-in defaults are in use and the file will not be
         * created. Does not write the INI.
         *
         * \param malformed True when the file exists but cannot be used.
         */
        AC_API void report_ini_unavailable(bool malformed = false);

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
         * \brief Writes UTF-8 text to the selected destinations.
         * \param message The message bytes.
         * \param message_class The diagnostic or print classification.
         * \param newline Whether to append a newline at each destination.
         */
        AC_API void write(std::string_view message, MessageClass message_class,
            bool newline = true);
        /** \brief Converts Unicode text to UTF-8 and routes it as `write()`. */
        AC_API void write(std::wstring_view message, MessageClass message_class,
            bool newline = true);

        /**
         * \name Output convenience methods
         *
         * `log_debug`, `log_trace`, `log_info`, and `log_alert` are
         * diagnostics. `trace` is the most detailed severity. `print` stays
         * on the console. Its file copy requires
         * `log_console_output` and the `write_logs_to_files` file gate.
         * Names containing `nl` suppress the
         * trailing newline. Narrow, wide, single-character, and formatted
         * UTF-8 overloads are provided. Formatting failures use `print`.
         * \{
         */
        void log_debug(std::string_view message) { write(message, MessageClass::debug); }
        void lognl_debug(std::string_view message) { write(message, MessageClass::debug, false); }
        void log_trace(std::string_view message) { write(message, MessageClass::trace); }
        void lognl_trace(std::string_view message) { write(message, MessageClass::trace, false); }
        void log_info(std::string_view message) { write(message, MessageClass::info); }
        void lognl_info(std::string_view message) { write(message, MessageClass::info, false); }
        void log_alert(std::string_view message) { write(message, MessageClass::alert); }
        void lognl_alert(std::string_view message) { write(message, MessageClass::alert, false); }
        void print(std::string_view message) { write(message, MessageClass::print); }
        void printnl(std::string_view message) { write(message, MessageClass::print, false); }

        void log_debug(std::wstring_view message) { write(message, MessageClass::debug); }
        void lognl_debug(std::wstring_view message) { write(message, MessageClass::debug, false); }
        void log_trace(std::wstring_view message) { write(message, MessageClass::trace); }
        void lognl_trace(std::wstring_view message) { write(message, MessageClass::trace, false); }
        void log_info(std::wstring_view message) { write(message, MessageClass::info); }
        void lognl_info(std::wstring_view message) { write(message, MessageClass::info, false); }
        void log_alert(std::wstring_view message) { write(message, MessageClass::alert); }
        void lognl_alert(std::wstring_view message) { write(message, MessageClass::alert, false); }
        void print(std::wstring_view message) { write(message, MessageClass::print); }
        void printnl(std::wstring_view message) { write(message, MessageClass::print, false); }

        template<typename Character>
            requires (std::same_as<Character, char> || std::same_as<Character, wchar_t>)
        void log_debug(Character message) { log_debug(std::basic_string_view<Character> {&message, 1}); }

        template<typename Character>
            requires (std::same_as<Character, char> || std::same_as<Character, wchar_t>)
        void lognl_debug(Character message) { lognl_debug(std::basic_string_view<Character> {&message, 1}); }

        template<typename Character>
            requires (std::same_as<Character, char> || std::same_as<Character, wchar_t>)
        void log_trace(Character message) { log_trace(std::basic_string_view<Character> {&message, 1}); }

        template<typename Character>
            requires (std::same_as<Character, char> || std::same_as<Character, wchar_t>)
        void lognl_trace(Character message) { lognl_trace(std::basic_string_view<Character> {&message, 1}); }

        template<typename Character>
            requires (std::same_as<Character, char> || std::same_as<Character, wchar_t>)
        void print(Character message) { print(std::basic_string_view<Character> {&message, 1}); }

        template<typename Character>
            requires (std::same_as<Character, char> || std::same_as<Character, wchar_t>)
        void printnl(Character message) { printnl(std::basic_string_view<Character> {&message, 1}); }

        template<typename... Args>
        void log_debug(const char* format_string, Args&&... args) {
            write_formatted(MessageClass::debug, true, format_string, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void lognl_debug(const char* format_string, Args&&... args) {
            write_formatted(MessageClass::debug, false, format_string, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void log_trace(const char* format_string, Args&&... args) {
            write_formatted(MessageClass::trace, true, format_string, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void lognl_trace(const char* format_string, Args&&... args) {
            write_formatted(MessageClass::trace, false, format_string, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void log_info(const char* format_string, Args&&... args) {
            write_formatted(MessageClass::info, true, format_string, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void lognl_info(const char* format_string, Args&&... args) {
            write_formatted(MessageClass::info, false, format_string, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void log_alert(const char* format_string, Args&&... args) {
            write_formatted(MessageClass::alert, true, format_string, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void lognl_alert(const char* format_string, Args&&... args) {
            write_formatted(MessageClass::alert, false, format_string, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void print(const char* format_string, Args&&... args) {
            write_formatted(MessageClass::print, true, format_string, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void printnl(const char* format_string, Args&&... args) {
            write_formatted(MessageClass::print, false, format_string, std::forward<Args>(args)...);
        }
        /** \} */

        /**
         * \brief Prints and inserts text, then sends Shift+Enter once.
         *
         * This default operation never prompts. Use the explicitly named
         * preserving variant when previous clipboard contents should survive.
         * Trailing `\n` characters are removed before output and insertion.
         * Shift+Enter is sent separately so plain-text, rich-text, and chat
         * controls advance by one line without submitting the field.
         */
        AC_API void print_and_insert(std::string_view message);
        AC_API void print_and_insert(std::wstring_view message);

        /**
         * \brief Replaces the clipboard with text, then sends Shift+Enter.
         *
         * Trailing `\n` characters are removed before insertion.
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
         * Trailing `\n` characters are removed and Shift+Enter is sent after
         * the paste. Legacy text is normalized to Unicode, Explorer file
         * objects become newline-separated paths, and unsupported data is
         * replaced by a newline with a diagnostic.
         */
        AC_API void insert_text_preserving_clipboard_text(
            std::string_view message
        );
        AC_API void insert_text_preserving_clipboard_text(
            std::wstring_view message
        );
        /**
         * \brief Inserts text into a previously captured console window.
         *
         * `target_window` is an `ac::console::WindowHandle` (`void*` so this
         * module does not import the console API). Use this overload when a
         * console prompt temporarily owns focus. The target remains
         * authoritative through any clipboard warning prompt. Activation
         * failure skips insertion and leaves the clipboard unchanged.
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
         * Trailing newlines stay in the printed text; insert still strips
         * them. `print_and_insert` strips trailing newlines before both.
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
                    MessageClass::print);
                return std::nullopt;
            }
        }

        template<typename... Args>
        void write_formatted(MessageClass message_class, bool newline,
            const char* format_string, Args&&... args) {
            if (auto message = format_message(format_string, std::forward<Args>(args)...)) {
                write(*message, message_class, newline);
            }
        }

        void report_error(std::string_view message);
    };
} // namespace ac
