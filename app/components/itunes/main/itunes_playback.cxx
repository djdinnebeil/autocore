module itunes_client;
import std;
import itunes_component;
import <Windows.h>;
import <comdef.h>;

void itunes_client::play_pause() {
    invoke_player_method(L"PlayPause");
}

void itunes_client::next_song() {
    invoke_player_method(L"NextTrack");
}

void itunes_client::prev_song() {
    invoke_player_method(L"PreviousTrack");
}

void itunes_client::stop_song() {
    invoke_player_method(L"Stop");
}

void itunes_client::invoke_player_method(const wchar_t* method_name) {
    if (!initialized && !initialize_com()) {
        return;
    }

    try {
        com_executor.invoke([this, method_name] {
            invoke_player_method_on_com_thread(method_name);
        });
    }
    catch (const std::exception& exception) {
        itunes_component.log_print(
            "Failed to dispatch iTunes playback command: {}",
            exception.what()
        );
    }
}

void itunes_client::invoke_player_method_on_com_thread(const wchar_t* method_name) {
    DISPID dispatch_id;
    BSTR member = SysAllocString(method_name);
    const HRESULT result = itunes_app->GetIDsOfNames(
        IID_NULL, &member, 1, LOCALE_USER_DEFAULT, &dispatch_id
    );
    SysFreeString(member);
    if (FAILED(result)) {
        return;
    }

    DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
    itunes_app->Invoke(
        dispatch_id,
        IID_NULL,
        LOCALE_SYSTEM_DEFAULT,
        DISPATCH_METHOD,
        &dispparamsNoArgs,
        nullptr,
        nullptr,
        nullptr
    );
}

bool itunes_client::is_playing() {
    if (!initialized) {
        return false;
    }
    return com_executor.invoke([this] {
        return is_playing_on_com_thread();
    });
}

bool itunes_client::is_playing_on_com_thread() {
    const OLECHAR* member_name = L"PlayerState";
    BSTR member = SysAllocString(member_name);
    DISPID dispatch_id;
    HRESULT result = itunes_app->GetIDsOfNames(
        IID_NULL, &member, 1, LOCALE_USER_DEFAULT, &dispatch_id
    );
    SysFreeString(member);
    if (FAILED(result)) {
        return false;
    }

    DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
    VARIANT varResult;
    VariantInit(&varResult);
    result = itunes_app->Invoke(dispatch_id, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dispparamsNoArgs, &varResult, NULL, NULL);
    if (FAILED(result)) {
        VariantClear(&varResult);
        return false;
    }
    bool is_playing = false;
    if (V_VT(&varResult) == VT_I4) {
        is_playing = V_I4(&varResult) == 1;
    }
    VariantClear(&varResult);
    return is_playing;
}

int itunes_client::get_current_playback_position_on_com_thread() {
    const OLECHAR* szPlayerPosition = L"PlayerPosition";
    BSTR bstrPlayerPosition = SysAllocString(szPlayerPosition);
    DISPID dispidPlayerPosition;
    HRESULT result = itunes_app->GetIDsOfNames(IID_NULL, &bstrPlayerPosition, 1, LOCALE_USER_DEFAULT, &dispidPlayerPosition);
    SysFreeString(bstrPlayerPosition);
    if (FAILED(result)) {
        return -1;
    }
    DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
    VARIANT varResult;
    VariantInit(&varResult);
    result = itunes_app->Invoke(dispidPlayerPosition, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dispparamsNoArgs, &varResult, NULL, NULL);
    if (FAILED(result)) {
        VariantClear(&varResult);
        return -1;
    }
    int currentPosition = -1;
    if (V_VT(&varResult) == VT_I4) {
        currentPosition = V_I4(&varResult);
    }
    VariantClear(&varResult);
    return currentPosition;
}
