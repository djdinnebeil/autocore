/**
 * \file spotify_devices.cxx
 * \brief Serves device discovery to `spotify_editor.exe`.
 */
module spotify_client;

import std;
import auto_core.core.pipes;
import spotify_component;
import spotify_devices_protocol;

import <Windows.h>;

namespace {

std::thread device_thread;
std::atomic<bool> device_stop {false};

void serve_device_client(ac::pipes::Pipe& pipe) {
    const auto hello = ac::pipes::read_string(pipe);
    if (!hello || *hello != spotify::devices::protocol_id) {
        (void)ac::pipes::send_string(pipe, "protocol");
        return;
    }
    if (const auto sent = ac::pipes::send_string(pipe, "ok"); !sent) {
        return;
    }
    ac::pipes::CommandDispatcher dispatcher;
    dispatcher.set_command(
        spotify::devices::to_wire(spotify::devices::Request::list_devices),
        [&pipe, &dispatcher] {
            const auto records = ac_spotify.discover_current_devices();
            if (!records) {
                (void)ac::pipes::send_string(pipe, "error");
                (void)ac::pipes::send_string(pipe, records.error());
                dispatcher.request_stop();
                return;
            }
            if (const auto sent = ac::pipes::send_string(pipe, "ok"); !sent) {
                dispatcher.request_stop();
                return;
            }
            (void)ac::pipes::send_string(pipe, *records);
            dispatcher.request_stop();
        }
    );
    (void)dispatcher.process(pipe);
}

void device_request_loop() {
    while (!device_stop.load()) {
        auto server = ac::pipes::create_pipe_server(
            std::wstring {spotify::devices::pipe_name}
        );
        if (!server) {
            spotify_component.log_print("Unable to open the Spotify device pipe.");
            return;
        }
        ac::pipes::Pipe pipe = std::move(*server);
        if (ConnectNamedPipe(static_cast<HANDLE>(pipe.native_handle()), nullptr) == FALSE &&
            GetLastError() != ERROR_PIPE_CONNECTED) {
            continue;
        }
        if (device_stop.load()) {
            return;
        }
        serve_device_client(pipe);
    }
}

} // namespace

void start_spotify_device_requests() {
    if (device_thread.joinable()) {
        return;
    }
    device_stop.store(false);
    device_thread = std::thread {device_request_loop};
}

void stop_spotify_device_requests() {
    device_stop.store(true);
    if (auto poke = ac::pipes::connect_to_pipe_server(
            std::wstring {spotify::devices::pipe_name}
        )) {
        poke->reset();
    }
    if (device_thread.joinable()) {
        device_thread.join();
    }
}
