/**
 * \file wake_capture.hpp
 * \brief One-shot `powercfg /lastwake` stdout capture.
 */
#pragma once

#include <optional>
#include <string>

namespace wake {

    /**
     * \brief Runs `powercfg.exe /lastwake` and returns its stdout.
     *
     * A launch failure, non-zero exit, or empty stdout is an empty result.
     * This function does not modify Wake history files.
     */
    [[nodiscard]] std::optional<std::string> capture_powercfg();

} // namespace wake
