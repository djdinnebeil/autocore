/**
 * \file logger_x.ixx
 * \brief This module updates component logs.
 *
 * This module provides functionality to update individual logging components, such as iTunes and Spotify loggers.
 * It ensures that the logs for these components are kept up-to-date and properly managed.
 */
module;

#include "ac_api.hpp"

export module logger_x;

import std;
import component;
import pipes;
import <Windows.h>;

export namespace ac::logger {

    AC_API void connect_to_logger(
        ac::Component& component
    );

    AC_API bool send_to_logger(
        const ac::pipes::LogEvent& event
    );

    AC_API void close_logger_connection();
    AC_API bool shutdown_logger();
}