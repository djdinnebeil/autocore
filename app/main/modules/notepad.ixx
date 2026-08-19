/**
 * \file notepad.ixx
 * \brief Inserts an api key into the active window
 *
 */
export module notepad;
export import command_registry;
import std;
import auto_core.clipboard;
import ac_main;
import ac_main;
import <Windows.h>;

export namespace notepad::runtime_commands {
    void register_with(command_registry::Registry& registry);
}

export {
	void print_openai_api_key();
	void print_api_key();
}
