/**
 * \file entry.cxx
 * \brief Process entry for `itunes_formatter.exe`.
 *
 * `main` stays outside the formatter module so the linker can find it.
 */
import itunes_formatter_main;
import auto_core.core.shell;

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    return run_itunes_formatter(argc, argv);
}
