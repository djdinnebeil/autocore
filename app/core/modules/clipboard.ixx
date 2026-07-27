/**
\file clipboard.ixx
\brief Facilitates clipboard interactions for std::string handling within the Auto Core system.

Auto Core streamlines text insertion by utilizing the system clipboard and the 'ctrl + v' paste shortcut to send text to the active textbox.
*/
module;

#ifdef BUILDING_DLL
	#define DLL_API __declspec(dllexport)
#else
	#define DLL_API __declspec(dllimport)
#endif

export module clipboard;
import std;
import config;
import logger;
import <Windows.h>;

export namespace ac::clipboard {
	DLL_API void set_clipboard_text(const std::wstring& text);
	DLL_API std::wstring get_clipboard_text();
	DLL_API void paste_from_clipboard();
}
