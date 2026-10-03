/**
 * \file stamp_auto_core_shortcut.cxx
 * \brief Stamps an existing Auto Core shortcut with System.AppUserModel.ID.
 *
 * Obtains IPropertyStore from IShellLink, sets PKEY_AppUserModel_ID to
 * Djdinn.AutoCore, commits, and saves. Does not create a shortcut.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <propkey.h>
#include <propvarutil.h>
#include <shlobj.h>
#include <shobjidl.h>

#include <filesystem>
#include <iostream>
#include <string>

namespace {

    void release(IUnknown* pointer) noexcept {
        if (pointer != nullptr) {
            pointer->Release();
        }
    }

    [[nodiscard]]
    bool stamp_shortcut(const std::wstring& path) {
        const std::filesystem::path shortcut {path};
        std::error_code exists_error;
        if (!std::filesystem::is_regular_file(shortcut, exists_error)) {
            std::wcerr
                << L"Shortcut not found: " << path << L"\n"
                << L"Create <installation_root>\\Auto Core.lnk targeting "
                << L"<installation_root>\\bin\\auto_core.exe, then run this "
                << L"tool again.\n";
            return false;
        }

        const HRESULT initialize =
            CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(initialize)) {
            std::wcerr
                << L"CoInitializeEx failed. HRESULT = 0x"
                << std::hex << static_cast<unsigned long>(initialize)
                << std::dec << L"\n";
            return false;
        }

        IShellLinkW* link = nullptr;
        IPersistFile* file = nullptr;
        IPropertyStore* store = nullptr;
        bool saved = false;

        const HRESULT created = CoCreateInstance(
            CLSID_ShellLink,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&link)
        );
        if (FAILED(created)) {
            std::wcerr
                << L"CoCreateInstance(CLSID_ShellLink) failed. HRESULT = 0x"
                << std::hex << static_cast<unsigned long>(created)
                << std::dec << L"\n";
        }
        else {
            const HRESULT queried_file =
                link->QueryInterface(IID_PPV_ARGS(&file));
            if (FAILED(queried_file)) {
                std::wcerr
                    << L"QueryInterface(IPersistFile) failed. HRESULT = 0x"
                    << std::hex << static_cast<unsigned long>(queried_file)
                    << std::dec << L"\n";
            }
            else {
                const HRESULT loaded =
                    file->Load(path.c_str(), STGM_READWRITE);
                if (FAILED(loaded)) {
                    std::wcerr
                        << L"IPersistFile::Load failed. HRESULT = 0x"
                        << std::hex << static_cast<unsigned long>(loaded)
                        << std::dec << L"\n";
                }
                else {
                    const HRESULT queried_store =
                        link->QueryInterface(IID_PPV_ARGS(&store));
                    if (FAILED(queried_store)) {
                        std::wcerr
                            << L"QueryInterface(IPropertyStore) failed. HRESULT = 0x"
                            << std::hex
                            << static_cast<unsigned long>(queried_store)
                            << std::dec << L"\n";
                    }
                    else {
                        PROPVARIANT value {};
                        const HRESULT initialized = InitPropVariantFromString(
                            L"Djdinn.AutoCore",
                            &value
                        );
                        if (FAILED(initialized)) {
                            std::wcerr
                                << L"InitPropVariantFromString failed. HRESULT = 0x"
                                << std::hex
                                << static_cast<unsigned long>(initialized)
                                << std::dec << L"\n";
                        }
                        else {
                            const HRESULT assigned =
                                store->SetValue(PKEY_AppUserModel_ID, value);
                            PropVariantClear(&value);
                            if (FAILED(assigned)) {
                                std::wcerr
                                    << L"IPropertyStore::SetValue failed. HRESULT = 0x"
                                    << std::hex
                                    << static_cast<unsigned long>(assigned)
                                    << std::dec << L"\n";
                            }
                            else {
                                const HRESULT committed = store->Commit();
                                if (FAILED(committed)) {
                                    std::wcerr
                                        << L"IPropertyStore::Commit failed. HRESULT = 0x"
                                        << std::hex
                                        << static_cast<unsigned long>(committed)
                                        << std::dec << L"\n";
                                }
                                else {
                                    const HRESULT written =
                                        file->Save(nullptr, TRUE);
                                    if (FAILED(written)) {
                                        std::wcerr
                                            << L"IPersistFile::Save failed. HRESULT = 0x"
                                            << std::hex
                                            << static_cast<unsigned long>(written)
                                            << std::dec << L"\n";
                                    }
                                    else {
                                        saved = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        release(store);
        release(file);
        release(link);
        CoUninitialize();

        if (saved) {
            std::wcout
                << L"Stamped System.AppUserModel.ID = Djdinn.AutoCore on "
                << path << L"\n";
        }
        return saved;
    }

}

int wmain(int argc, wchar_t** argv) {
    if (argc != 2) {
        std::wcerr
            << L"Usage: stamp_auto_core_shortcut.exe <path-to-Auto-Core.lnk>\n"
            << L"Sets System.AppUserModel.ID to Djdinn.AutoCore on an existing shortcut.\n"
            << L"Does not create the shortcut.\n";
        return 1;
    }

    return stamp_shortcut(argv[1]) ? 0 : 1;
}
