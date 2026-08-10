/**
\file keymap.ixx
\brief The keymap
*/
export module keymap;

import std;
import ac_modules;
import ac_actions;

/**
 * \brief Struct to hold primary and secondary function states.
 */
export struct function_state {
    std::function<void()> primary;
    std::function<void()> secondary;
};

/**
 * \brief Unordered map to store function states for numkey events.
 */
export std::unordered_map<int, function_state> ac_numkey_event;

/**
 * \brief Sets the hardcoded keymap.
 *
 * This function populates the keymap with the configured primary and
 * secondary actions for each supported key.
 */
export void set_hardcoded_keymap() {
    ac_numkey_event[numkey_0] = {activate_function_key, deactivate_function_key};
    ac_numkey_event[numkey_1] = {activate_auto_core, close_program};
    ac_numkey_event[numkey_2] = {activate_word, save_file_and_create_new_file};
    ac_numkey_event[numkey_3] = {print_Jose_choice, make_print_choice("14th",true)};
    ac_numkey_event[numkey_4] = {print_Daniel_choice, activate_chrome};
    ac_numkey_event[numkey_5] = {activate_chrome, print_Tyler_choice};
    ac_numkey_event[numkey_6] = {iTunes_next_song, print_next_up_song_list};
    ac_numkey_event[numkey_7] = {print_gpt_message, activate_folder};
    ac_numkey_event[numkey_8] = {refresh_firefox, activate_firefox};
    ac_numkey_event[numkey_9] = {print_timestamp, print_date_iso_with_timestamp_w};
    ac_numkey_event[numkey_star] = {print_Star_choice, print_episode_title};
    ac_numkey_event[numkey_plus] = {print_iTunes_songs, print_spotify_songs};
    ac_numkey_event[numkey_dot]  = {print_Lily_choice, print_James_choice};
    ac_numkey_event[numkey_enter] = {print_one_is_selected, spotify_next_song};
    ac_numkey_event[numkey_dash]  = {iTunes_stop_song, print_Jose_choices};
    ac_numkey_event[numkey_slash] = {retrieve_and_delete_recycle_bin, remove_iTunes_song};
    ac_numkey_event[play_pause_key] = {iTunes_play_pause, print_Eric_choice};
    ac_numkey_event[home_page_key] = {activate_iTunes, activate_discord};
    ac_numkey_event[mail_key] = {print_Luna_choice, print_Katrina_choice};
    ac_numkey_event[calculator_key] = {create_new_note_in_notepad, launch_task_list};
}
