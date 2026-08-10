module itunes_c;
import std;
import <Windows.h>;
import <comdef.h>;

void iTunes::play_pause() {
    if (!initialized) {
        initialize_com();
    }
    DISPID dispidPlayPause;
    const OLECHAR* szPlayPause = L"PlayPause";
    BSTR bstrPlayPause = SysAllocString(szPlayPause);
    iTunes_app->GetIDsOfNames(IID_NULL, &bstrPlayPause, 1, LOCALE_USER_DEFAULT, &dispidPlayPause);
    SysFreeString(bstrPlayPause);
    DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
    iTunes_app->Invoke(dispidPlayPause, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &dispparamsNoArgs, NULL, NULL, NULL);
}

void iTunes::next_song() {
    if (!initialized) {
        initialize_com();
    }
    DISPID dispidNextTrack;
    const OLECHAR* szNextTrack = L"NextTrack";
    BSTR bstrNextTrack = SysAllocString(szNextTrack);
    iTunes_app->GetIDsOfNames(IID_NULL, &bstrNextTrack, 1, LOCALE_USER_DEFAULT, &dispidNextTrack);
    SysFreeString(bstrNextTrack);
    DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
    iTunes_app->Invoke(dispidNextTrack, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &dispparamsNoArgs, NULL, NULL, NULL);
}

void iTunes::prev_song() {
    if (!initialized) {
        initialize_com();
    }
    DISPID dispidPreviousTrack;
    const OLECHAR* szPreviousTrack = L"PreviousTrack";
    BSTR bstrPreviousTrack = SysAllocString(szPreviousTrack);
    iTunes_app->GetIDsOfNames(IID_NULL, &bstrPreviousTrack, 1, LOCALE_USER_DEFAULT, &dispidPreviousTrack);
    SysFreeString(bstrPreviousTrack);
    DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
    iTunes_app->Invoke(dispidPreviousTrack, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &dispparamsNoArgs, NULL, NULL, NULL);
}

void iTunes::stop_song() {
    if (!initialized) {
        initialize_com();
    }
    DISPID dispidStop;
    const OLECHAR* szStop = L"Stop";
    BSTR bstrStop = SysAllocString(szStop);
    iTunes_app->GetIDsOfNames(IID_NULL, &bstrStop, 1, LOCALE_USER_DEFAULT, &dispidStop);
    SysFreeString(bstrStop);
    DISPPARAMS dispparamsNoArgs = {nullptr, nullptr, 0, 0};
    iTunes_app->Invoke(dispidStop, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &dispparamsNoArgs, nullptr, nullptr, nullptr);
}

bool iTunes::is_playing() {
    if (!initialized || iTunes_app == nullptr) {
        return false;
    }
    const OLECHAR* szPlayerState = L"PlayerState";
    BSTR bstrPlayerState = SysAllocString(szPlayerState);
    DISPID dispidPlayerState;
    hr = iTunes_app->GetIDsOfNames(IID_NULL, &bstrPlayerState, 1, LOCALE_USER_DEFAULT, &dispidPlayerState);
    SysFreeString(bstrPlayerState);
    if (FAILED(hr)) {
        return false;
    }
    DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
    VARIANT varResult;
    VariantInit(&varResult);
    hr = iTunes_app->Invoke(dispidPlayerState, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dispparamsNoArgs, &varResult, NULL, NULL);
    if (FAILED(hr)) {
        return false;
    }
    bool is_playing = false;
    if (V_VT(&varResult) == VT_I4) {
        is_playing = V_I4(&varResult) == 1;
    }
    VariantClear(&varResult);
    return is_playing;
}

int iTunes::get_current_playback_position() {
    if (!initialized || iTunes_app == nullptr) {
        return -1;
    }
    const OLECHAR* szPlayerPosition = L"PlayerPosition";
    BSTR bstrPlayerPosition = SysAllocString(szPlayerPosition);
    DISPID dispidPlayerPosition;
    hr = iTunes_app->GetIDsOfNames(IID_NULL, &bstrPlayerPosition, 1, LOCALE_USER_DEFAULT, &dispidPlayerPosition);
    SysFreeString(bstrPlayerPosition);
    if (FAILED(hr)) {
        return -1;
    }
    DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
    VARIANT varResult;
    VariantInit(&varResult);
    hr = iTunes_app->Invoke(dispidPlayerPosition, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dispparamsNoArgs, &varResult, NULL, NULL);
    if (FAILED(hr)) {
        return -1;
    }
    int currentPosition = -1;
    if (V_VT(&varResult) == VT_I4) {
        currentPosition = V_I4(&varResult);
    }
    VariantClear(&varResult);
    return currentPosition;
}
