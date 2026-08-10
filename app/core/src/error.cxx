module error;

import std;
import paths;
import <Windows.h>;

namespace ac::error {

    void log(std::string_view message) {
        bool written_to_file = false;

        try {
            const auto directory =
                ac::paths::error_log_directory();

            std::filesystem::create_directories(directory);

            std::ofstream stream(
                directory / "errors.log",
                std::ios::app
            );

            if (stream.is_open()) {
                SYSTEMTIME time {};
                GetLocalTime(&time);

                stream
                    << std::setfill('0')
                    << std::setw(4) << time.wYear << '-'
                    << std::setw(2) << time.wMonth << '-'
                    << std::setw(2) << time.wDay << ' '
                    << std::setw(2) << time.wHour << ':'
                    << std::setw(2) << time.wMinute << ':'
                    << std::setw(2) << time.wSecond
                    << " | "
                    << message
                    << '\n';

                stream.flush();

                written_to_file = stream.good();
            }
        }
        catch (...) {
        }

        if (!written_to_file) {
            std::cerr << "[error log unavailable] ";
        }

        std::cerr << message << '\n';
    }
}
