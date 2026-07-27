/**
 * \file link.ixx
 * \brief Generates custom user-defined strings for use within the Auto Core application.
 *
 * This module provides functions to generate and print messages for journaling inspiration
 * and task lists. It interacts with the clipboard and logs the generated messages.
 */
export module link;
import std;
import config;
import clipboard;
import logger;
import print;
import thread;
import ac_core;
import <Windows.h>;

export {
    void print_gpt_message();
}
