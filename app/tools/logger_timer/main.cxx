/**
 * \file main.cxx
 * \brief Compares 20 separate flushed log writes with one combined write.
 */

#include <array>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace {

    constexpr int entry_count = 20;
    constexpr int test_iterations = 1'000;

    std::ofstream log_stream;

    void logg(const std::string& message) {
        log_stream << message << std::endl;
    }

    std::array<std::string, entry_count> create_entries() {
        std::array<std::string, entry_count> entries;

        for (int index = 0; index < entry_count; ++index) {
            entries[index] =
                "keymap entry " + std::to_string(index) +
                " = primary_action, secondary_action";
        }

        return entries;
    }

    std::string merge_entries(
        const std::array<std::string, entry_count>& entries
    ) {
        std::ostringstream buffer;

        for (std::size_t index = 0; index < entries.size(); ++index) {
            if (index != 0) {
                buffer << '\n';
            }

            buffer << entries[index];
        }

        return buffer.str();
    }

    std::chrono::nanoseconds test_separate_calls(
        const std::array<std::string, entry_count>& entries
    ) {
        const auto start = std::chrono::steady_clock::now();

        for (const std::string& entry : entries) {
            logg(entry);
        }

        const auto end = std::chrono::steady_clock::now();

        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            end - start
        );
    }

    std::chrono::nanoseconds test_combined_call(
        const std::string& combined_entries
    ) {
        const auto start = std::chrono::steady_clock::now();

        logg(combined_entries);

        const auto end = std::chrono::steady_clock::now();

        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            end - start
        );
    }

    double to_microseconds(
        const std::chrono::nanoseconds duration
    ) {
        return static_cast<double>(duration.count()) / 1'000.0;
    }

}

int main() {
    log_stream.open(
        "logger_benchmark.log",
        std::ios::out | std::ios::trunc
    );

    if (!log_stream.is_open()) {
        std::cerr << "Could not open logger_benchmark.log\n";
        return 1;
    }

    const std::array<std::string, entry_count> entries =
        create_entries();

    const std::string combined_entries =
        merge_entries(entries);

    std::chrono::nanoseconds separate_total {0};
    std::chrono::nanoseconds combined_total {0};

    // Warm-up.
    test_separate_calls(entries);
    test_combined_call(combined_entries);

    for (int iteration = 0;
        iteration < test_iterations;
        ++iteration) {
        /*
         * Alternate the order so one test is not always favored by running
         * first.
         */
        if (iteration % 2 == 0) {
            separate_total += test_separate_calls(entries);
            combined_total += test_combined_call(combined_entries);
        }
        else {
            combined_total += test_combined_call(combined_entries);
            separate_total += test_separate_calls(entries);
        }
    }

    log_stream.close();

    const double separate_average =
        to_microseconds(separate_total) / test_iterations;

    const double combined_average =
        to_microseconds(combined_total) / test_iterations;

    std::cout << std::fixed << std::setprecision(3);

    std::cout
        << "Test iterations: " << test_iterations << '\n'
        << "Entries per test: " << entry_count << "\n\n"
        << "20 separate logg() calls:\n"
        << "  Average: " << separate_average
        << " microseconds\n\n"
        << "1 combined logg() call:\n"
        << "  Average: " << combined_average
        << " microseconds\n\n";

    if (combined_average > 0.0) {
        std::cout
            << "Separate / combined ratio: "
            << separate_average / combined_average
            << "x\n";
    }

    std::cin.get();

    return 0;
}