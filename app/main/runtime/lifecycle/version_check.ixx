/**
 * \file version_check.ixx
 * \brief One startup comparison against the published Auto Core version.
 */
export module auto_core.main.version_check;

import std;

/**
 * Reads `config/auto_core.ini` and, when update checking is on, starts one
 * background request. The returned thread is not joinable when checking is off.
 * Join it after the message loop.
 */
export [[nodiscard]] std::jthread start_version_check();
