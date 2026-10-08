/**
 * \file launch_descriptor_resource.hpp
 * \brief Reads `AC_LAUNCH_DESCRIPTOR` RCDATA from an executable image.
 */
#pragma once

#include "launch_descriptor.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <optional>
#include <string>
#include <string_view>

namespace ac::main::launch_descriptor {

    /**
     * \brief Reads the embedded launch descriptor from `module`.
     * \return `nullopt` when the resource is absent. A present resource that
     *         cannot be read or parsed returns `ok == false`.
     */
    [[nodiscard]]
    inline std::optional<ParseResult> read_embedded(
        const HMODULE module,
        const std::string_view label
    ) {
        const HRSRC found = FindResourceW(
            module,
            embedded_resource_name,
            RT_RCDATA
        );
        if (!found) {
            return std::nullopt;
        }

        const auto fail = [&](std::string message) {
            ParseResult failed;
            failed.error = std::string {label} + ": " + std::move(message);
            return failed;
        };

        const DWORD size = SizeofResource(module, found);
        const HGLOBAL loaded = LoadResource(module, found);
        const auto* bytes = loaded
            ? static_cast<const char*>(LockResource(loaded))
            : nullptr;
        if (!loaded || bytes == nullptr || size == 0) {
            return fail("unable to read launch descriptor");
        }

        auto parsed = parse(std::string_view {bytes, size});
        if (!parsed.ok) {
            parsed.error = std::string {label} + ": " + parsed.error;
        }
        return parsed;
    }

} // namespace ac::main::launch_descriptor
