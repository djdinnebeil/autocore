/**
 * \file journey.ixx
 * \brief Provides functions to create processes and start a journey process.
 *
 * This module includes functions for creating processes and starting a specific journey process.
 */
export module journey;

import std;

export void start_journey();

export namespace ac::main {
	bool create_process(const std::wstring& path);
}