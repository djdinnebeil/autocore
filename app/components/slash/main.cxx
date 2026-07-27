/**
 * \file main.cxx
 * \brief Reports and deletes Recycle Bin contents.
 *
 * This component scans the Recycle Bin and organizes its contents into files,
 * folders, archives, and music files. Music metadata is retrieved with TagLib.
 * The resulting report is copied to the clipboard and displayed before the
 * Recycle Bin is emptied.
 */
import std;
import config;
import clipboard;
import logger;
import music;
import path_utils;
import print;
import utils;
import <atlbase.h>;
import <shlobj.h>;
import <shlwapi.h>;

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Shlwapi.lib")

namespace {

    bool is_archive_extension(const std::wstring& extension) {
        return
            extension == L".rar" ||
            extension == L".zip" ||
            extension == L".7z" ||
            extension == L".tar" ||
            extension == L".gz" ||
            extension == L".bz2" ||
            extension == L".cab";
    }

    bool is_music_extension(const std::wstring& extension) {
        return extension == L".mp3" || extension == L".m4a";
    }

    /**
     * \brief Reports and deletes the contents of the Recycle Bin.
     */
    void report_and_empty_recycle_bin() {
        HRESULT com_result = CoInitialize(NULL);
        std::wostringstream recycle_bin_contents;
        std::wostringstream recycle_bin_filenames;
        std::wostringstream recycle_bin_folders;
        std::wostringstream recycle_bin_archive_files;
        std::wostringstream music_filenames;
        bool files_detected = false;
        bool folders_detected = false;
        bool archive_files_detected = false;
        bool music_files_detected = false;

        recycle_bin_contents << L"Recycle bin contents:\n";
        recycle_bin_filenames << L"---Files---\n";
        recycle_bin_folders << L"---Folders---\n";
        recycle_bin_archive_files << L"---Archive files---\n";
        music_filenames << L"---Music files---\n";

        CComPtr<IShellFolder> desktop_folder;
        if (SUCCEEDED(SHGetDesktopFolder(&desktop_folder))) {
            LPITEMIDLIST recycle_bin_pidl;
            if (SUCCEEDED(SHGetSpecialFolderLocation(
                NULL,
                CSIDL_BITBUCKET,
                &recycle_bin_pidl
            ))) {
                CComPtr<IShellFolder> recycle_bin_folder;
                if (SUCCEEDED(desktop_folder->BindToObject(
                    recycle_bin_pidl,
                    NULL,
                    IID_IShellFolder,
                    reinterpret_cast<void**>(&recycle_bin_folder)
                ))) {
                    CComPtr<IEnumIDList> item_enumerator;
                    if (SUCCEEDED(recycle_bin_folder->EnumObjects(
                        NULL,
                        SHCONTF_FOLDERS | SHCONTF_NONFOLDERS,
                        &item_enumerator
                    ))) {
                        LPITEMIDLIST item_pidl;
                        while (item_enumerator->Next(1, &item_pidl, NULL) != S_FALSE) {
                            std::wstring filepath;
                            STRRET display_name;

                            if (SUCCEEDED(recycle_bin_folder->GetDisplayNameOf(
                                item_pidl,
                                SHGDN_NORMAL,
                                &display_name
                            ))) {
                                WCHAR display_name_buffer[MAX_PATH];
                                StrRetToBufW(
                                    &display_name,
                                    item_pidl,
                                    display_name_buffer,
                                    MAX_PATH
                                );
                                filepath = display_name_buffer;
                                recycle_bin_contents << filepath << L'\n';

                                SFGAOF attributes = SFGAO_FOLDER;
                                HRESULT attributes_result =
                                    recycle_bin_folder->GetAttributesOf(
                                        1,
                                        const_cast<LPCITEMIDLIST*>(&item_pidl),
                                        &attributes
                                    );

                                if (SUCCEEDED(attributes_result)) {
                                    if (attributes & SFGAO_FOLDER) {
                                        size_t extension_position =
                                            filepath.find_last_of(L'.');

                                        if (extension_position == std::wstring::npos) {
                                            folders_detected = true;
                                            recycle_bin_folders
                                                << extract_path_filename(filepath)
                                                << L'\n';
                                        }
                                        else {
                                            std::wstring extension =
                                                extract_lowercase_extension(filepath);

                                            if (is_archive_extension(extension)) {
                                                archive_files_detected = true;
                                                recycle_bin_archive_files
                                                    << extract_file_stem(filepath)
                                                    << L'\n';
                                            }
                                            else {
                                                folders_detected = true;
                                                recycle_bin_folders
                                                    << extract_path_filename(filepath)
                                                    << L'\n';
                                            }
                                        }
                                    }
                                    else {
                                        std::wstring extension =
                                            extract_lowercase_extension(filepath);

                                        if (is_music_extension(extension)) {
                                            music_files_detected = true;
                                            if (filepath.ends_with(L".MP3")) {
                                                music_filenames << L"Bonus found: ";
                                            }

                                            if (SUCCEEDED(
                                                recycle_bin_folder->GetDisplayNameOf(
                                                    item_pidl,
                                                    SHGDN_FORPARSING,
                                                    &display_name
                                                )
                                            )) {
                                                TCHAR parsing_path[MAX_PATH];
                                                StrRetToBuf(
                                                    &display_name,
                                                    item_pidl,
                                                    parsing_path,
                                                    MAX_PATH
                                                );
                                                music_filenames
                                                    << format_music_file(
                                                        parsing_path,
                                                        filepath
                                                    );
                                            }
                                        }

                                        files_detected = true;
                                        std::wstring filename = extract_file_stem(filepath);
                                        if (filename.empty()) {
                                            filename = extract_path_filename(filepath);
                                        }
                                        recycle_bin_filenames << filename << L'\n';
                                    }
                                }
                            }

                            CoTaskMemFree(item_pidl);
                        }
                    }
                }

                CoTaskMemFree(recycle_bin_pidl);
            }
        }

        if (SUCCEEDED(com_result)) {
            CoUninitialize();
        }

        HRESULT empty_result = SHEmptyRecycleBin(
            NULL,
            NULL,
            SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND
        );

        std::wostringstream output;
        if (
            files_detected ||
            folders_detected ||
            archive_files_detected ||
            music_files_detected
        ) {
            output << recycle_bin_contents.str();
            if (files_detected) {
                output << recycle_bin_filenames.str();
            }
            if (folders_detected) {
                output << recycle_bin_folders.str();
            }
            if (archive_files_detected) {
                output << recycle_bin_archive_files.str();
            }
            if (music_files_detected) {
                output << music_filenames.str();
            }
            output << L'\n';
        }
        else {
            output << L"\n\n";
        }

        ac::clipboard::set_clipboard_text(output.str());
        ac::printnl(output.str());
        ac::clipboard::paste_from_clipboard();
    }

}

/**
 * \brief Runs the slash component.
 * \return Exit code of the process.
 */
int main() {
    ac::logger::update_main_log_file();

    try {
        report_and_empty_recycle_bin();
    }
    catch (const std::exception& exception) {
        ac::print("caught exception: {}\n", exception.what());
    }
    catch (...) {
        ac::print("uncaught exception\n");
    }

    ac::logger::close_main_log_file();
    return 0;
}
