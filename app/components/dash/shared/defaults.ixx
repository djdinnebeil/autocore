/**
 * \file defaults.ixx
 * \brief Portable defaults for `config/dash.ini`.
 *
 * The vault stays under `%LOCALAPPDATA%\Auto Core`. This INI exists so the
 * shared config contract applies; Dash has no dist/config tunables yet.
 */
export module dash_defaults;

import std;

export namespace dash::defaults {

    constexpr std::string_view ini_text = "[dash]\n";

} // namespace dash::defaults
