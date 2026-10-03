/**
 * \file vault_mutate.ixx
 * \brief Protect a Dash secret and publish the vault.
 *
 * Compiled only into `dash_editor.exe`. Directory creation and atomic
 * replacement live here so the runtime cannot call them.
 */
module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <dpapi.h>

export module dash_vault_mutate;

import std;
import dash_vault;

#pragma comment(lib, "Crypt32.lib")

namespace {

class LocalBuffer {
public:
    LocalBuffer(BYTE* data, DWORD size) noexcept : data_(data), size_(size) {}
    ~LocalBuffer() {
        if (data_ != nullptr) {
            SecureZeroMemory(data_, size_);
            LocalFree(data_);
        }
    }

    LocalBuffer(const LocalBuffer&) = delete;
    LocalBuffer& operator=(const LocalBuffer&) = delete;

    BYTE* data() const noexcept { return data_; }
    DWORD size() const noexcept { return size_; }

private:
    BYTE* data_{};
    DWORD size_{};
};

template <typename T>
void write_value(std::ostream& output, const T& value) {
    output.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

} // namespace

export namespace dash::vault {

std::vector<std::byte> protect(std::wstring_view plaintext) {
    const auto byte_count = plaintext.size() * sizeof(wchar_t);
    if (byte_count == 0 ||
        byte_count > (std::numeric_limits<DWORD>::max)()) {
        throw std::runtime_error("The secret value is empty or too large.");
    }

    DATA_BLOB input{
        static_cast<DWORD>(byte_count),
        reinterpret_cast<BYTE*>(const_cast<wchar_t*>(plaintext.data()))};
    DATA_BLOB output{};
    if (!CryptProtectData(&input, L"Auto Core dash secret", nullptr, nullptr,
                          nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output)) {
        throw std::runtime_error("Windows could not protect the secret value.");
    }

    LocalBuffer protected_data(output.pbData, output.cbData);
    return {reinterpret_cast<std::byte*>(protected_data.data()),
            reinterpret_cast<std::byte*>(protected_data.data()) +
                protected_data.size()};
}

void save_vault(const std::filesystem::path& path,
                       const std::vector<SecretRecord>& records) {
    std::filesystem::create_directories(path.parent_path());

    const auto temporary = path.wstring() + L".new";
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Unable to create the dash vault.");
    }

    write_value(output, magic);
    write_value(output, version);
    write_value(output, static_cast<std::uint32_t>(records.size()));
    for (const auto& record : records) {
        const auto name_size = static_cast<std::uint32_t>(record.name.size());
        const auto blob_size =
            static_cast<std::uint32_t>(record.protected_value.size());
        write_value(output, name_size);
        output.write(reinterpret_cast<const char*>(record.name.data()),
                     static_cast<std::streamsize>(name_size) * sizeof(wchar_t));
        write_value(output, blob_size);
        output.write(reinterpret_cast<const char*>(record.protected_value.data()),
                     blob_size);
    }
    output.flush();
    if (!output) {
        throw std::runtime_error("Unable to write the dash vault.");
    }
    output.close();

    if (!MoveFileExW(temporary.c_str(), path.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(temporary.c_str());
        throw std::runtime_error("Unable to replace the dash vault.");
    }
}

} // namespace dash::vault
