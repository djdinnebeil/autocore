module auto_core.component;

import :console_writer;
import std;

namespace ac::component_detail {

    void ConsoleWriter::write(
        const std::string_view message,
        const bool newline
    ) {
        write_to(std::cout, message, newline);
    }

    void ConsoleWriter::write_error(
        const std::string_view message,
        const bool newline
    ) {
        write_to(std::cerr, message, newline);
    }

    void ConsoleWriter::write_to(
        std::ostream& stream,
        const std::string_view message,
        const bool newline
    ) {
        std::scoped_lock lock(mutex_);
        stream << message;

        if (newline) {
            stream << '\n';
        }

        stream.flush();
    }

}
