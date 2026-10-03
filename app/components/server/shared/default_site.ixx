/**
 * \file default_site.ixx
 * \brief Compiled starter page for the Server document root.
 *
 * `server_builder.exe` writes these strings only when the matching file
 * is missing. `defaults.ixx` stays limited to `config/server.ini`.
 */
export module server_default_site;

import std;

export namespace server::default_site {

    constexpr std::string_view index_html =
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "<meta charset=\"utf-8\">\n"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        "<title>Server site</title>\n"
        "<link rel=\"stylesheet\" href=\"styles.css\">\n"
        "</head>\n"
        "<body>\n"
        "<main>\n"
        "<h1>This folder is the generated site</h1>\n"
        "<p>Server serves the document root named by <code>document_root.id</code> "
        "on <code>127.0.0.1</code>. The seeded value <code>site</code> is this "
        "folder, resolved under the Server data directory. There is no LAN "
        "access and no authentication.</p>\n"
        "<h2>Replace these starter files</h2>\n"
        "<p><code>index.html</code> and <code>styles.css</code> are starters. "
        "Replace them, or add your own files beside them. Creation writes a "
        "file only when it is missing, so an edited page stays as it was "
        "left.</p>\n"
        "<h2>Choose another document root</h2>\n"
        "<p><code>server_editor.exe</code> writes <code>document_root.id</code>. "
        "A relative value stays under the Server data directory. An absolute "
        "path is served as-is and is not modified by <code>server_builder.exe</code>.</p>\n"
        "<p>Do not put secrets here. Anything reachable through the configured "
        "document root can be opened in a browser on this computer.</p>\n"
        "</main>\n"
        "</body>\n"
        "</html>\n";

    constexpr std::string_view styles_css =
        ":root {\n"
        "    color-scheme: light dark;\n"
        "    font-family: \"Segoe UI\", sans-serif;\n"
        "    line-height: 1.5;\n"
        "}\n"
        "\n"
        "body {\n"
        "    margin: 0;\n"
        "    padding: 2rem 1.25rem;\n"
        "}\n"
        "\n"
        "main {\n"
        "    max-width: 38rem;\n"
        "}\n"
        "\n"
        "code {\n"
        "    font-family: Consolas, monospace;\n"
        "}\n";

} // namespace server::default_site
