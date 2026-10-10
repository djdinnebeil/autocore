/**
 * \file config_menu.ixx
 * \brief Console presentation for `<name>_config.exe` setting menus.
 *
 * The module renders menus and the compact `#` reference header. It does
 * not read or write INI files. Each owner supplies current values, validates
 * input, and persists a change.
 */
export module config_menu;

import std;

export namespace ac::config_menu {

    /**
     * \brief One configurable INI key.
     *
     * `choices` empty means a free-form value. `constraint` is shown when a
     * free-form value is rejected. Closed sets also feed the `#` header.
     */
    struct Setting {
        std::string_view key {};
        std::string_view display_name {};
        std::string_view summary {};
        std::string_view default_value {};
        std::span<const std::string_view> choices {};
        std::string_view constraint {};
    };

} // namespace ac::config_menu

namespace ac::config_menu::detail {

    [[nodiscard]] std::string trim(const std::string_view value) {
        const auto first = value.find_first_not_of(" \t\r");
        if (first == std::string_view::npos) {
            return {};
        }
        const auto last = value.find_last_not_of(" \t\r");
        return std::string {value.substr(first, last - first + 1)};
    }

    [[nodiscard]] std::optional<std::string> read_line(std::istream& input) {
        std::string line;
        if (!std::getline(input, line)) {
            return std::nullopt;
        }
        return trim(line);
    }

    [[nodiscard]] std::size_t name_width(
        const std::span<const ac::config_menu::Setting> settings
    ) {
        std::size_t width = 0;
        for (const auto& setting : settings) {
            width = std::max(width, setting.display_name.size());
        }
        return width;
    }

    void write_padded(
        std::ostream& output,
        const std::string_view name,
        const std::string_view value,
        const std::size_t width
    ) {
        output << name;
        const auto padding = width - name.size() + 2;
        output << std::string(padding, ' ') << value << '\n';
    }

    void write_setting_facts(
        std::ostream& output,
        const ac::config_menu::Setting& setting,
        const std::string_view current
    ) {
        output
            << '\n'
            << setting.display_name
            << "\n\nCurrent: "
            << current
            << "\nDefault: "
            << setting.default_value
            << "\n\n"
            << setting.summary
            << "\n\n";
    }

    void reject(std::ostream& output, const ac::config_menu::Setting& setting) {
        if (!setting.constraint.empty()) {
            output << setting.constraint << '\n';
            return;
        }
        output << "That value is invalid.\n";
    }

} // namespace ac::config_menu::detail

export namespace ac::config_menu {

    enum class ApplyResult {
        stored,
        invalid,
        failed
    };

    enum class OfferResult {
        configure,
        leave,
        failed
    };

    struct MenuResult {
        bool ok {true};
        bool changed {false};
    };

    using CurrentValue = std::function<std::string(const Setting&)>;
    using ApplyValue =
        std::function<ApplyResult(const Setting&, std::string_view)>;

    /**
     * \brief `# key = a | b` lines for closed sets, then a blank line.
     *
     * Free-form settings are omitted. An empty span produces an empty string.
     */
    [[nodiscard]] std::string reference_header(
        const std::span<const Setting> settings
    ) {
        std::string text;
        for (const auto& setting : settings) {
            if (setting.choices.empty()) {
                continue;
            }
            text += "# ";
            text += setting.key;
            text += " = ";
            bool first = true;
            for (const auto choice : setting.choices) {
                if (!first) {
                    text += " | ";
                }
                first = false;
                text += choice;
            }
            text.push_back('\n');
        }
        if (!text.empty()) {
            text.push_back('\n');
        }
        return text;
    }

    [[nodiscard]] std::string with_header(
        const std::span<const Setting> settings,
        const std::string_view body
    ) {
        return reference_header(settings) + std::string {body};
    }

