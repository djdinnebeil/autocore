module;

#include "itunes_track_detail.hpp"
#include "../shared/itunes_metadata_detail.hpp"
#include "../shared/song_template_detail.hpp"

module itunes_client;
import std;
import itunes_component;
import <Windows.h>;
import <comdef.h>;

import auto_core.core.encoding;

using std::scoped_lock;

namespace {

void fetch_bstr(
    IDispatch* track,
    const wchar_t* property_name,
    std::wstring& value
) {
    DISPPARAMS no_arguments = {nullptr, nullptr, 0, 0};
    VARIANT result_value;
    DISPID dispatch_id = 0;
    BSTR name = SysAllocString(property_name);
    HRESULT result = track->GetIDsOfNames(
        IID_NULL,
        &name,
        1,
        LOCALE_USER_DEFAULT,
        &dispatch_id
    );
    VariantInit(&result_value);
    if (SUCCEEDED(result)) {
        result = track->Invoke(
            dispatch_id,
            IID_NULL,
            LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYGET,
            &no_arguments,
            &result_value,
            nullptr,
            nullptr
        );
    }
    if (SUCCEEDED(result) && V_VT(&result_value) == VT_BSTR) {
        if (V_BSTR(&result_value) != nullptr) {
            value = V_BSTR(&result_value);
        }
        else {
            itunes_component.log_main("iTunes error with V_BSTR in get_track_info");
            value = L"";
        }
    }
    VariantClear(&result_value);
    SysFreeString(name);
}

void fetch_i4(
    IDispatch* track,
    const wchar_t* property_name,
    int& value,
    bool& found
) {
    DISPPARAMS no_arguments = {nullptr, nullptr, 0, 0};
    VARIANT result_value;
    DISPID dispatch_id = 0;
    BSTR name = SysAllocString(property_name);
    HRESULT result = track->GetIDsOfNames(
        IID_NULL,
        &name,
        1,
        LOCALE_USER_DEFAULT,
        &dispatch_id
    );
    VariantInit(&result_value);
    if (SUCCEEDED(result)) {
        result = track->Invoke(
            dispatch_id,
            IID_NULL,
            LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYGET,
            &no_arguments,
            &result_value,
            nullptr,
            nullptr
        );
    }
    if (SUCCEEDED(result) && V_VT(&result_value) == VT_I4) {
        value = V_I4(&result_value);
        found = true;
    }
    VariantClear(&result_value);
    SysFreeString(name);
}

void fetch_duration(IDispatch* track, int& duration) {
    DISPPARAMS no_arguments = {nullptr, nullptr, 0, 0};
    VARIANT result_value;
    DISPID dispatch_id = 0;
    BSTR name = SysAllocString(L"Duration");
    HRESULT result = track->GetIDsOfNames(
        IID_NULL,
        &name,
        1,
        LOCALE_USER_DEFAULT,
        &dispatch_id
    );
    VariantInit(&result_value);
    if (SUCCEEDED(result)) {
        result = track->Invoke(
            dispatch_id,
            IID_NULL,
            LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYGET,
            &no_arguments,
            &result_value,
            nullptr,
            nullptr
        );
    }
    if (SUCCEEDED(result) && V_VT(&result_value) == VT_I4) {
        duration = V_I4(&result_value);
    }
    VariantClear(&result_value);
    SysFreeString(name);
}

} // namespace

TrackInfo itunes_client::get_track_info_on_com_thread() {
    TrackInfo info;
    fetch_bstr(p_current_track, L"Location", info.location);
    fetch_duration(p_current_track, info.duration);
    fetch_bstr(p_current_track, L"Name", info.name);
    fetch_bstr(p_current_track, L"Artist", info.artist);
    fetch_bstr(p_current_track, L"Album", info.album);

    int database_id = 0;
    bool found_database_id = false;
    fetch_i4(p_current_track, L"TrackDatabaseID", database_id, found_database_id);
    info.has_database_id = found_database_id;
    info.database_id = database_id;
    if (!found_database_id) {
        itunes_component.log(
            "iTunes track has no TrackDatabaseID; listening history was not updated."
        );
    }
    return info;
}

std::wstring itunes_client::get_current_track() {
    if (!initialized && !initialize_com()) {
        remaining_song_duration = -1;
        return L"";
    }

    try {
        return com_executor.invoke([this] {
            return get_current_track_on_com_thread(nullptr);
        });
    }
    catch (const std::exception& exception) {
        remaining_song_duration = -1;
        itunes_component.log_print(
            "Failed to dispatch iTunes track query: {}",
            exception.what()
        );
        return L"";
    }
}

itunes_client::ListeningSample itunes_client::capture_listening_sample() {
    if (!initialized && !initialize_com()) {
        remaining_song_duration = -1;
        return {};
    }

    try {
        return com_executor.invoke([this] {
            ListeningSample sample;
            static_cast<void>(get_current_track_on_com_thread(&sample));
            return sample;
        });
    }
    catch (const std::exception& exception) {
        remaining_song_duration = -1;
        itunes_component.log_print(
            "Failed to dispatch iTunes track query: {}",
            exception.what()
        );
        return {};
    }
}

std::wstring itunes_client::get_current_track_on_com_thread(ListeningSample* sample) {
    if (sample != nullptr) {
        *sample = {};
    }
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
    if (sample != nullptr) {
        sample->title = curr_song.name;
        sample->artist = curr_song.artist;
        sample->album = curr_song.album;
        sample->duration_seconds = duration;
        sample->position_seconds = playback_position;
        sample->playing = is_playing_on_com_thread();
        if (curr_song.has_database_id) {
            sample->has_track = true;
            sample->track_id = curr_song.database_id;
        }
    }
    const std::wstring duration_text =
        itunes::metadata::detail::render_duration(duration);
    const std::wstring current_song = itunes::song::detail::apply(
        song_format,
        [&](const std::string_view token) -> std::wstring {
            if (token == "name") {
                return curr_song.name;
            }
            if (token == "artist") {
                return curr_song.artist;
            }
            if (token == "album") {
                return curr_song.album;
            }
            if (token == "duration") {
                return duration_text;
            }
            return {};
        }
    );
    {
        scoped_lock lock(history_mtx);
        if (itunes::track::detail::record_history(
                song_history,
                last_retrieved_song,
                current_song
            )) {
            itunes_component.lognl_main("current song: ");
            itunes_component.log_print(current_song);
        }
    }
    return current_song;
}
