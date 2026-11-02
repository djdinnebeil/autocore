/**
 * \file writer_text_commands.ixx
 * \brief Generated Writer text-file commands.
 *
 * `writer_builder.exe` scans `<writer directory>/text` and replaces
 * `<writer directory>/insert_text.list`. `writer_ac.exe` reads that catalog.
 * File bytes are read when a command runs.
 */
export module writer_text_commands;

import std;

export namespace writer::text_commands {

struct TextCommand {
    std::string name;
    std::string filename;
};

struct CatalogResult {
    bool missing = false;
    bool unavailable = false;
    std::vector<TextCommand> commands;
    std::vector<std::string> issues;
};

[[nodiscard]] std::filesystem::path text_directory();

[[nodiscard]] std::filesystem::path catalog_path();

[[nodiscard]] CatalogResult build_catalog();

[[nodiscard]] CatalogResult read_catalog();

[[nodiscard]] bool write_catalog(std::span<const TextCommand> commands);

/**
 * \brief Reads a text file as unmodified bytes.
 *
 * The same read used by the Writer text menu: binary input, no trimming,
 * and no BOM removal. An empty file is an empty string.
 */
[[nodiscard]] std::optional<std::string> read_text_file(
    const std::filesystem::path& file
);

} // namespace writer::text_commands