    /**
     * \brief Numbered settings, current values, and `0. Exit`.
     *
     * A valid change is reported through `apply` and the menu is shown again.
     * `0` on a free-form value prompt is passed to `apply` as text.
     */
    [[nodiscard]] MenuResult run_menu(
        const std::string_view title,
        const std::span<const Setting> settings,
        const CurrentValue& current,
        const ApplyValue& apply,
        std::istream& input,
        std::ostream& output
    ) {
        MenuResult result;
        const auto width = detail::name_width(settings);
        while (true) {
            output << '\n' << title << "\n\n";
            for (std::size_t index = 0; index < settings.size(); ++index) {
                output << (index + 1) << ". ";
                detail::write_padded(
                    output,
                    settings[index].display_name,
                    current(settings[index]),
                    width
                );
            }
            output << "\n0. Exit\n";
            output.flush();

            const auto line = detail::read_line(input);
            if (!line) {
                result.ok = false;
                return result;
            }
            if (*line == "0") {
                return result;
            }

            std::size_t selected = 0;
            const auto parsed = std::from_chars(
                line->data(),
                line->data() + line->size(),
                selected
            );
            if (parsed.ec != std::errc {} ||
                parsed.ptr != line->data() + line->size() ||
                selected < 1 ||
                selected > settings.size()) {
                output << "Enter 0-" << settings.size() << ".\n";
                continue;
            }

            const auto& setting = settings[selected - 1];
            if (setting.choices.empty()) {
                while (true) {
                    detail::write_setting_facts(
                        output,
                        setting,
                        current(setting)
                    );
                    output << "1. Change value\n0. Back\n";
                    output.flush();
                    const auto command = detail::read_line(input);
                    if (!command) {
                        result.ok = false;
                        return result;
                    }
                    if (*command == "0") {
                        break;
                    }
                    if (*command != "1") {
                        output << "Enter 0 or 1.\n";
                        continue;
                    }

                    while (true) {
                        output << "New value: ";
                        output.flush();
                        const auto value = detail::read_line(input);
                        if (!value) {
                            result.ok = false;
                            return result;
                        }
                        if (value->empty()) {
                            output << "Enter a value.\n";
                            continue;
                        }
                        const auto applied = apply(setting, *value);
                        if (applied == ApplyResult::failed) {
                            result.ok = false;
                            return result;
                        }
                        if (applied == ApplyResult::invalid) {
                            detail::reject(output, setting);
                            continue;
                        }
                        result.changed = true;
                        break;
                    }
                    break;
                }
                continue;
            }

            while (true) {
                detail::write_setting_facts(output, setting, current(setting));
                for (std::size_t index = 0; index < setting.choices.size(); ++index) {
                    output
                        << (index + 1)
                        << ". "
                        << setting.choices[index]
                        << '\n';
                }
                output << "\n0. Back\n";
                output.flush();
                const auto command = detail::read_line(input);
                if (!command) {
                    result.ok = false;
                    return result;
                }
                if (*command == "0") {
                    break;
                }
                std::size_t choice = 0;
                const auto choice_parsed = std::from_chars(
                    command->data(),
                    command->data() + command->size(),
                    choice
                );
                if (choice_parsed.ec != std::errc {} ||
                    choice_parsed.ptr != command->data() + command->size() ||
                    choice < 1 ||
                    choice > setting.choices.size()) {
                    output << "Enter 0-" << setting.choices.size() << ".\n";
                    continue;
                }
                const auto applied = apply(setting, setting.choices[choice - 1]);
                if (applied == ApplyResult::failed) {
                    result.ok = false;
                    return result;
                }
                if (applied == ApplyResult::invalid) {
                    detail::reject(output, setting);
                    continue;
                }
                result.changed = true;
                break;
            }
        }
    }

    /**
     * \brief Readiness summary, then `1. Configure` / `0. Continue`.
     */
    [[nodiscard]] OfferResult offer_configuration(
        const std::string_view title,
        const std::span<const Setting> settings,
        const CurrentValue& current,
        std::istream& input,
        std::ostream& output
    ) {
        const auto width = detail::name_width(settings);
        while (true) {
            output << '\n' << title << "\n\n";
            for (const auto& setting : settings) {
                detail::write_padded(
                    output,
                    setting.display_name,
                    current(setting),
                    width
                );
            }
            output << "\n1. Configure\n0. Continue\n";
            output.flush();
            const auto line = detail::read_line(input);
            if (!line) {
                return OfferResult::failed;
            }
            if (*line == "0") {
                return OfferResult::leave;
            }
            if (*line == "1") {
                return OfferResult::configure;
            }
            output << "Enter 0 or 1.\n";
        }
    }

} // namespace ac::config_menu
