/**
 * \file vault_decrypt.ixx
 * \brief Decrypt a Dash secret for insertion.
 *
 * Compiled only into `dash_ac.exe`.
 */
module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <dpapi.h>

export module dash_vault_decrypt;

import std;

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

} // namespace

export namespace dash::vault {

std::wstring unprotect(std::span<const std::byte> ciphertext) {
    DATA_BLOB input{
        static_cast<DWORD>(ciphertext.size()),
        reinterpret_cast<BYTE*>(const_cast<std::byte*>(ciphertext.data()))};
    DATA_BLOB output{};
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr,
                            CRYPTPROTECT_UI_FORBIDDEN, &output)) {
        throw std::runtime_error("Windows could not decrypt the secret value.");
    }

    LocalBuffer plaintext(output.pbData, output.cbData);
    if (plaintext.size() == 0 || plaintext.size() % sizeof(wchar_t) != 0) {
        throw std::runtime_error("The decrypted secret value is invalid.");
    }
    return {reinterpret_cast<wchar_t*>(plaintext.data()),
            plaintext.size() / sizeof(wchar_t)};
}

} // namespace dash::vault
