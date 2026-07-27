module print;

import std;
import config;
import logger;
import clipboard;
import <Windows.h>;

namespace ac {

    void print(const std::string& msg) {
        std::cout << msg << std::endl;
        main_log_stream << msg << std::endl;
    }

    void printnl(const std::string& msg) {
        std::cout << msg;
        main_log_stream << msg;
    }

    void print(const std::wstring& msg) {
        std::cout << ac::encoding::to_utf8(msg) << std::endl;
        main_log_stream << ac::encoding::to_utf8(msg) << std::endl;
    }

    void printnl(const std::wstring& msg) {
        std::cout << ac::encoding::to_utf8(msg);
        main_log_stream << ac::encoding::to_utf8(msg);
    }

    void print(char msg) {
        std::cout << ac::encoding::to_utf8(msg) << std::endl;
        main_log_stream << ac::encoding::to_utf8(msg) << std::endl;
    }

    void printnl(char msg) {
        std::cout << ac::encoding::to_utf8(msg);
        main_log_stream << ac::encoding::to_utf8(msg);
    }

    void print(wchar_t msg) {
        std::cout << ac::encoding::to_utf8(msg) << std::endl;
        main_log_stream << ac::encoding::to_utf8(msg) << std::endl;
    }

    void printnl(wchar_t msg) {
        std::cout << ac::encoding::to_utf8(msg);
        main_log_stream << ac::encoding::to_utf8(msg);
    }

    void print_to_screen(const std::string& msg) {
        print(msg);

        std::wostringstream ws;
        ws << msg.c_str() << L"\n\n";

        ac::clipboard::set_clipboard_text(ws.str());
        ac::clipboard::paste_from_clipboard();
    }

    void print_to_screen_w(const std::wstring& msg) {
        print(msg);

        std::wostringstream ws;
        ws << msg << L"\n\n";

        ac::clipboard::set_clipboard_text(ws.str());
        ac::clipboard::paste_from_clipboard();
    }

}