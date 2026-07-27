/**
 * \file notepad.ixx
 * \brief Inserts an api key into the active window
 *
 */
export module notepad;
import std;
import config;
import clipboard;
import logger;
import print;
import ac_core;
import <Windows.h>;


export {
	void print_openai_api_key();
	void print_api_key();
}