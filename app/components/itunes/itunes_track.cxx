module itunes_c;
import visual;
import itunes_x;
import <Windows.h>;
import <comdef.h>;

using std::scoped_lock;

TrackInfo iTunes::get_track_info() {
    TrackInfo info;
    DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
    VARIANT varResult;
    DISPID dispid;
    struct PropertyInfo {
        const wchar_t* name;
        wstring& value;
    };
    PropertyInfo properties[] = {
        {L"Name", info.name},
        {L"Artist", info.artist},
        {L"Album", info.album},
        {L"Location", info.location},
    };
    for (const auto& prop : properties) {
        BSTR bstrProp = SysAllocString(prop.name);
        hr = p_current_track->GetIDsOfNames(IID_NULL, &bstrProp, 1, LOCALE_USER_DEFAULT, &dispid);
        VariantInit(&varResult);
        hr = p_current_track->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dispparamsNoArgs, &varResult, NULL, NULL);
        if (SUCCEEDED(hr) && V_VT(&varResult) == VT_BSTR) {
            if (V_BSTR(&varResult) != NULL) {
                prop.value = V_BSTR(&varResult);
            }
            else {
                iTunes_logger.logg_and_logg("iTunes error with V_BSTR in get_track_info");
                prop.value = L"";
            }
        }
        VariantClear(&varResult);
        SysFreeString(bstrProp);
    }
    BSTR bstrDuration = SysAllocString(L"Duration");
    hr = p_current_track->GetIDsOfNames(IID_NULL, &bstrDuration, 1, LOCALE_USER_DEFAULT, &dispid);
    VariantInit(&varResult);
    hr = p_current_track->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dispparamsNoArgs, &varResult, NULL, NULL);
    if (SUCCEEDED(hr) && V_VT(&varResult) == VT_I4) {
        info.duration = V_I4(&varResult);
    }
    VariantClear(&varResult);
    SysFreeString(bstrDuration);
    return info;
}

wstring iTunes::get_current_track() {
    if (!initialized) {
        initialize_com();
    }
    p_current_track = get_current_track_com_object();
    if (!p_current_track) {
        remaining_song_duration = -1;
        return L"";
    }
    TrackInfo curr_song = get_track_info();
    int duration = curr_song.duration;
    int playback_position = get_current_playback_position();
    if (playback_position == 0) {
        remaining_song_duration = duration;
    }
    else if (playback_position > 0) {
        remaining_song_duration = duration - playback_position;
    }
    else if (playback_position == -1) {
        remaining_song_duration = -1;
    }
    wss dur;
    dur << duration / 60 << ':' << setw(2) << setfill(L'0') << duration % 60;
    wss ws;
    ws << '[' << curr_song.name << "] [" << curr_song.artist << "] [" << curr_song.album << "] [" << dur.str() << ']';
    track_location = curr_song.location;
    wstring current_song = ws.str();
    {
        scoped_lock lock(history_mtx);
        if (last_retrieved_song != current_song) {
            iTunes_logger.loggnl_and_loggnl("current song: ");
            iTunes_logger.logg_and_print(current_song);
            song_history.push_back(current_song);
        }
        last_retrieved_song = current_song;
    }
    return current_song;
}
