module cloud;

import std;

import config;
import ac_component;
import print;

import <cpr/cpr.h>;

std::string firebase_url = "https://auto-core-cloud-default-rtdb.firebaseio.com/star.json";

/**
 * \brief Removes the first and last quotation marks from the input string.
 * \param input The input string to process.
 * \return The input string without the first and last characters if they are quotation marks.
 *
 * \details
 * This function is used to clean up the string data retrieved from Firebase.
 */
std::string remove_first_and_last_quotation_mark(const std::string& input) {
    if (!input.empty() && input[0] == '"' && input[input.length() - 1] == '"') {
        if (input.size() <= 2) {
            return {};
        }

        return input.substr(1, input.size() - 2);
    }
    return input;
}

/**
 * \brief Appends quotation marks around the input string.
 * \param input The input string to process.
 * \return The input string surrounded by quotation marks.
 *
 * \details
 * This function ensures that the string data sent to Firebase is properly formatted.
 */
std::string append_first_and_last_quotation_mark(const std::string& input) {
    return std::format("\"{}\"", input);
}

/**
 * \brief Retrieves a string value from Firebase.
 *
 * \details
 * The retrieved string value will have quotation marks added, which need to be removed for further processing.
 */
void get_string_from_firebase() {
    auto response = cpr::Get(cpr::Url {firebase_url});
    if (response.status_code != cpr::status::HTTP_OK) {
        auto_core.print("HTTP GET Request failed with status: {}", response.status_code);
    }
    else {
        auto_core.logg_and_logg("Retrieved data: {}", remove_first_and_last_quotation_mark(response.text));
    }
}

/**
 * \brief Updates a string value in Firebase.
 * \param value The new value to update in Firebase.
 *
 * \details
 * The string value sent to Firebase must have quotation marks added to be properly formatted.
 */
void update_string_in_firebase(const std::string& value) {
    std::string value_with_quotes = append_first_and_last_quotation_mark(value);
    auto response = cpr::Put(cpr::Url {firebase_url},
        cpr::Body {value_with_quotes},
        cpr::Header {{"Content-Type", "application/json"}});
    if (response.status_code != cpr::status::HTTP_OK) {
        auto_core.print("HTTP PUT Request failed with status: {}", response.status_code);
    }
    else {
        auto_core.logg_and_logg("Updated data: {}", remove_first_and_last_quotation_mark(response.text));
    }
}
