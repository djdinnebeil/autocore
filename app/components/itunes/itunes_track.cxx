module;

#include "itunes_track_detail.hpp"

module itunes_client;
import std;
import itunes_component;
import <Windows.h>;
import <comdef.h>;

import auto_core.core.encoding;

using std::scoped_lock;

TrackInfo itunes_client::get_track_info_on_com_thread() {
    TrackInfo info;
    DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
    VARIANT varResult;
    DISPID dispid;
    struct PropertyInfo {
        const wchar_t* name;
        std::wstring& value;
    };
    PropertyInfo properties[] = {
        {L"Name", info.name},
        {L"Artist", info.artist},
        {L"Album", info.album},
        {L"Location", info.location},
    };
    for (const auto& prop : properties) {
        BSTR bstrProp = SysAllocString(prop.name);
        HRESULT result = p_current_track->GetIDsOfNames(IID_NULL, &bstrProp, 1, LOCALE_USER_DEFAULT, &dispid);
        VariantInit(&varResult);
        if (SUCCEEDED(result)) {
            result = p_current_track->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dispparamsNoArgs, &varResult, NULL, NULL);
        }
        if (SUCCEEDED(result) && V_VT(&varResult) == VT_BSTR) {
            if (V_BSTR(&varResult) != NULL) {
                prop.value = V_BSTR(&varResult);
            }
            else {
                itunes_component.logg_and_logg("iTunes error with V_BSTR in get_track_info");
                prop.value = L"";
            }
        }
        VariantClear(&varResult);
        SysFreeString(bstrProp);
    }
    BSTR bstrDuration = SysAllocString(L"Duration");
    HRESULT result = p_current_track->GetIDsOfNames(IID_NULL, &bstrDuration, 1, LOCALE_USER_DEFAULT, &dispid);
    VariantInit(&varResult);
    if (SUCCEEDED(result)) {
        result = p_current_track->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dispparamsNoArgs, &varResult, NULL, NULL);
    }
    if (SUCCEEDED(result) && V_VT(&varResult) == VT_I4) {
        info.duration = V_I4(&varResult);
    }
    VariantClear(&varResult);
    SysFreeString(bstrDuration);
    return info;
}

std::wstring itunes_client::get_current_track() {
    if (!initialized && !initialize_com()) {
        remaining_song_duration = -1;
        return L"";
    }

    try {
        return com_executor.invoke([this] {
            return get_current_track_on_com_thread();
        });
    }
    catch (const std::exception& exception) {
        remaining_song_duration = -1;
        itunes_component.logg_and_print(
            "Failed to dispatch iTunes track query: {}",
            exception.what()
        );
        return L"";
    }
}

std::wstring itunes_client::get_current_track_on_com_thread() {
    p_current_track = get_current_track_com_object_on_com_thread();
    if (!p_current_track) {
        remaining_song_duration = -1;
        return L"";
    }
    TrackInfo curr_song = get_track_info_on_com_thread();
    const int duration = curr_song.duration;
    int playback_position = get_current_playback_position_on_com_thread();
    if (playback_position == 0) {
        remaining_song_duration = duration;
    }
    else if (playback_position > 0) {
        remaining_song_duration = duration - playback_position;
    }
    else if (playback_position == -1) {
        remaining_song_duration = -1;
    }
    track_location = curr_song.location;
    const std::wstring current_song = itunes::track::detail::format_track(
        curr_song.name,
        curr_song.artist,
        curr_song.album,
        duration
    );
    {
        scoped_lock lock(history_mtx);
        if (itunes::track::detail::record_history(
                song_history,
                last_retrieved_song,
                current_song
            )) {
            itunes_component.loggnl_and_loggnl("current song: ");
            itunes_component.logg_and_print(current_song);
        }
    }
    return current_song;
}
