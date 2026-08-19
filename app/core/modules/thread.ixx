/**
 * \file thread.ixx
 * \brief Provides exception-handling support for thread entry functions.
 *
 * This module provides a template function that invokes a callable and reports
 * any exception that escapes from it. It is intended for use at thread entry
 * points, where an uncaught exception would otherwise terminate the program.
 */
module;

#include "ac_api.hpp"

export module auto_core.thread;

import std;
import auto_core.component;

export namespace ac::thread {

    /**
     * \brief Invokes a callable and reports any exception that it throws.
     *
     * The callable is executed inside a try-catch block. Standard exceptions
     * are logged with their diagnostic message. Any other exception is reported
     * with a generic thread-failure message.
     *
     * \tparam Func The callable type.
     * \param func The callable to invoke.
     * \param component The component used to report escaped exceptions.
     */
    template<typename Func>
    void run_with_exception_handling(Func&& func, ac::Component& component) {
        try {
            std::invoke(std::forward<Func>(func));
        }
        catch (const std::exception& exception) {
            component.print("Caught exception in thread function: {}", exception.what());
        }
        catch (...) {
            component.print("Unknown exception in thread function");
        }
    }

} // namespace ac::thread
