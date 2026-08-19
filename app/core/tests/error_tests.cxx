#include "catch_amalgamated.hpp"

#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>

import auto_core.error;
import auto_core.paths;

namespace {

    class ErrorLogGuard {
    public:
        ErrorLogGuard()
            : directory_(ac::paths::error_log_directory()),
              file_(directory_ / "errors.log"),
              directory_existed_(std::filesystem::exists(directory_)),
              file_existed_(std::filesystem::exists(file_)),
              original_size_(
                  file_existed_ ? std::filesystem::file_size(file_) : 0
              ) {
        }

        ~ErrorLogGuard() {
            try {
                if (file_existed_) {
                    std::filesystem::resize_file(file_, original_size_);
                }
                else {
                    std::filesystem::remove(file_);
                }

                if (!directory_existed_) {
                    std::filesystem::remove(directory_);
                }
            }
            catch (...) {
            }
        }

        [[nodiscard]]
        std::string appended_text() const {
            std::ifstream stream(file_, std::ios::binary);
            stream.seekg(static_cast<std::streamoff>(original_size_));

            return {
                std::istreambuf_iterator<char> {stream},
                std::istreambuf_iterator<char> {}
            };
        }

    private:
        std::filesystem::path directory_;
        std::filesystem::path file_;
        bool directory_existed_;
        bool file_existed_;
        std::uintmax_t original_size_;
    };

    class StderrCapture {
    public:
        StderrCapture()
            : previous_(std::cerr.rdbuf(stream_.rdbuf())) {
        }

        ~StderrCapture() {
            std::cerr.rdbuf(previous_);
        }

        [[nodiscard]]
        std::string text() const {
            return stream_.str();
        }

    private:
        std::ostringstream stream_;
        std::streambuf* previous_;
    };

} // namespace

TEST_CASE(
    "Error logging writes to stderr and the timestamped error log",
    "[error][windows-integration]"
) {
    ErrorLogGuard error_log;
    StderrCapture stderr_capture;

    ac::error::log(std::string_view {"Auto Core error test"});

    CHECK(stderr_capture.text() == "Auto Core error test\n");
    CHECK(std::regex_match(
        error_log.appended_text(),
        std::regex {
            R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2} \| Auto Core error test\r?\n$)"
        }
    ));
}

TEST_CASE(
    "Error logging formats arguments before reporting",
    "[error][windows-integration]"
) {
    ErrorLogGuard error_log;
    StderrCapture stderr_capture;

    ac::error::log("Error {}: {}", 42, "details");

    CHECK(stderr_capture.text() == "Error 42: details\n");
    CHECK(error_log.appended_text().ends_with(" | Error 42: details\r\n"));
}

TEST_CASE(
    "Error logging reports invalid format strings",
    "[error][windows-integration]"
) {
    ErrorLogGuard error_log;
    StderrCapture stderr_capture;

    ac::error::log("{} {}", 1);

    CHECK(stderr_capture.text().starts_with(
        "Error message formatting failed: "
    ));
    CHECK(error_log.appended_text().find(
        " | Error message formatting failed: "
    ) != std::string::npos);
}

TEST_CASE(
    "Error logging reports a null format string",
    "[error][windows-integration]"
) {
    ErrorLogGuard error_log;
    StderrCapture stderr_capture;

    ac::error::log(static_cast<const char*>(nullptr), 1);

    CHECK(stderr_capture.text() ==
        "Error message invalid argument: Format string is null\n");
    CHECK(error_log.appended_text().ends_with(
        " | Error message invalid argument: Format string is null\r\n"
    ));
}
