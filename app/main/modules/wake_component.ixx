/**
 * \file wake_component.ixx
 * \brief Main-process client for `wake_ac.exe`.
 */
export module auto_core.main.components.wake;

export {
    /** Creates the wake control-pipe server. */
    void create_wake_pipe();
    /** Starts `wake_ac.exe`. There is no ready-wait. */
    void start_wake_component();
    /** Sends the wake shutdown command. */
    void send_wake_end_signal();
}
