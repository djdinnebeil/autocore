/**
 * \file keymap_hardcoded.ixx
 * \brief Compiled keymap configuration.
 */
export module keymap_hardcoded;

import std;
import keymap;
import ac_modules;
import ac_actions;

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
        return {key, key_name, primary_name, secondary_name,
            {std::move(primary), std::move(secondary)}};
    }

#define binding(key, primary, secondary) \
    make_binding(key, #key, #primary, #secondary, primary, secondary)

#define named_binding(key, primary_name, secondary_name, primary, secondary) \
    make_binding(key, #key, primary_name, secondary_name, primary, secondary)

    const auto& hardcoded_keymap_entries() {
        static const std::array entries {
            binding(numkey_0, activate_function_key, deactivate_function_key),
            binding(numkey_1, activate_auto_core, close_program),
            named_binding(numkey_2, "activate_word", "save_file_and_create_new_file", activate_word, journal_command(::save_file_and_create_new_file)),
            named_binding(numkey_3, "print_Jose_choice", "make_print_choice(\"14th\", true)", journal_command(::print_Jose_choice), journal_print_choice_command("14th", true)),
            named_binding(numkey_4, "print_Daniel_choice", "activate_chrome", journal_command(::print_Daniel_choice), activate_chrome),
            named_binding(numkey_5, "activate_chrome", "print_Tyler_choice", activate_chrome, journal_command(::print_Tyler_choice)),
            named_binding(numkey_6, "itunes_next_song", "itunes_print_next_up", itunes_command(itunes::commands::next_song), itunes_command(itunes::commands::print_next_up)),
            binding(numkey_7, print_gpt_message, activate_folder),
            binding(numkey_8, refresh_firefox, activate_firefox),
            binding(numkey_9, print_timestamp, print_date_iso_with_timestamp_w),
            named_binding(numkey_star, "print_Star_choice", "print_episode_title", journal_command(::print_Star_choice), journal_command(::print_episode_title)),
            named_binding(numkey_plus, "itunes_print_songs", "sp_print_songs", itunes_command(itunes::commands::print_songs), sp_command(sp::commands::print_songs)),
            named_binding(numkey_dot, "print_Lily_choice", "print_James_choice", journal_command(::print_Lily_choice), journal_command(::print_James_choice)),
            named_binding(numkey_enter, "print_one_is_selected", "sp_next_song", journal_command(::print_one_is_selected), sp_command(sp::commands::next_song)),
            named_binding(numkey_dash, "itunes_stop_song", "print_Jose_choices", itunes_command(itunes::commands::stop_song), journal_command(::print_Jose_choices)),
            named_binding(numkey_slash, "retrieve_and_delete_recycle_bin", "itunes_remove_song", retrieve_and_delete_recycle_bin, itunes_command(itunes::commands::remove_song)),
            named_binding(play_pause_key, "itunes_play_pause", "print_Eric_choice", itunes_command(itunes::commands::play_pause), journal_command(::print_Eric_choice)),
            binding(home_page_key, activate_iTunes, activate_discord),
            named_binding(mail_key, "print_Luna_choice", "print_Katrina_choice", journal_command(::print_Luna_choice), journal_command(::print_Katrina_choice)),
            binding(calculator_key, create_new_note_in_notepad, launch_task_list),
        };
        return entries;
    }

#undef binding
#undef named_binding
}

export std::string hardcoded_keymap_contents() {
    std::string contents = "[keymap]\n";
    for (const auto& entry : hardcoded_keymap_entries()) {
        contents += std::format("{} = {{{}, {}}}\n", entry.key_name,
            entry.primary_name, entry.secondary_name);
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
