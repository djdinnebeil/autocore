/**
 * \file app_config.ixx
 * \brief Provides main-application configuration.
 */
export module app_config;

import std;

export namespace app_config {
    /** Returns the configured title, or `Auto Core` when unavailable. */
    [[nodiscard]] std::wstring_view program_title();
}
