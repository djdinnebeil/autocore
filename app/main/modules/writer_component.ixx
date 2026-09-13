/**
 * \file writer_component.ixx
 * \brief Main-process client for `writer_ac.exe`.
 */
export module auto_core.main.components.writer;

export import command_registry;
export import writer_protocol;
import std;

export namespace writer_component::runtime_commands {
    /** Registers names from `writer.keymap_commands.txt`, or the protocol list. */
    void register_with(command_registry::Registry& registry);
}

export {
    /** Creates `ac_writer_pipe`. */
    void create_writer_pipe();
    /** Starts `writer_ac.exe`. */
    void start_writer_component();
    /** Waits up to five seconds for `writer_ready`. */
    bool wait_for_writer_ready();
    /** Sends protocol shutdown and resets the pipe handle. */
    void send_writer_end_signal();
}
