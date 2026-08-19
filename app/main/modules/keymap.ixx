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

namespace {
    struct hardcoded_keymap_entry {
        int key;
        std::string_view key_name;
        std::string_view primary_name;
        std::string_view secondary_name;
        function_state functions;
    };

    hardcoded_keymap_entry make_binding(
        int key,
        std::string_view key_name,
        std::string_view primary_name,
        std::string_view secondary_name,
        std::function<void()> primary,
        std::function<void()> secondary
    ) {
        return {
            key,
            key_name,
            primary_name,
            secondary_name,
            {std::move(primary), std::move(secondary)}
        };
    }

    // Preserve normal C++ identifiers at each call site while capturing their
    // spellings for creation of the default keymap.ini file.
#define binding(key, primary, secondary) \
    make_binding(key, #key, #primary, #secondary, primary, secondary)

    const auto& hardcoded_keymap_entries() {
        static const std::array entries {
            binding(numkey_0, activate_function_key, deactivate_function_key),
            binding(numkey_1, activate_auto_core, close_program),
            binding(numkey_2, activate_word, save_file_and_create_new_file),
            binding(numkey_3, print_Jose_choice, make_print_choice("14th", true)),
            binding(numkey_4, print_Daniel_choice, activate_chrome),
            binding(numkey_5, activate_chrome, print_Tyler_choice),
            binding(numkey_6, iTunes_next_song, print_next_up_song_list),
            binding(numkey_7, print_gpt_message, activate_folder),
            binding(numkey_8, refresh_firefox, activate_firefox),
            binding(numkey_9, print_timestamp, print_date_iso_with_timestamp_w),
            binding(numkey_star, print_Star_choice, print_episode_title),
            binding(numkey_plus, print_iTunes_songs, print_spotify_songs),
            binding(numkey_dot, print_Lily_choice, print_James_choice),
            binding(numkey_enter, print_one_is_selected, spotify_next_song),
            binding(numkey_dash, iTunes_stop_song, print_Jose_choices),
            binding(numkey_slash, retrieve_and_delete_recycle_bin, remove_iTunes_song),
            binding(play_pause_key, iTunes_play_pause, print_Eric_choice),
            binding(home_page_key, activate_iTunes, activate_discord),
            binding(mail_key, print_Luna_choice, print_Katrina_choice),
            binding(calculator_key, create_new_note_in_notepad, launch_task_list),
        };

        return entries;
    }

#undef binding
}

/**
 * \brief Returns the compiled keymap in runtime keymap.ini format.
 */
export std::string hardcoded_keymap_contents() {
    std::string contents = "[keymap]\n";

    for (const auto& entry : hardcoded_keymap_entries()) {
        contents += std::format(
            "{} = {{{}, {}}}\n",
            entry.key_name,
            entry.primary_name,
            entry.secondary_name
        );
    }

    return contents;
}

export void set_hardcoded_keymap() {
    ac_numkey_event.clear();
    ac_numkey_event.reserve(hardcoded_keymap_entries().size());

    for (const auto& entry : hardcoded_keymap_entries()) {
        ac_numkey_event.emplace(entry.key, entry.functions);
    }
}
