module itunes_removal;
import std;
import itunes_client;
import itunes_component;
import itunes_runtime;
import <Windows.h>;
import <shobjidl.h>;
import <ShlObj.h>;

std::filesystem::path itunes_client::remove_current_track() {
    return com_executor.invoke([this] {
        DISPID dispatch_id;
        const OLECHAR* member_name = L"Delete";
        BSTR member = SysAllocString(member_name);
        const HRESULT result = p_current_track->GetIDsOfNames(
            IID_NULL, &member, 1, LOCALE_USER_DEFAULT, &dispatch_id
        );
        SysFreeString(member);
        if (SUCCEEDED(result)) {
            DISPPARAMS no_arguments = {nullptr, nullptr, 0, 0};
            p_current_track->Invoke(
                dispatch_id,
                IID_NULL,
                LOCALE_SYSTEM_DEFAULT,
                DISPATCH_METHOD,
                &no_arguments,
                nullptr,
                nullptr,
                nullptr
            );
        }
        p_current_track = nullptr;
        return std::filesystem::path {track_location};
    });
}

bool itunes_client::has_current_track() const noexcept {
    if (!initialized) {
        return false;
    }
    try {
        return const_cast<itunes_client*>(this)->com_executor.invoke([this] {
            return p_current_track != nullptr;
        });
    }
    catch (...) {
        return false;
    }
}

namespace {
    class windows_file_recycler final : public itunes::runtime::file_recycler {
    public:
        bool recycle(const std::filesystem::path& path) override {
            SHFILEOPSTRUCT operation {};
            operation.wFunc = FO_DELETE;
            std::wstring double_null_terminated_path = path.wstring() + L'\0';
            operation.pFrom = double_null_terminated_path.c_str();
            operation.fFlags = FOF_ALLOWUNDO | FOF_NO_UI;
            return SHFileOperation(&operation) == 0;
        }
    };

    windows_file_recycler file_recycler;
}

void remove_itunes_song() {
    itunes_component.log_main("remove_itunes_song()");
    const auto result = itunes::runtime::remove_song(
        ac_itunes,
        file_recycler
    );
    if (result.status == itunes::runtime::removal_status::no_current_track) {
        return;
    }

    const std::wstring log_message =
        result.status == itunes::runtime::removal_status::recycled
        ? L"File moved to the recycle bin: " + result.path.wstring()
        : L"Error - file not moved to the recycle bin: " + result.path.wstring();
    itunes_component.log_main(log_message);
}
