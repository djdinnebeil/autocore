module auto_core.core.component;

import :console_writer;
import :logger;
import :text_inserter;
import std;
import auto_core.core.clipboard;
import auto_core.core.clock;
import auto_core.core.console;
import auto_core.core.encoding;
import auto_core.core.logging.config;
import auto_core.core.logging.client;
import auto_core.core.logging.protocol;

namespace ac {

    class Component::Impl {
    public:
        explicit Impl(const std::string_view component_name)
            : session_start(ac::clock::get_local_datetime()),
              name(component_name),
              component_logger(
                  component_name,
                  ac::logging::config::components_directory() / component_name,
                  session_start
              ) {
        }

        const ac::clock::DateTime session_start;
        std::string name;
        std::mutex routing_mutex;
        ac::component_detail::ComponentLogger component_logger;
        ac::component_detail::ConsoleWriter console_writer;
        ac::component_detail::TextInserter text_inserter;
        ac::logger::MainLogConnection main_log_connection;

        void send_to_main_log(
            const std::string_view timestamp,
            const std::string_view message,
            const bool newline
        ) {
            const ac::logging::Event event {
                .timestamp = std::string {timestamp},
                .component = name,
                .message = std::string {message},
                .newline = newline
            };

            (void)main_log_connection.send(event);
        }

        void write_connection_failure(
            const std::string_view message
        ) {
            const auto event_time = ac::clock::get_local_datetime();
            std::scoped_lock lock(routing_mutex);
            component_logger.write(
                ac::clock::format_log_timestamp(event_time),
                event_time.date_iso,
                message,
                false
            );
            console_writer.write_error(message);
        }

        void write_message(
            const std::string_view message,
            const bool newline,
            const OutputRoute route
        ) {
            const auto event_time = ac::clock::get_local_datetime();
            const std::string timestamp =
                ac::clock::format_log_timestamp(event_time);
            const bool main_worthy =
                route == OutputRoute::component_and_main ||
                route == OutputRoute::component_main_and_console;

            std::scoped_lock lock(routing_mutex);
            component_logger.write(
                timestamp,
                event_time.date_iso,
                message,
                main_worthy,
                newline
            );

            if (main_worthy) {
                send_to_main_log(timestamp, message, newline);
            }

            if (route == OutputRoute::component_main_and_console) {
                console_writer.write(message, newline);
            }
        }
    };

    Component::Component(const std::string_view name)
        : impl_(std::make_unique<Impl>(name)) {
    }

    Component::~Component() noexcept {
        impl_->main_log_connection.close();
    }

    const clock::DateTime& Component::session_start() const noexcept {
        return impl_->session_start;
    }

    void Component::write(
        const std::string_view message,
        const OutputRoute route,
        const bool newline
    ) {
        impl_->write_message(message, newline, route);
    }

    void Component::write(
        const std::wstring_view message,
        const OutputRoute route,
        const bool newline
    ) {
        write(ac::encoding::to_utf8(message), route, newline);
    }

    void Component::print_and_insert(const std::string_view message) {
        std::string normalized {message};

        while (
            !normalized.empty() &&
            normalized.back() == '\n'
        ) {
            normalized.pop_back();
        }

        print(normalized);

        try {
            insert_text_replacing_clipboard(
                ac::encoding::to_utf16(normalized)
            );
        }
        catch (const std::exception& exception) {
            report_error(
                std::format(
                    "Unable to convert text for insertion: {}",
                    exception.what()
                )
            );
        }
    }

    void Component::print_and_insert(const std::wstring_view message) {
        std::wstring normalized {message};

        while (
            !normalized.empty() &&
            normalized.back() == L'\n'
        ) {
            normalized.pop_back();
        }

        print(normalized);
        insert_text_replacing_clipboard(normalized);
    }

    void Component::report_error(const std::string_view message) {
        write(message, OutputRoute::component_main_and_console);
    }

    void Component::insert_text_replacing_clipboard(
        const std::wstring_view message
    ) {
        std::wstring normalized {message};

        while (!normalized.empty() && normalized.back() == L'\n') {
            normalized.pop_back();
        }

        const auto result =
            impl_->text_inserter.insert_replacing_clipboard(normalized);

        if (!result) {
            report_error(
                ac::component_detail::error_message(result.error())
            );
        }
    }

    void Component::insert_text_replacing_clipboard(
        const std::string_view message
    ) {
        try {
            insert_text_replacing_clipboard(
                ac::encoding::to_utf16(message)
            );
        }
        catch (const std::exception& exception) {
            report_error(std::format(
                "Unable to convert text for insertion: {}",
                exception.what()
            ));
        }
    }

    void Component::insert_text_preserving_clipboard_text(
        const std::wstring_view message
    ) {
        insert_text_preserving_clipboard_text(nullptr, message);
    }

