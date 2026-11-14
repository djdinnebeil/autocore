/**
 * \file entry.cxx
 * \brief Formatter entry used by `itunes_ac.exe --formatter`.
 *
 * `main` stays outside the formatter module so the linker can find it.
 */
import itunes_formatter_main;
import auto_core.core.shell;

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    return run_itunes_formatter(argc, argv);
}
