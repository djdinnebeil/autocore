/**
 * \file journal_cloud.ixx
 * \brief Optional Firebase string sync for the active journal series.
 */
export module journal_cloud;

import std;

export void get_string_from_firebase();
export void update_string_in_firebase(const std::string& value);
