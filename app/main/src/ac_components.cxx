/**
 * \file ac_components.cxx
 * \brief Coordinates startup of the Main application components.
 */
module auto_core.main.components;

import auto_core.main.application;

namespace {
    void create_component_pipe_servers() {
        create_journal_pipe();
        create_spotify_pipe();
        create_itunes_pipe();
        create_wake_pipe();
        create_writer_pipe();
    }

    void start_components() {
        start_journal_component();
        start_itunes_component();
        start_spotify_component();
        start_wake_component();
        start_writer_component();
        start_server();
    }
}

ac::main::components::Session::Session(Session&& other) noexcept
    : active_ {other.active_} {
    other.active_ = false;
}

ac::main::components::Session::~Session() {
    if (active_) {
        stop_taskbar_component();
    }
}

ac::main::components::Session ac::main::components::initialize() {
    Session session;

    if (!initialize_taskbar_component()) {
        auto_core.logg_and_print(
            "taskbar_ac.exe did not initialize; native taskbar activation is "
            "unavailable for this session."
        );
    }

    create_component_pipe_servers();
    start_components();
    wait_for_journal_ready();

    if (!wait_for_writer_ready()) {
        auto_core.logg_and_print(
            "writer_ac.exe did not initialize; writer commands are "
            "unavailable for this session."
        );
    }

    return session;
}