    void Component::insert_text_preserving_clipboard_text(
        const ac::console::WindowHandle target_window,
        const std::wstring_view message
    ) {
        const auto representation =
            ac::clipboard::capture_clipboard_text_representation();
        std::wstring insertion {message};

        while (!insertion.empty() && insertion.back() == L'\n') {
            insertion.pop_back();
        }
        ac::clipboard::ClipboardTextSnapshot previous_clipboard;

        if (!representation) {
            report_error(std::format(
                "Unable to preserve clipboard text: {}. "
                "A newline will be restored to the clipboard instead.",
                ac::clipboard::error_message(representation.error())
            ));
            previous_clipboard = std::wstring {L"\n"};
        }
        else {
            switch (representation->kind) {
            case ac::clipboard::ClipboardTextKind::unicode_text:
                previous_clipboard = representation->text;
                break;

            case ac::clipboard::ClipboardTextKind::empty:
                break;

            case ac::clipboard::ClipboardTextKind::file_paths:
                report_error(
                    "The copied files or directories were converted to "
                    "full paths because Auto Core cannot yet preserve "
                    "file clipboard objects."
                );
                print(representation->text);
                previous_clipboard = representation->text;
                break;

            case ac::clipboard::ClipboardTextKind::unsupported:
                report_error(
                    "The clipboard contains non-text data Auto Core cannot "
                    "preserve. A newline will be restored to the clipboard "
                    "instead."
                );
                previous_clipboard = std::wstring {L"\n"};
                break;
            }
        }

        if (target_window != nullptr) {
            const auto activated =
                ac::console::activate_window(target_window);

            if (!activated) {
                report_error(ac::console::error_message(activated.error()));
                return;
            }
        }

        const auto result =
            impl_->text_inserter.insert_preserving_clipboard_text(
                insertion,
                previous_clipboard
            );

        if (!result) {
            report_error(
                ac::component_detail::error_message(result.error())
            );
        }
    }

    void Component::insert_text_preserving_clipboard_text(
        const std::string_view message
    ) {
        try {
            insert_text_preserving_clipboard_text(
                ac::encoding::to_utf16(message)
            );
        }
        catch (const std::exception& exception) {
            report_error(std::format(
                "Unable to convert text for insertion: {}",
                exception.what()
            ));
            insert_text_preserving_clipboard_text(L"");
        }
    }

    std::optional<std::wstring> Component::get_clipboard_text() {
        auto result = ac::clipboard::get_clipboard_text();

        if (!result) {
            report_error(std::format(
                "Clipboard error: {}",
                ac::clipboard::error_message(result.error())
            ));
            return std::nullopt;
        }

        return std::move(*result);
    }

    void Component::print_and_insert_text_replacing_clipboard(
        const std::string_view message
    ) {
        print(message);
        insert_text_replacing_clipboard(message);
    }

    void Component::print_and_insert_text_replacing_clipboard(
        const std::wstring_view message
    ) {
        print(message);
        insert_text_replacing_clipboard(message);
    }

    void Component::printnl_and_insert_text_replacing_clipboard(
        const std::string_view message
    ) {
        printnl(message);
        insert_text_replacing_clipboard(message);
    }

    void Component::printnl_and_insert_text_replacing_clipboard(
        const std::wstring_view message
    ) {
        printnl(message);
        insert_text_replacing_clipboard(message);
    }

    void Component::print_and_insert_text_preserving_clipboard_text(
        const std::string_view message
    ) {
        print(message);
        insert_text_preserving_clipboard_text(message);
    }

    void Component::print_and_insert_text_preserving_clipboard_text(
        const std::wstring_view message
    ) {
        print(message);
        insert_text_preserving_clipboard_text(message);
    }

    void Component::printnl_and_insert_text_preserving_clipboard_text(
        const std::string_view message
    ) {
        printnl(message);
        insert_text_preserving_clipboard_text(message);
    }

    void Component::printnl_and_insert_text_preserving_clipboard_text(
        const std::wstring_view message
    ) {
        printnl(message);
        insert_text_preserving_clipboard_text(message);
    }

    void Component::connect_to_logger() {
        if (!ac::logging::config::enabled()) {
            return;
        }

        impl_->main_log_connection.start(
            impl_->name,
            [component = impl_.get()](const std::string_view message) {
                component->write_connection_failure(message);
            }
        );
    }

    bool Component::request_logger_shutdown() {
        return impl_->main_log_connection.request_logger_shutdown();
    }

    void Component::update_log_file() {
        std::scoped_lock lock(impl_->routing_mutex);
        impl_->component_logger.update_file();
    }

    void Component::flush() {
        std::scoped_lock lock(impl_->routing_mutex);
        impl_->component_logger.flush();
    }

}
