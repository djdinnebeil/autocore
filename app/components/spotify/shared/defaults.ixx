/**
 * \file defaults.ixx
 * \brief Portable defaults for `config/spotify.ini`.
 */
export module spotify_defaults;

import std;

export namespace spotify::defaults {

    constexpr std::string_view directory = "components\\spotify";

    constexpr std::string_view ini_text =
        "[spotify]\n"
        "directory = components\\spotify\n"
        "auto_launch_oauth = off\n"
        "logging = on\n";

    /**
     * \brief `on` and `true` enable automatic OAuth. `off` and `false`
     * disable it. Any other value, including empty, keeps the compiled
     * default `off`.
     */
    [[nodiscard]]
    inline bool auto_launch_enabled(std::string_view value) noexcept {
        if (value == "on") {
            return true;
        }
        if (value == "off") {
            return false;
        }
        return false;
    }

    /**
     * \brief Blank, `y`, or `Y` accepts. `n` or `N` declines.
     * Any other answer asks again.
     */
    [[nodiscard]]
    inline std::optional<bool> accepted_yes_no(std::string_view answer) noexcept {
        if (answer.empty() || answer == "y" || answer == "Y") {
            return true;
        }
        if (answer == "n" || answer == "N") {
            return false;
        }
        return std::nullopt;
    }

    enum class AuthorizationCondition {
        usable_access_token,
        refreshable,
        warning_only,
        transient_failure,
        interactive_required
    };

    /**
     * \brief True only for interactive reauthorization when the setting
     * is on and this failure has not already launched OAuth.
     */
    [[nodiscard]]
    inline bool should_auto_launch_oauth(
        AuthorizationCondition condition,
        bool setting_on,
        bool already_attempted
    ) noexcept {
        return condition == AuthorizationCondition::interactive_required
            && setting_on
            && !already_attempted;
    }

    [[nodiscard]]
    inline std::string ini_for(
        std::string_view stored_directory,
        bool auto_launch_oauth_on = false,
        const bool logging = true
    ) {
        const std::string value =
            stored_directory.empty()
                ? std::string {directory}
                : std::string {stored_directory};
        if (value == directory && !auto_launch_oauth_on && logging) {
            return std::string {ini_text};
        }
        return std::string {
            "[spotify]\n"
            "directory = "
        } + value +
            "\nauto_launch_oauth = " +
            (auto_launch_oauth_on ? "on" : "off") +
            "\nlogging = " +
            (logging ? "on" : "off") +
            "\n";
    }

} // namespace spotify::defaults
