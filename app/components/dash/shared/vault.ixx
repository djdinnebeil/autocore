/**
 * \file vault.ixx
 * \brief Dash vault format and read-side loading.
 *
 * Path resolution does not create the vault directory. A missing file is an
 * empty vault. This module does not protect, decrypt, or save.
 */
module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <knownfolders.h>
#include <shlobj.h>

export module dash_vault;

import std;

namespace dash::vault {

constexpr std::uint32_t maximum_blob_bytes = 1024 * 1024;

template <typename T>
bool read_value(std::istream& input, T& value) {
    return static_cast<bool>(input.read(
        reinterpret_cast<char*>(&value), sizeof(value)));
}

} // namespace dash::vault

export namespace dash::vault {

constexpr std::uint32_t magic = 0x48534144; // "DASH"
constexpr std::uint32_t version = 1;
constexpr std::uint32_t maximum_entry_count = 1'000;
constexpr std::uint32_t maximum_name_characters = 1'024;

struct SecretRecord {
    std::wstring name;
    std::vector<std::byte> protected_value;
};

void wipe(std::wstring& value) noexcept {
    if (!value.empty()) {
        SecureZeroMemory(value.data(), value.size() * sizeof(wchar_t));
        value.clear();
    }
}

void wipe(std::vector<std::byte>& value) noexcept {
    if (!value.empty()) {
        SecureZeroMemory(value.data(), value.size());
        value.clear();
    }
}

std::filesystem::path vault_path() {
    PWSTR local_app_data = nullptr;
    const HRESULT result = SHGetKnownFolderPath(
        FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &local_app_data);
    if (FAILED(result)) {
        throw std::runtime_error("Unable to find Local AppData.");
    }

    const std::filesystem::path directory =
        std::filesystem::path(local_app_data) / L"Auto Core";
    CoTaskMemFree(local_app_data);
    return directory / L"dash.vault";
}

std::vector<SecretRecord> load_vault(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return {};
    }

    std::ifstream input(path, std::ios::binary);
    std::uint32_t file_magic{}, file_version{}, count{};
    if (!read_value(input, file_magic) || !read_value(input, file_version) ||
        !read_value(input, count) || file_magic != magic ||
        file_version != version || count > maximum_entry_count) {
        throw std::runtime_error("The dash vault is invalid or unsupported.");
    }

    std::vector<SecretRecord> records;
    records.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index) {
        std::uint32_t name_size{}, blob_size{};
        if (!read_value(input, name_size) || name_size > maximum_name_characters) {
            throw std::runtime_error("The dash vault contains an invalid name.");
        }

        SecretRecord record;
        record.name.resize(name_size);
        if (!input.read(reinterpret_cast<char*>(record.name.data()),
                        static_cast<std::streamsize>(name_size) * sizeof(wchar_t)) ||
            !read_value(input, blob_size) || blob_size == 0 ||
            blob_size > maximum_blob_bytes) {
            throw std::runtime_error("The dash vault contains an invalid entry.");
        }

        record.protected_value.resize(blob_size);
        if (!input.read(reinterpret_cast<char*>(record.protected_value.data()),
                        blob_size)) {
            throw std::runtime_error("The dash vault is truncated.");
        }
        records.push_back(std::move(record));
    }

    if (input.peek() != std::char_traits<char>::eof()) {
        throw std::runtime_error("The dash vault contains unexpected data.");
    }
    return records;
}

} // namespace dash::vault
