/**
 * \file server_data_detail.hpp
 * \brief Pure resolution of Server directory and scalar data files.
 *
 * `port.id` and `document_root.id` have no compiled runtime substitute.
 * Seed text lives in `defaults.ixx` and is written only by `server_editor.exe`.
 */
#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace server::data {

    enum class DocumentRootStartup {
        serve,
        fail
    };

    [[nodiscard]] std::optional<int> parse_port(std::string_view text);

    [[nodiscard]] std::optional<std::string> parse_document_root(std::string_view text);

    [[nodiscard]] bool document_root_is_absolute(std::string_view stored);

    /**
     * \brief Resolves `[server] directory` against the installation root.
     *
     * A relative path joins `installation_root`. An absolute path is used
     * as-is. A missing, empty, or unusable value uses
     * `<installation_root>/components/server`. The directory is not created.
     */
    [[nodiscard]] std::filesystem::path resolve_directory(
        const std::optional<std::string_view>& stored,
        const std::filesystem::path& installation_root
    );

    /**
     * \brief Resolves a validated `document_root.id` value.
     *
     * A relative value joins `data_directory`. An absolute value is used as-is.
     */
    [[nodiscard]] std::filesystem::path resolve_document_root(
        std::string_view stored,
        const std::filesystem::path& data_directory
    );

    /**
     * \brief The document root the builder may create, when the scalar is a
     *        relative path that stays inside `data_directory`.
     *
     * Absolute paths and values such as `..\other` are refused.
     */
    [[nodiscard]] std::optional<std::filesystem::path> builder_document_root(
        std::string_view stored,
        const std::filesystem::path& data_directory
    );

    /**
     * \brief What runtime does after the scalar itself has been validated.
     *
     * An existing directory is served as-is. Starter files are not consulted.
     * A missing directory fails. Runtime does not create it and does not
     * launch `server_builder.exe`.
     */
    [[nodiscard]] DocumentRootStartup document_root_startup(
        bool directory_exists
    ) noexcept;

} // namespace server::data
