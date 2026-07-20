/**
 * \file music.ixx
 * \brief Handles music-file metadata for the slash component.
 */
export module music;

import base;
import path_utils;

#pragma warning(disable : 4251)

import <taglib/fileref.h>;
import <taglib/tag.h>;

namespace {

    /**
     * \brief Extracts the artist name from a music file path.
     * \param filepath The full file path.
     * \return The artist name extracted from the path.
     */
    wstring extract_artist_from_path(const wstring& filepath) {
        size_t last_backslash_index = filepath.rfind(L'\\');
        if (last_backslash_index == wstring::npos) {
            return L"";
        }

        size_t second_last_backslash_index = filepath.rfind(
            L'\\',
            last_backslash_index - 1
        );
        if (second_last_backslash_index == wstring::npos) {
            return L"";
        }

        size_t third_last_backslash_index = filepath.rfind(
            L'\\',
            second_last_backslash_index - 1
        );
        if (third_last_backslash_index == wstring::npos) {
            return L"";
        }

        return filepath.substr(
            third_last_backslash_index + 1,
            second_last_backslash_index - third_last_backslash_index - 1
        );
    }

    /**
     * \brief Extracts the album name from a music file path.
     * \param filepath The full file path.
     * \return The album name extracted from the path.
     */
    wstring extract_album_from_path(const wstring& filepath) {
        size_t last_backslash_index = filepath.rfind(L'\\');
        if (last_backslash_index == wstring::npos) {
            return L"";
        }

        size_t second_last_backslash_index = filepath.rfind(
            L'\\',
            last_backslash_index - 1
        );
        if (second_last_backslash_index == wstring::npos) {
            return L"";
        }

        return filepath.substr(
            second_last_backslash_index + 1,
            last_backslash_index - second_last_backslash_index - 1
        );
    }

}

/**
 * \brief Reads and formats metadata for a music file.
 * \param recycled_file_path The parsing path of the recycled file.
 * \param filepath The displayed file path.
 * \return A formatted string containing the music-file metadata.
 */
export wstring format_music_file(
    const wstring& recycled_file_path,
    const wstring& filepath
) {
    TagLib::FileRef recycled_file(recycled_file_path.c_str());
    wstring name;
    wstring artist;
    wstring album;
    wstring duration_string;

    if (!recycled_file.isNull() && recycled_file.tag()) {
        TagLib::Tag* tag = recycled_file.tag();
        name = tag->title().toCWString();
        artist = tag->artist().toCWString();
        album = tag->album().toCWString();

        if (recycled_file.audioProperties()) {
            int duration_seconds =
                recycled_file.audioProperties()->lengthInSeconds();
            wss duration_stream;
            duration_stream
                << (duration_seconds / 60)
                << L":"
                << setw(2)
                << setfill(L'0')
                << (duration_seconds % 60);
            duration_string = duration_stream.str();
        }
    }

    if (name.empty()) {
        name = extract_file_stem(filepath);
    }
    if (artist.empty()) {
        artist = extract_artist_from_path(filepath);
    }
    if (album.empty()) {
        album = extract_album_from_path(filepath);
    }
    if (duration_string.empty()) {
        duration_string = L"[99:99]";
    }
    else if (duration_string == L"0:00") {
        duration_string = L"[[00:00]]";
    }

    wss output;
    output
        << L"[" << name << L"] "
        << L"[" << artist << L"] "
        << L"[" << album << L"] "
        << L"[" << duration_string << L"]\n";
    return output.str();
}
