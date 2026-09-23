module;

#include "itunes_formatting_detail.hpp"
#include "itunes_track_detail.hpp"

module itunes_client;

import std;
import auto_core.core.clipboard;
import auto_core.core.encoding;

import itunes_component;
import itunes_runtime;
import <Windows.h>;

using std::scoped_lock;

void print_next_up_song_list() {
    itunes_component.log_and_log("print_next_up_song_list()");

    auto clipboard_text = itunes_component.get_clipboard_text();

    if (!clipboard_text) {
        return;
    }

    auto next_up_list =
        ac::encoding::to_utf8(*clipboard_text);

    std::istringstream list_stream(next_up_list);
    std::ostringstream formatted_list;

    std::string item;

    while (std::getline(list_stream, item, '\n')) {
        auto formatted_item =
            replace_tabs_with_brackets(item);

        formatted_list << formatted_item << '\n';
    }

    auto formatted_str = formatted_list.str();

    itunes_component
        .printnl_and_insert_text_replacing_clipboard(formatted_str);
}

void itunes_play_pause() {
    itunes_component.log_and_log("itunes_play_pause()");
    itunes::runtime::play_pause(ac_itunes);
}

void itunes_prev_song() {
    itunes_component.log_and_log("itunes_prev_song()");
    itunes::runtime::previous_song(ac_itunes);
}

void itunes_stop_song() {
    itunes_component.log_and_log("itunes_stop_song()");
    std::wstring current_track =
        itunes::runtime::stop_song(ac_itunes) + L"\n\n";
    itunes_component.insert_text_replacing_clipboard(current_track);
}

void print_itunes_songs() {
    itunes_component.log_and_log("print_itunes_songs()");

    ac_itunes.get_current_track();

    std::wstring output;
    {
        scoped_lock lock(history_mtx);
        output = itunes::track::detail::drain_history(
            ac_itunes.song_history
        );
    }

    itunes_component.print_and_insert(output);
}

std::string replace_tabs_with_brackets(const std::string& input) {
    return itunes::formatting::detail::format_queue_item(
        input,
        ac_itunes.tab_end
    );
}
