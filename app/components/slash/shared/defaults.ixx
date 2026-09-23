/**
 * \file defaults.ixx
 * \brief Portable defaults for `config/slash.ini`.
 *
 * Slash has no tunables yet. The file exists so the shared config contract
 * applies; recycle-bin behavior is unchanged.
 */
export module slash_defaults;

import std;

export namespace slash::defaults {

    constexpr std::string_view ini_text = "[slash]\n";

} // namespace slash::defaults
