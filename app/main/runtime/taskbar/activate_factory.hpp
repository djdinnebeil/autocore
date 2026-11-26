/**
 * \file activate_factory.hpp
 * \brief Catalog metadata for the Taskbar `activate` factory.
 *
 * `auto_core_keymap.exe --build` and Main startup both publish this row.
 * The factory function stays in the Main runtime.
 */
#pragma once

#include <string_view>

namespace ac::main {

    inline constexpr std::string_view activate_factory_name = "activate";
    inline constexpr std::string_view activate_factory_autocomplete =
        "activate(\"application\")";
    inline constexpr std::string_view activate_factory_description =
        "Activates a taskbar application by its configured key. "
        "In keymap.map, use activate(\"<application_key>\"), "
        "for example activate(\"auto_core\").";
    inline constexpr std::string_view activate_factory_provider = "taskbar";

}
