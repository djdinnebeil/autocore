/**
 * \file error.ixx
 * \brief Provides best-effort error reporting to a log file and stderr.
 *
 * Messages are always written to `std::cerr`. The module also attempts to
 * append timestamped records to `errors/errors.log` beneath the executable
 * directory. Failures while creating or writing the file are suppressed so
 * error reporting can continue through stderr.
 */
module;

#include "ac_api.hpp"

export module auto_core.error;

import std;
import auto_core.formatting;

export namespace ac::error {

    /**
     * \brief Reports an error message without formatting it.
     *
     * The message is written verbatim to `std::cerr` followed by a newline.
     * A record in `YYYY-MM-DD HH:MM:SS | message` form is also appended to
     * `errors/errors.log` when the file is available. If file logging fails,
     * stderr is prefixed with `[error log unavailable] `.
     *
     * This function creates the `errors` directory when necessary.
     * Filesystem and file-stream failures are suppressed.
     *
     * \param message The message to report.
     */
    AC_API void log(std::string_view message);

    /**
     * \brief Formats and reports an error message.
     *
     * The format string and arguments follow `auto_core.formatting`. A null
     * format string or invalid format expression is reported as a diagnostic
     * error message instead of being propagated.
     *
     * Conversion failures and other exceptions not caused by an invalid
     * format string propagate to the caller.
     *
     * \param format_string A null-terminated UTF-8 format string.
     * \param args Values referenced by the replacement fields.
     */
    template<typename... Args>
    void log(
        const char* format_string,
        Args&&... args
    ) {
        try {
            const std::string message = ac::formatting::format(
                format_string,
                std::forward<Args>(args)...
            );

            log(message);
        }
        catch (const std::format_error& exception) {
            log(
                std::string("Error message formatting failed: ") +
                exception.what()
            );
        }
        catch (const std::invalid_argument& exception) {
            log(
                std::string("Error message invalid argument: ") +
                exception.what()
            );
        }
    }

} // namespace ac::error

