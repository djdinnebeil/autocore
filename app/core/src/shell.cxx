module;

#include <Windows.h>
#include <shlobj.h>

module auto_core.core.shell;

import std;
import auto_core.core.error;

namespace ac::shell {

    void set_process_app_user_model_id() noexcept {
        constexpr wchar_t app_user_model_id[] = L"Djdinn.AutoCore";
        const HRESULT result =
            SetCurrentProcessExplicitAppUserModelID(app_user_model_id);
        if (SUCCEEDED(result)) {
            return;
        }

        try {
            ac::error::log(
                std::format(
                    "SetCurrentProcessExplicitAppUserModelID(Djdinn.AutoCore) failed. HRESULT = 0x{:08X}",
                    static_cast<unsigned int>(result)
                )
            );
        }
        catch (...) {
            ac::error::log(
                "SetCurrentProcessExplicitAppUserModelID(Djdinn.AutoCore) failed."
            );
        }
    }

}
