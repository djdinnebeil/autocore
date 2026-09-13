/**
 * \file server_component.ixx
 * \brief Starts and stops the local file-server process.
 */
export module auto_core.main.components.server;

export {
    /** Starts `server_ac.exe` once. A second call is a no-op. */
    void start_server();
    /** Stops `server_ac.exe` with `TerminateProcess`. */
    void stop_server();
}
