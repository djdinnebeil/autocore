/**
 * \file path_utils.ixx
 * \brief Provides path-handling utilities for the slash component.
 */
export module path_utils;

import std;

/**
 * \brief Extracts the final component from a file or folder path.
 * \param filepath The full path.
 * \return The final path component.
 */
export std::wstring extract_path_filename(const std::wstring& filepath) {
    size_t backslash_index = filepath.find_last_of(L'\\');
    if (backslash_index == std::wstring::npos) {
        return filepath;
    }
    return filepath.substr(backslash_index + 1);
}

/**
 * \brief Extracts the file name without its extension.
 * \param filepath The full file path.
 * \return The file name without its extension.
 */
export std::wstring extract_file_stem(const std::wstring& filepath) {
    std::wstring filename = extract_path_filename(filepath);
    size_t extension_position = filename.find_last_of(L'.');
    if (extension_position == std::wstring::npos || extension_position == 0) {
        return filename;
    }
    return filename.substr(0, extension_position);
}

/**
 * \brief Extracts the lowercase file extension.
 * \param filepath The full file path.
 * \return The lowercase file extension, or an empty string if none exists.
 */
export std::wstring extract_lowercase_extension(const std::wstring& filepath) {
    size_t extension_position = filepath.find_last_of(L'.');
    if (extension_position == std::wstring::npos) {
        return L"";
    }

    std::wstring extension = filepath.substr(extension_position);
    transform(
        extension.begin(),
        extension.end(),
        extension.begin(),
        [](wchar_t character) {
            return static_cast<wchar_t>(std::towlower(character));
        }
    );

    return extension;
}
