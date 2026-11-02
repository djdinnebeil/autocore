/**
 * \file main.cxx
 * \brief Rebuilds Writer text-file commands from `<writer directory>/text`.
 */
import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.shell;
import writer_text_commands;

namespace {

ac::Component writer_builder {
    "writer_builder",
    ac::logging::config::LoggingScope {"writer"}
};

} // namespace

int main(int argc, char* argv[]) {
    (void)argv;
    ac::shell::set_process_app_user_model_id();
    writer_builder.log_info("writer_builder.exe started");
    if (argc != 1) {
        std::cerr << "Usage: writer_builder.exe\n";
        return 1;
    }

    try {
        const writer::text_commands::CatalogResult built =
            writer::text_commands::build_catalog();
        for (const std::string& issue : built.issues) {
            writer_builder.print("{}", issue);
        }
        if (built.unavailable) {
            return 1;
        }

        const std::filesystem::path path = writer::text_commands::catalog_path();
        if (!writer::text_commands::write_catalog(built.commands)) {
            writer_builder.print("Failed to write {}", path.string());
            return 1;
        }

        for (const writer::text_commands::TextCommand& command : built.commands) {
            writer_builder.print("{}", command.name);
        }
        writer_builder.print("Wrote {}.", path.string());
        writer_builder.print("Restart Writer before the catalog changes.");
        return 0;
    }
    catch (const std::exception& error) {
        writer_builder.print("{}", error.what());
        return 1;
    }
}
