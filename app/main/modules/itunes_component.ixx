/**
 * \file itunes_component.ixx
 * \brief Owns the main-process side of the iTunes component connection.
 */
export module auto_core.main.components.itunes;

export import itunes_protocol;
export import command_registry;
import std;

export namespace itunes_component::runtime_commands {
    /** Registers manifest names plus the `print_next_up_song_list` alias. */
    void register_with(command_registry::Registry& registry);
}

export {
    /** Creates `ac_itunes_pipe`. There is no ready-wait. */
    void create_itunes_pipe();
    /** Starts `itunes_ac.exe`. */
    void start_itunes_component();
    /** Sends protocol shutdown. Not serialized with named invokes. */
    void send_itunes_end_signal();
}
