module utils;
import std;
import <Windows.h>;

namespace ac::utils {
    /**
     * \brief Converts a wide std::string (std::wstring) to a standard std::string.
     *
     * \param wstr The wide std::string to convert.
     * \return The converted standard std::string.
     */
    std::string wstr_to_str(const std::wstring& wstr) {
        if (wstr.empty()) return std::string();
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string str(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size_needed, NULL, NULL);
        return str;
    }
    /**
     * \brief Converts a standard std::string to a wide std::string (std::wstring).
     *
     * \param str The standard std::string to convert.
     * \return The converted wide std::string.
     */
    std::wstring str_to_wstr(const std::string& str) {
        if (str.empty()) return std::wstring();
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
        std::wstring wstr(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
        return wstr;
    }
    /**
     * \brief Encodes a std::string into base64 format.
     *
     * \param in The input std::string to encode.
     * \return The base64 encoded std::string.
     */
    std::string base64_encode(const std::string& in) {
        std::string out;
        int val = 0, valb = -6;
        for (uint8_t c : in) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val << 8) >> (valb + 8)) & 0x3F]);
        while (out.size() % 4) out.push_back('=');
        return out;
    }
    /**
     * \brief Removes the first and last characters from a std::string.
     *
     * \param input The input std::string.
     * \return The std::string with the first and last characters removed. Returns an empty std::string if the input length is less than or equal to 2.
     */
    std::string remove_first_and_last_char(const std::string& input) {
        if (input.length() <= 2) {
            return "";
        }
        return input.substr(1, input.length() - 2);
    }
}