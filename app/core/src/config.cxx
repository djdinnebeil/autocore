module config;

import std;
import encoding;
import paths;

import <Windows.h>;

namespace {

    struct ConfigData {
        std::string configuration_log =
            "Configuration log:\n";

        std::wstring executable_directory;
        std::wstring program_title;

        bool use_keymap_file = false;
        bool keymap_trace_enabled = false;

        bool start_server = false;
        int port_number = 0;

        int end_of_day = 0;

        std::string logger_directory;
        bool send_logg_to_cout = false;

        std::unordered_map<std::string, int>
            taskbar_positions;

        std::unordered_map<int, std::string>
            taskbar_programs;

        ConfigData();

    private:
        void load_keymap_config();
        void load_server_config();
        void load_clock_config();
        void load_logger_config();
        void load_taskbar_config();
    };

    const ConfigData& config_data() {
        static const ConfigData data;
        return data;
    }

    /**
     * \brief Gets the executable directory.
     * \return std::wstring for executable directory
     */
    std::wstring get_executable_directory() {
        constexpr std::size_t max_buffer_size =
            (std::numeric_limits<DWORD>::max)();

        std::wstring buffer(260, L'\0');

        while (true) {
            if (buffer.size() > max_buffer_size) {
                return L".";
            }

            const DWORD size = GetModuleFileNameW(
                nullptr,
                buffer.data(),
                static_cast<DWORD>(buffer.size())
            );

            if (size == 0) {
                return L".";
            }

            if (size < buffer.size()) {
                buffer.resize(size);

                const std::filesystem::path executable_path {
                    buffer
                };

                const std::filesystem::path parent_path =
                    executable_path.parent_path();

                if (parent_path.empty()) {
                    return L".";
                }

                return parent_path.wstring();
            }

            if (buffer.size() > max_buffer_size / 2) {
                return L".";
            }

            buffer.resize(buffer.size() * 2);
        }
    }

    std::optional<std::string> read_bracket_value(
        std::istream& input
    ) {
        std::string line;

        if (!std::getline(input, line)) {
            return std::nullopt;
        }

        const std::size_t open_bracket = line.find('[');
        const std::size_t close_bracket =
            line.find(']', open_bracket);

        if (open_bracket == std::string::npos ||
            close_bracket == std::string::npos ||
            close_bracket <= open_bracket) {
            return std::nullopt;
        }

        return line.substr(
            open_bracket + 1,
            close_bracket - open_bracket - 1
        );
    }

    std::optional<int> parse_int(
        const std::string& value
    ) {
        try {
            std::size_t parsed_length = 0;

            const int result =
                std::stoi(value, &parsed_length);

            if (parsed_length != value.size()) {
                return std::nullopt;
            }

            return result;
        }
        catch (const std::invalid_argument&) {
            return std::nullopt;
        }
        catch (const std::out_of_range&) {
            return std::nullopt;
        }
    }

    std::optional<int> read_bracket_int(
        std::istream& input
    ) {
        const std::optional<std::string> value =
            read_bracket_value(input);

        if (!value) {
            return std::nullopt;
        }

        return parse_int(*value);
    }

    std::optional<bool> read_bracket_bool(
        std::istream& input
    ) {
        const std::optional<std::string> value =
            read_bracket_value(input);

        if (!value) {
            return std::nullopt;
        }

        if (*value == "true") {
            return true;
        }

        if (*value == "false") {
            return false;
        }

        return std::nullopt;
    }

    struct TaskbarEntry {
        int position;
        std::string program;
    };

    std::optional<TaskbarEntry> parse_taskbar_entry(
        const std::string& line
    ) {
        const std::size_t first_space = line.find(' ');
        const std::size_t last_space = line.rfind(' ');

        if (first_space == std::string::npos ||
            last_space == std::string::npos ||
            last_space == line.size() - 1) {
            return std::nullopt;
        }

        const std::optional<int> parsed_position =
            parse_int(line.substr(0, first_space));

        if (!parsed_position) {
            return std::nullopt;
        }

        int position = *parsed_position;

        if (position == 10) {
            position = 0;
        }

        return TaskbarEntry {
            position,
            line.substr(last_space + 1)
        };
    }

    std::wstring get_program_title() {
        std::ifstream app_file(R"(.\config\app.ini)");

        const std::optional<std::string> value =
            read_bracket_value(app_file);

        if (!value) {
            return L"";
        }

        return ac::encoding::to_utf16(*value);
    }

