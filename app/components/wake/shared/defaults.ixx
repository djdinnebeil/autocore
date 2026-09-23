/**
 * \file defaults.ixx
 * \brief Portable defaults for `config/wake.ini`.
 *
 * Wake has no tunables yet. The file exists so the shared config contract
 * applies; runtime behavior is unchanged.
 */
export module wake_defaults;

import std;

export namespace wake::defaults {

    constexpr std::string_view ini_text = "[wake]\n";

} // namespace wake::defaults
