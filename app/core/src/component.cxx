module;

#include "console_route_detail.hpp"

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

namespace ac {

    namespace {
        std::atomic_bool disabled_notice_emitted {false};

        struct OwnedScope {
            std::string name;
            ac::logging::config::LoggingFallback fallback;
        };
    }

    class Component::Impl {
    public:
        explicit Impl(const std::string_view component_name)
            : session_start(ac::clock::get_local_datetime()),
              name(component_name) {
        }

        Impl(
            const std::string_view component_name,
            const ac::logging::config::LoggingScope scope
        )
            : session_start(ac::clock::get_local_datetime()),
              name(component_name),
              scope(OwnedScope {std::string {scope.name}, scope.fallback}) {
        }

        const ac::clock::DateTime session_start;
        std::string name;
        std::optional<OwnedScope> scope;
        std::mutex routing_mutex;
        std::unique_ptr<ac::component_detail::ComponentLogger> component_logger;
        ac::component_detail::ConsoleWriter console_writer;
        ac::component_detail::TextInserter text_inserter;

        [[nodiscard]]
        bool component_logging_on() const {
            if (ac::logging::config::disable_all()) {
                return false;
            }
            if (!scope) {
                return ac::logging::config::component_logging_default();
            }
            return ac::logging::config::component_logging_enabled({
                scope->name,
                scope->fallback
            });
        }

        [[nodiscard]]
        bool file_logging_on() const {
            return component_logging_on() &&
                ac::logging::config::write_logs_to_files();
        }

        void ensure_file_logger() {
            if (component_logger) {
                return;
            }
            component_logger = std::make_unique<ac::component_detail::ComponentLogger>(
                name,
                ac::logging::config::components_directory() / name,
                session_start
            );
        }

        void emit_disabled_notice_if_needed() {
            if (ac::logging::config::disable_all() || component_logging_on()) {
                return;
            }
            if (disabled_notice_emitted.exchange(true)) {
                return;
            }
            const std::string notice_name = scope ? scope->name : name;
            console_writer.write(
                "Logging is disabled for " + notice_name + ".",
                true
            );
        }

        void write_message(
            const std::string_view message,
            const bool newline,
            const OutputRoute route
        ) {
            emit_disabled_notice_if_needed();

            const bool log_print =
                route == OutputRoute::component_main_and_console;
            const bool user_facing =
                route == OutputRoute::user_facing ||
                (log_print &&
                    ac::logging::config::log_print_mode() ==
                        ac::logging::config::LogPrintMode::print);
            const bool main_subset =
                route != OutputRoute::component;

            const auto decision = ac::component_detail::decide_sinks({
                .disable_all = ac::logging::config::disable_all(),
                .component_logging = component_logging_on(),
                .write_logs_to_files = ac::logging::config::write_logs_to_files(),
                .write_logs_to_console =
                    ac::logging::config::write_logs_to_console(),
                .user_facing = user_facing,
                .main_subset = main_subset,
                .notice_already_emitted = true
            });

            const auto event_time = ac::clock::get_local_datetime();
            const std::string timestamp =
                ac::clock::format_log_timestamp(event_time);

            std::scoped_lock lock(routing_mutex);
            if (decision.write_files) {
                ensure_file_logger();
                component_logger->write(
                    timestamp,
                    event_time.date_iso,
                    message,
                    decision.write_main,
                    newline
                );
            }
            if (decision.write_console) {
                console_writer.write(message, newline);
            }
        }
    };

    Component::Component(const std::string_view name)
        : impl_(std::make_unique<Impl>(name)) {
    }

    Component::Component(
        const std::string_view name,
        const ac::logging::config::LoggingScope scope
    )
        : impl_(std::make_unique<Impl>(name, scope)) {
    }

    Component::~Component() noexcept = default;

    const clock::DateTime& Component::session_start() const noexcept {
        return impl_->session_start;
    }

    std::string_view Component::name() const noexcept {
        return impl_->name;
    }

    void Component::report_ini_unavailable(const bool malformed) {
        const auto& component_name = impl_->name;
        const auto file = std::string {"config/"} + component_name + ".ini";
        const auto exe = component_name + "_config.exe";
        if (malformed) {
            log_print(
                "{} is malformed. Run {} to generate a valid file. "
                "Using built-in defaults; the file will not be created.",
                file,
                exe
            );
            return;
        }
        log_print(
            "{} is missing. Run {} to generate it. "
            "Using built-in defaults; the file will not be created.",
            file,
            exe
        );
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
        write(message, OutputRoute::user_facing);
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

    void Component::update_log_file() {
        impl_->emit_disabled_notice_if_needed();
        if (!impl_->file_logging_on()) {
            return;
        }
        std::scoped_lock lock(impl_->routing_mutex);
        impl_->ensure_file_logger();
        impl_->component_logger->update_file();
    }

    void Component::flush() {
        std::scoped_lock lock(impl_->routing_mutex);
        if (!impl_->component_logger) {
            return;
        }
        impl_->component_logger->flush();
    }

}
