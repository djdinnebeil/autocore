/**
 * \file auto_core.cxx
 * \brief Main entry point for the Auto Core application.
 *
 * This file contains the primary logic for initializing and running the Auto Core application.
 * It includes functions for setting up the console, initializing core components, creating
 * named pipe servers for inter-process communication, and starting necessary components.
 * The main function manages the application lifecycle, including setting up the environment,
 * entering the message loop, and handling exceptions.
 *
 * \author Jose, Star, DJ, Daniel, Tabby
 */
import std;
import ac_modules;
import app_config;
import auto_core.logging.config;
import auto_core.clock;
import keymap;
import journey;
import runtime;
import keymap_runtime;
import end;
import auto_core.pipes;
import <Windows.h>;


/**
 * \brief Initializes the console and sets up exception handling.
 */
void auto_init() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleW(app_config::program_title().data());
    SetUnhandledExceptionFilter(unhandled_exception_handler);
    SetConsoleCtrlHandler(console_close_event, TRUE);
}
/**
 * \brief Initializes the core components, sets up keyboard hooks, and configures the program window.
 */
void core_init() {
    ac::main::keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, send_numpad_event, NULL, 0);
    ac::main::main_thread_id = GetCurrentThreadId();
    ac::main::close_window = close_window_hidden_init();
}
/**
 * \brief Creates named pipe servers for inter-process communication.
 */
void create_pipe_servers() {
    create_sp_pipe();
    create_itunes_pipe();
    create_wake_pipe();
}

/**
 * \brief Starts the components required for the application.
 */
void start_components() {
    start_iTunes_component();
    start_sp_component();
    start_wake_component();
    start_server();
}
void print_program_ready() {
    std::string program_start_str = "Program ready";
    program_start_str += "\nToday is " + ac::clock::get_day_of_week();
    program_start_str += "\n" + get_task_list();
    auto_core.print(program_start_str);
    Sleep(350);
}

/**
 * \brief The main function of the application.
 *
 * This function initializes the application, sets up the environment, and enters the message loop.
 *
 * \return int The exit code of the application.
 */
int main() {
    auto_init();
    core_init();
    initialize_logger_component();
    initialize_taskbar();
    crash_check();
    create_pipe_servers();
    start_components();
    initialize_keymap();
    std::thread taskbar_ps_thread(run_taskbar_ps);
    taskbar_ps_thread.detach();
    std::thread program_ready_thread(print_program_ready);
    program_ready_thread.detach();
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        try { process_numpad_event(msg); }
        catch (const std::exception& e) { auto_core.print("caught exception: {}", e.what()); }
        catch (...) { auto_core.print("uncaught exception"); }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    auto_core.logg_and_logg(
        "Auto Core is shutting down"
    );

    if (ac::logging::config::enabled()) {
        const bool logger_shutdown_sent =
            auto_core.request_logger_shutdown();

        if (!logger_shutdown_sent) {
            std::cerr
                << "Failed to send the termination signal to logger.exe\n";
        }
    }

    Sleep(100);
    return 0;
}
 
