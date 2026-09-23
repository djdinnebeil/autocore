module itunes_client;
import std;
import itunes_component;
import <Windows.h>;
import <comdef.h>;
import <atlbase.h>;

BOOL CALLBACK enum_itunes_window(HWND hwnd, LPARAM lParam) {
    const size_t max_length = 6;
    const int length = GetWindowTextLength(hwnd);
    if (length != max_length) {
        return TRUE;
    }
    std::wstring window_title(length, L'\0');
    if (!GetWindowTextW(hwnd, &window_title[0], length + 1)) {
        return TRUE;
    }
    if (window_title == L"iTunes") {
        *reinterpret_cast<bool*>(lParam) = true;
        return FALSE;
    }
    return TRUE;
}

bool is_itunes_open() {
    bool itunes_window_found = false;
    EnumWindows(enum_itunes_window, reinterpret_cast<LPARAM>(&itunes_window_found));
    return itunes_window_found;
}

bool itunes_client::initialize_com() {
    return itunes::runtime::initialize_with_retry(
        *this,
        {},
        [](const std::chrono::milliseconds delay) {
            std::this_thread::sleep_for(delay);
        }
    );
}

bool itunes_client::initialize_attempt() {
    const std::scoped_lock lock {initialization_mutex};
    if (initialized) {
        return true;
    }
    if (shutdown_requested) {
        return false;
    }

    try {
        return com_executor.invoke([this] {
            return initialize_on_com_thread();
        });
    }
    catch (const std::exception& exception) {
        itunes_component.log_and_print(
            "Failed to dispatch iTunes COM initialization: {}",
            exception.what()
        );
        return false;
    }
}

bool itunes_client::initialize_on_com_thread() {
    if (initialized) {
        return true;
    }

    HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(result)) {
        itunes_component.log_and_print("Failed to initialize COM library.");
        return false;
    }
    com_apartment_initialized = true;

    CLSID clsid;
    result = CLSIDFromProgID(L"iTunes.Application", &clsid);
    if (FAILED(result)) {
        itunes_component.log_and_print("Failed to get CLSID from ProgID.");
        release_on_com_thread();
        return false;
    }

    result = CoCreateInstance(
        clsid,
        nullptr,
        CLSCTX_LOCAL_SERVER,
        IID_IDispatch,
        reinterpret_cast<void**>(&itunes_app)
    );
    if (FAILED(result)) {
        itunes_component.log_and_print("Failed to create iTunes COM instance.");
        release_on_com_thread();
        return false;
    }

    initialized = true;
    end_thread = false;
    if (!itunes_thread.joinable()) {
        itunes_thread = std::thread(&itunes_client::start_itunes_thread, this);
    }
    return true;
}

bool itunes_client::is_initialized() const noexcept {
    return initialized.load();
}

void itunes_client::shutdown() noexcept {
    if (shutdown_requested.exchange(true)) {
        return;
    }

    end_thread = true;
    itunes_playback_state_change = true;
    itunes_condition.notify_one();
    if (itunes_thread.joinable()) {
        itunes_thread.join();
    }

    try {
        com_executor.invoke([this] {
            release_on_com_thread();
        });
    }
    catch (...) {
        initialized = false;
    }
    com_executor.stop();
    itunes_component.log("end of itunes_client::shutdown()");
}

void itunes_client::release_on_com_thread() noexcept {
    if (p_current_track != nullptr) {
        p_current_track.Release();
    }
    if (itunes_app != nullptr) {
        itunes_app.Release();
    }
    if (com_apartment_initialized) {
        CoUninitialize();
        com_apartment_initialized = false;
    }
    initialized = false;
}

CComPtr<IDispatch> itunes_client::get_current_track_com_object_on_com_thread() {
    if (itunes_app == nullptr) {
        return nullptr;
    }
    const OLECHAR* szMember = L"CurrentTrack";
    BSTR bstrMember = SysAllocString(szMember);
    if (!bstrMember) {
        return nullptr;
    }
    DISPID dispidCurrentTrack;
    HRESULT result = itunes_app->GetIDsOfNames(IID_NULL, &bstrMember, 1, LOCALE_USER_DEFAULT, &dispidCurrentTrack);
    SysFreeString(bstrMember);
    if (FAILED(result)) {
        return nullptr;
    }
    DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
    VARIANT varResult;
    VariantInit(&varResult);
    result = itunes_app->Invoke(dispidCurrentTrack, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dispparamsNoArgs, &varResult, NULL, NULL);
    if (FAILED(result)) {
        VariantClear(&varResult);
        return nullptr;
    }
    if (V_VT(&varResult) != VT_DISPATCH || V_DISPATCH(&varResult) == NULL) {
        VariantClear(&varResult);
        return nullptr;
    }
    CComPtr<IDispatch> current_track = V_DISPATCH(&varResult);
    VariantClear(&varResult);
    p_current_track = std::move(current_track);
    const OLECHAR* szName = L"Name";
    BSTR bstrName = SysAllocString(szName);
    DISPID dispidName;
    result = p_current_track->GetIDsOfNames(IID_NULL, &bstrName, 1, LOCALE_USER_DEFAULT, &dispidName);
    SysFreeString(bstrName);
    if (FAILED(result)) {
        return nullptr;
    }
    return p_current_track;
}
