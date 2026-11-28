/**
 * \file main.cxx
 * \brief On-demand log merge for auto_core_logger.exe.
 *
 * `--refresh` updates today's Main log. `--merge` merges every date.
 * `--shutdown` only labels a `--merge` launch.
 */
import std;
import auto_core.core.clock;
import auto_core.core.logging.config;
import auto_core.core.paths;
import auto_core.core.shell;
import auto_core.utility.log_merge;

namespace {

void print_usage() {
    std::cerr
        << "Usage: auto_core_logger.exe --refresh | --merge [--shutdown]\n";
}

[[nodiscard]]
int publish(ac::log_merge::Result result, const bool refresh) {
    if (!result.ok && refresh && result.message.empty()) {
        result.message = "Unable to update the Main log.";
    }
    ac::log_merge::report(result);
    if (!result.ok) {
        return 1;
    }
    if (refresh) {
        ac::log_merge::log_refresh_success();
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    ac::logging::config::resolve();

    bool refresh = false;
    bool merge = false;
    bool shutdown_launch = false;
    if (argv != nullptr) {
        for (int index = 1; index < argc; ++index) {
            const std::string_view argument {argv[index]};
            if (argument == "--refresh") {
                refresh = true;
            }
            else if (argument == "--merge") {
                merge = true;
            }
            else if (argument == "--shutdown") {
                shutdown_launch = true;
            }
            else {
                print_usage();
                return 1;
            }
        }
    }
    if (refresh == merge || (shutdown_launch && !merge)) {
        print_usage();
        return 1;
    }

    const auto directory = ac::logging::config::directory();
    if (refresh) {
        return publish(
            ac::log_merge::merge_once(directory, ac::clock::get_date_iso()),
            true
        );
    }

    ac::log_merge::log_oneshot(shutdown_launch);
    return publish(ac::log_merge::merge_once(directory), false);
}
