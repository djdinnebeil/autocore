#include <Windows.h>

#include <iostream>
#include <string>

namespace {

    class ClipboardSession {
    public:
        ClipboardSession()
            : opened_(OpenClipboard(GetConsoleWindow()) != FALSE) {
        }

        ~ClipboardSession() {
            if (opened_) {
                CloseClipboard();
            }
        }

        ClipboardSession(const ClipboardSession&) = delete;
        ClipboardSession& operator=(const ClipboardSession&) = delete;

        [[nodiscard]] explicit operator bool() const noexcept {
            return opened_;
        }

    private:
        bool opened_;
    };

}

int main() {
    ClipboardSession clipboard;

    if (!clipboard) {
        std::cerr
            << "Failed to open the clipboard. "
            << "Another process may already have it open.\n";
        return 1;
    }

    std::cout
        << "The clipboard is being held open.\n"
        << "Run the Auto Core clipboard test now.\n"
        << "Press Enter to release the clipboard and exit.\n";

    std::string input;
    std::getline(std::cin, input);

    return 0;
}