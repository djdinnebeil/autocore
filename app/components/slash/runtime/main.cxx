/**
 * \file main.cxx
 * \brief Reports and deletes Recycle Bin contents.
 *
 * This component scans the Recycle Bin and organizes its contents into files,
 * folders, archives, and music files. Music metadata is retrieved with TagLib.
 * The scan runs first when a report needs it. The Recycle Bin is then emptied.
 * The report is copied to the clipboard and displayed after that empty succeeds.
 */
import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.ini;
import auto_core.core.paths;
import command_registry;
import slash_protocol;
import slash_defaults;

import music;
import path_utils;

import <atlbase.h>;
import <shlobj.h>;
import <shlwapi.h>;
import auto_core.core.shell;

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Shlwapi.lib")


ac::Component slash_component(
    "slash",
    ac::logging::config::LoggingScope {"slash"}
);

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
     * \brief Builds the categorized Recycle Bin report.
     */
    std::wstring verbose_recycle_bin_report() {
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

        return output.str();
    }

    std::size_t count_recycle_bin_items() {
        std::size_t count = 0;
        const HRESULT com_result = CoInitialize(NULL);
        CComPtr<IShellFolder> desktop_folder;
        if (SUCCEEDED(SHGetDesktopFolder(&desktop_folder))) {
            LPITEMIDLIST recycle_bin_pidl = nullptr;
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
                        LPITEMIDLIST item_pidl = nullptr;
                        while (item_enumerator->Next(1, &item_pidl, nullptr) == S_OK) {
                            ++count;
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
        return count;
    }

    enum class ReportMode {
        verbose,
        concise,
        silent
    };

    ReportMode configured_mode() {
        const auto ini_path = ac::paths::config_directory() / "slash.ini";
        const auto document = ac::ini::read(ini_path);
        if (!document) {
            std::error_code exists_error;
            const bool present =
                std::filesystem::exists(ini_path, exists_error);
            slash_component.report_ini_unavailable(present && !exists_error);
            return ReportMode::verbose;
        }
        const auto mode = document->find("slash", "mode");
        if (!mode || !slash::defaults::is_mode(*mode)) {
            slash_component.log_print(
                "config/slash.ini mode is missing or invalid. Using verbose."
            );
            return ReportMode::verbose;
        }
        if (*mode == slash::defaults::mode_concise) {
            return ReportMode::concise;
        }
        if (*mode == slash::defaults::mode_silent) {
            return ReportMode::silent;
        }
        return ReportMode::verbose;
    }

    /**
     * \brief True when the shell reports no Recycle Bin items.
     *
     * `std::nullopt` means the count is unknown. `i64NumItems` of `-1` is
     * the shell's unknown value and is not treated as empty.
     */
    std::optional<bool> recycle_bin_is_empty() {
        SHQUERYRBINFO info {};
        info.cbSize = sizeof(info);
        const HRESULT query_result = SHQueryRecycleBin(nullptr, &info);
        if (FAILED(query_result) || info.i64NumItems < 0) {
            return std::nullopt;
        }
        return info.i64NumItems == 0;
    }

    enum class EmptyResult {
        emptied,
        already_empty
    };

    EmptyResult empty_recycle_bin() {
        const HRESULT empty_result = SHEmptyRecycleBin(
            NULL,
            NULL,
            SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND
        );
        // An empty bin returns E_UNEXPECTED (0x8000FFFF) from this call.
        if (empty_result == E_UNEXPECTED) {
            return EmptyResult::already_empty;
        }
        if (FAILED(empty_result)) {
            const auto message = std::format(
                "Failed to empty the Recycle Bin ({:#010x})",
                static_cast<std::uint32_t>(empty_result)
            );
            slash_component.log_print("{}", message);
            throw std::runtime_error(message);
        }
        return EmptyResult::emptied;
    }

    /**
     * \brief Reports and deletes the contents of the Recycle Bin.
     *
     * Enumeration runs only when the selected mode needs it and the bin
     * has items. The bin is emptied after that scan. Success text is
     * inserted only after the empty succeeds. `silent` prints a console
     * line and does not insert text.
     */
    void report_and_empty_recycle_bin() {
        const ReportMode mode = configured_mode();
        if (const auto already_empty = recycle_bin_is_empty();
            already_empty && *already_empty) {
            slash_component.print("Recycle bin is empty");
            return;
        }

        std::wstring verbose_report;
        std::size_t item_count = 0;
        if (mode == ReportMode::verbose) {
            verbose_report = verbose_recycle_bin_report();
        }
        else if (mode == ReportMode::concise) {
            item_count = count_recycle_bin_items();
        }

        if (empty_recycle_bin() == EmptyResult::already_empty) {
            slash_component.print("Recycle bin is empty");
            return;
        }

        if (mode == ReportMode::silent) {
            slash_component.print("Recycle bin emptied");
            return;
        }
        if (mode == ReportMode::concise) {
            slash_component.printnl_and_insert_text_replacing_clipboard(
                std::format(L"Recycle Bin emptied: {} items", item_count)
            );
            return;
        }
        slash_component.printnl_and_insert_text_replacing_clipboard(
            verbose_report
        );
    }

    command_registry::Registry create_slash_command_registry() {
        command_registry::Registry registry;
        registry.add(
            std::string {
                slash::commands::report_and_empty_recycle_bin.name
            },
            &report_and_empty_recycle_bin
        );
        return registry;
    }

}

/**
 * \brief Runs the slash component.
 * \return Exit code of the process.
 */
int main(int argument_count, char* arguments[]) {
    ac::shell::set_process_app_user_model_id();
    const auto registry = create_slash_command_registry();

    const std::string_view command_name = argument_count == 2
        ? std::string_view {arguments[1]}
        : slash::commands::report_and_empty_recycle_bin.name;

    try {
        auto action = registry.resolve(command_name);
        if (!action) {
            slash_component.log_print(
                "Unknown Slash command: {}",
                command_name
            );
            return 1;
        }
        action();
    }
    catch (const std::exception& exception) {
        slash_component.print(
            "caught exception: {}\n",
            exception.what()
        );
        return 1;
    }
    catch (...) {
        slash_component.print("uncaught exception\n");
        return 1;
    }
    return 0;
}
