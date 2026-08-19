module itunes_c;

import std;
import auto_core.clipboard;
import auto_core.encoding;

import itunes_x;
import <Windows.h>;

using std::scoped_lock;

void print_next_up_song_list() {
    itunes_component.logg_and_logg("print_next_up_song_list()");

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

void iTunes_play_pause() {
    itunes_component.logg_and_logg("iTunes_play_pause()");
    ac_iTunes.play_pause();
    ac_iTunes.get_current_track();
}

void iTunes_prev_song() {
    itunes_component.logg_and_logg("iTunes_prev_song()");
    ac_iTunes.prev_song();
    ac_iTunes.get_current_track();
}

void iTunes_stop_song() {
    itunes_component.logg_and_logg("iTunes_stop_song()");
    ac_iTunes.stop_song();
    ac_iTunes.play_pause();
    ac_iTunes.play_pause();
    std::wstring current_track = ac_iTunes.get_current_track() + L"\n\n";
    itunes_component.insert_text_replacing_clipboard(current_track);
}

void print_iTunes_songs() {
    itunes_component.logg_and_logg("print_iTunes_songs()");

    std::wostringstream song_text;

    ac_iTunes.get_current_track();

    {
        scoped_lock lock(history_mtx);

        if (!ac_iTunes.song_history.empty()) {
            for (const auto& song : ac_iTunes.song_history) {
                song_text << song << L'\n';
            }

            ac_iTunes.song_history.clear();
        }
        else {
            song_text << L'\n';
        }
    }

    song_text << L'\n';

    const std::wstring output = song_text.str();

    itunes_component
        .printnl_and_insert_text_replacing_clipboard(output);
}

std::string replace_tabs_with_brackets(const std::string& input) {
    std::ostringstream output;
    output << '[';
    int tab_number = 0;
    for (char ch : input) {
        if (ch == '\t') {
            tab_number++;
            if (tab_number == ac_iTunes.tab_end) {
                break;
            }
            output << "] [";
        }
        else if (ch == '\r') {
        }
        else {
            output << ch;
        }
    }
    output << ']';
    return output.str();
}
