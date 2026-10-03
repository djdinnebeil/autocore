/**
 * \file logger_init_detail.ixx
 * \brief Menu and owner launches for logger_init.exe.
 *
 * This module does not write configuration files. Each choice names the
 * configuration executable and the argument that executable already owns.
 */
export module logger_init_detail;

import std;

export namespace ac::main::logger_init {

    enum class Choice {
        use_defaults,
        configure,
        disable_logging
    };

    struct OwnerLaunch {
        std::string_view executable;
        std::wstring_view arguments;
    };

    inline constexpr std::array<OwnerLaunch, 2> default_launches {{
        {"logging_config.exe", L"--seed"},
        {"logger_config.exe", L"--seed"}
    }};

    inline constexpr std::array<OwnerLaunch, 2> configure_launches {{
        {"logging_config.exe", L"--init"},
        {"logger_config.exe", L"--init"}
    }};

    inline constexpr std::array<OwnerLaunch, 2> disable_launches {{
        {"logging_config.exe", L"--disable"},
        {"logger_config.exe", L"--seed"}
    }};

    [[nodiscard]]
    constexpr std::span<const OwnerLaunch> launches_for(const Choice choice) noexcept {
        switch (choice) {
        case Choice::use_defaults:
            return default_launches;
        case Choice::configure:
            return configure_launches;
        case Choice::disable_logging:
            return disable_launches;
        }
        return default_launches;
    }

    /**
     * \brief Reads the Logger menu.
     *
     * Invalid input is asked again. End of input cancels.
     */
    [[nodiscard]]
    std::optional<Choice> prompt_menu(
        std::istream& input,
        std::ostream& output
    ) {
        while (true) {
            output
                << "Logger\n"
                << "\n"
                << "1. Use defaults\n"
                << "2. Configure\n"
                << "3. Disable logging\n"
                << "Choice: ";
            output.flush();

            std::string line;
            if (!std::getline(input, line)) {
                return std::nullopt;
            }
            const auto first = line.find_first_not_of(" \t");
            const auto choice = first == std::string::npos
                ? std::string {}
                : line.substr(first, line.find_last_not_of(" \t") - first + 1);
            if (choice == "1") {
                return Choice::use_defaults;
            }
            if (choice == "2") {
                return Choice::configure;
            }
            if (choice == "3") {
                return Choice::disable_logging;
            }
            output << "Enter 1, 2, or 3.\n";
        }
    }

} // namespace ac::main::logger_init
