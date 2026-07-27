module itunes_c;
import visual;
import itunes_x;
import <Windows.h>;
import <comdef.h>;
import <atlbase.h>;

BOOL CALLBACK enum_iTunes_window(HWND hwnd, LPARAM lParam) {
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

bool is_iTunes_open() {
    bool iTunes_window_found = false;
    EnumWindows(enum_iTunes_window, reinterpret_cast<LPARAM>(&iTunes_window_found));
    return iTunes_window_found;
}

void iTunes::initialize_com() {
    if (initialized) {
        return;
    }
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        iTunes_logger.logg_and_print("Failed to initialize COM library.");
        return;
    }
    CLSID clsid;
    hr = CLSIDFromProgID(L"iTunes.Application", &clsid);
    if (FAILED(hr)) {
        iTunes_logger.logg_and_print("Failed to get CLSID from ProgID.");
        CoUninitialize();
        return;
    }
    hr = CoCreateInstance(clsid, NULL, CLSCTX_LOCAL_SERVER, IID_IDispatch, (void**)&iTunes_app);
    if (FAILED(hr)) {
        iTunes_logger.logg_and_print("Failed to create iTunes COM instance.");
        CoUninitialize();
        return;
    }
    iTunes_thread = std::thread(&iTunes::start_iTunes_thread, this);
    iTunes_thread.detach();
    initialized = true;
}

void iTunes::finalize_com() {
    if (!initialized) {
        return;
    }
    end_thread = true;
    iT_playback_state_change = true;
    iT_cv.notify_one();
    if (iTunes_thread.joinable()) {
        iTunes_thread.join();
    }
    if (p_current_track != nullptr) {
        p_current_track.Release();
    }
    if (iTunes_app != nullptr) {
        iTunes_app.Release();
    }
    CoUninitialize();
    initialized = false;
    iTunes_logger.logg("end of iTunes::finalize_com()");
}

CComPtr<IDispatch> iTunes::get_current_track_com_object() {
    if (iTunes_app == nullptr) {
        return nullptr;
    }
    const OLECHAR* szMember = L"CurrentTrack";
    BSTR bstrMember = SysAllocString(szMember);
    if (!bstrMember) {
        return nullptr;
    }
    DISPID dispidCurrentTrack;
    hr = iTunes_app->GetIDsOfNames(IID_NULL, &bstrMember, 1, LOCALE_USER_DEFAULT, &dispidCurrentTrack);
    SysFreeString(bstrMember);
    if (FAILED(hr)) {
        return nullptr;
    }
    DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
    VARIANT varResult;
    VariantInit(&varResult);
    hr = iTunes_app->Invoke(dispidCurrentTrack, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dispparamsNoArgs, &varResult, NULL, NULL);
    if (FAILED(hr)) {
        CoUninitialize();
        return nullptr;
    }
    if (V_VT(&varResult) != VT_DISPATCH || V_DISPATCH(&varResult) == NULL) {
        VariantClear(&varResult);
        return nullptr;
    }
    p_current_track = V_DISPATCH(&varResult);
    const OLECHAR* szName = L"Name";
    BSTR bstrName = SysAllocString(szName);
    DISPID dispidName;
    hr = p_current_track->GetIDsOfNames(IID_NULL, &bstrName, 1, LOCALE_USER_DEFAULT, &dispidName);
    SysFreeString(bstrName);
    if (FAILED(hr)) {
        CoUninitialize();
        return nullptr;
    }
    return p_current_track;
}
