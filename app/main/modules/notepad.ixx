/**
 * \file notepad.ixx
 * \brief Inserts an api key into the active window
 *
 */
export module notepad;
import std;
import config;
import clipboard;
import ac_component;
import print;
import ac_main;
import <Windows.h>;


export {
	void print_openai_api_key();
	void print_api_key();
}