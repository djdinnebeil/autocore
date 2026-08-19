/**
 * \file journey.ixx
 * \brief Provides functions to create processes and start a journey process.
 *
 * This module includes functions for creating processes and starting a specific journey process.
 */
export module journey;

import std;

export namespace ac::main {

    bool create_process(
        const std::filesystem::path& executable_path,
        std::wstring_view arguments = {}
    );

}
