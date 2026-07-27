module itunes_c;
import visual;
import itunes_x;
import <Windows.h>;

using std::scoped_lock;

void print_next_up_song_list() {
    iTunes_logger.logg_and_logg("print_next_up_song_list()");
    auto next_up_list = ac::utils::wstr_to_str(ac::clipboard::get_clipboard_text());
    std::istringstream list_stream(next_up_list);
    std::ostringstream formatted_list;
    std::string item;
    while (getline(list_stream, item, '\n')) {
        auto formatted_item = replace_tabs_with_brackets(item);
        formatted_list << formatted_item << "\n";
    }
    auto formatted_str = formatted_list.str();
    iTunes_logger.loggnl_and_printnl(formatted_str);
    ac::clipboard::set_clipboard_text(ac::utils::str_to_wstr(formatted_str));
    Sleep(50);
    ac::clipboard::paste_from_clipboard();
}

void iTunes_play_pause() {
    iTunes_logger.logg_and_logg("iTunes_play_pause()");
    ac_iTunes.play_pause();
    ac_iTunes.get_current_track();
}

void iTunes_prev_song() {
    iTunes_logger.logg_and_logg("iTunes_prev_song()");
    ac_iTunes.prev_song();
    ac_iTunes.get_current_track();
}

void iTunes_stop_song() {
    iTunes_logger.logg_and_logg("iTunes_stop_song()");
    ac_iTunes.stop_song();
    ac_iTunes.play_pause();
    ac_iTunes.play_pause();
    std::wstring current_track = ac_iTunes.get_current_track() + L"\n\n";
    ac::clipboard::set_clipboard_text(current_track.c_str());
    Sleep(50);
    ac::clipboard::paste_from_clipboard();
}

void print_iTunes_songs() {
    iTunes_logger.logg_and_logg("print_iTunes_songs()");
    std::wostringstream song_text;
    ac_iTunes.get_current_track();
    {
        scoped_lock lock(history_mtx);
        if (!ac_iTunes.song_history.empty()) {
            for (const auto& song : ac_iTunes.song_history) {
                song_text << song << L"\n";
            }
            if (ac_iTunes.song_history.size() != 1) {
                iTunes_logger.loggnl_and_printnl(song_text.str());
            }
            ac_iTunes.song_history.clear();
        }
        else {
            song_text << L"\n";
        }
    }
    song_text << L"\n";
    ac::clipboard::set_clipboard_text(song_text.str());
    Sleep(50);
    ac::clipboard::paste_from_clipboard();
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
