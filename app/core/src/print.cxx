module print;

import std;

import clipboard;
import encoding;
import logger;

namespace {

    void paste_text(const std::wstring& msg) {
        std::wstring clipboard_text = msg + L"\n\n";

        ac::clipboard::set_clipboard_text(clipboard_text);
        ac::clipboard::paste_from_clipboard();
    }

}

namespace ac {

    void print(const std::string& msg) {
        std::cout << msg << '\n';
        std::cout.flush();
    }

    void printnl(const std::string& msg) {
        std::cout << msg;
        std::cout.flush();
    }

    void print(const std::wstring& msg) {
        print(ac::encoding::to_utf8(msg));
    }

    void printnl(const std::wstring& msg) {
        printnl(ac::encoding::to_utf8(msg));
    }

    void print(const char msg) {
        print(std::string(1, msg));
    }

    void printnl(const char msg) {
        printnl(std::string(1, msg));
    }

    void print(const wchar_t msg) {
        print(ac::encoding::to_utf8(msg));
    }

    void printnl(const wchar_t msg) {
        printnl(ac::encoding::to_utf8(msg));
    }

    void insert_text(const std::string& msg) {
        try {
            paste_text(ac::encoding::to_utf16(msg));
        }
        catch (const std::exception& exception) {
            ac::print(
                "Unable to convert text for insertion: {}",
                exception.what()
            );

            paste_text(L""); // Send the completion signal.
        }
    }

    void insert_text(const std::wstring& msg) {
        paste_text(msg);
    }

    void print_and_insert(const std::string& msg) {
        print(msg);
        insert_text(msg);
    }

    void print_and_insert(const std::wstring& msg) {
        print(msg);
        insert_text(msg);
    }

}