    ConfigData::ConfigData()
        : executable_directory(get_executable_directory()) {
        SetCurrentDirectoryW(executable_directory.c_str());

        program_title = get_program_title();
        SetConsoleTitleW(program_title.c_str());

        load_keymap_config();
        load_server_config();
        load_clock_config();
        load_logger_config();
        load_taskbar_config();
    }

    void ConfigData::load_keymap_config() {
        std::ifstream keymap_config_file(
            R"(.\config\keymap_config.ini)"
        );

        const std::optional<bool> use_file =
            read_bracket_bool(keymap_config_file);

        const std::optional<bool> trace_enabled =
            read_bracket_bool(keymap_config_file);

        if (!use_file || !trace_enabled) {
            return;
        }

        use_keymap_file = *use_file;
        keymap_trace_enabled = *trace_enabled;

        configuration_log += "keymap values set\n";
    }

    /**
     * \brief Loads server configuration settings from the server.ini file.
     */
    void ConfigData::load_server_config() {
        std::ifstream server_file(
            R"(.\config\server.ini)"
        );

        const std::optional<bool> enabled =
            read_bracket_bool(server_file);

        const std::optional<int> port =
            read_bracket_int(server_file);

        if (!enabled || !port) {
            return;
        }

        start_server = *enabled;
        port_number = *port;

        configuration_log += "server values set\n";
    }

    /**
     * \brief Loads clock configuration settings from the clock.ini file.
     */
    void ConfigData::load_clock_config() {
        std::ifstream clock_file(
            R"(.\config\clock.ini)"
        );

        const std::optional<int> value =
            read_bracket_int(clock_file);

        if (!value) {
            return;
        }

        end_of_day = *value;

        configuration_log += "clock values set\n";
    }

    /**
     * \brief Loads logger configuration settings from the logger.ini file.
     */
    void ConfigData::load_logger_config() {
        std::ifstream logger_file(
            ac::paths::config_directory() /
            "logger.ini"
        );

        const std::optional<bool> send_to_console =
            read_bracket_bool(logger_file);

        const std::optional<std::string> directory =
            read_bracket_value(logger_file);

        if (!send_to_console || !directory) {
            return;
        }

        send_logg_to_cout = *send_to_console;
        logger_directory = *directory;

        configuration_log += "logger values set\n";
    }

    /**
     * \brief Sets the taskbar positions based on the configuration file.
     */
    void ConfigData::load_taskbar_config() {
        std::ifstream taskbar_config_file(
            R"(.\config\taskbar.ini)"
        );

        std::string line;

        while (std::getline(taskbar_config_file, line)) {
            const std::optional<TaskbarEntry> entry =
                parse_taskbar_entry(line);

            if (!entry) {
                continue;
            }

            taskbar_positions[entry->program] =
                entry->position;

            taskbar_programs[entry->position] =
                entry->program;
        }

        configuration_log += "taskbar values set\n";
    }

}

namespace ac::config {

    std::string_view configuration_log() noexcept {
        return config_data().configuration_log;
    }

    std::wstring_view executable_directory() noexcept {
        return config_data().executable_directory;
    }

    std::wstring_view program_title() noexcept {
        return config_data().program_title;
    }

    bool use_keymap_file() noexcept {
        return config_data().use_keymap_file;
    }

    bool keymap_trace_enabled() noexcept {
        return config_data().keymap_trace_enabled;
    }

    bool start_server() noexcept {
        return config_data().start_server;
    }

    int port_number() noexcept {
        return config_data().port_number;
    }

    int end_of_day() noexcept {
        return config_data().end_of_day;
    }

    std::string_view logger_directory() noexcept {
        return config_data().logger_directory;
    }

    bool send_logg_to_cout() noexcept {
        return config_data().send_logg_to_cout;
    }

    std::optional<int> taskbar_position(
        const std::string_view program
    ) {
        const auto& positions =
            config_data().taskbar_positions;

        const auto iterator =
            positions.find(std::string {program});

        if (iterator == positions.end()) {
            return std::nullopt;
        }

        return iterator->second;
    }

    std::optional<std::string_view> taskbar_program(
        const int position
    ) {
        const auto& programs =
            config_data().taskbar_programs;

        const auto iterator = programs.find(position);

        if (iterator == programs.end()) {
            return std::nullopt;
        }

        return iterator->second;
    }
}
