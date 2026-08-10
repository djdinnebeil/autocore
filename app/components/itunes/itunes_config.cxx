module itunes_c;
import std;

void iTunes::set_config() {
    std::ifstream itunes_file(R"(.\config\itunes.ini)");

    if (!itunes_file) {
        return;
    }

    const auto read_bracket_value =
        [](std::istream& input) -> std::optional<std::string> {
        std::string line;

        if (!std::getline(input, line)) {
            return std::nullopt;
        }

        const std::size_t open_bracket =
            line.find('[');

        if (open_bracket == std::string::npos) {
            return std::nullopt;
        }

        const std::size_t close_bracket =
            line.find(']', open_bracket + 1);

        if (close_bracket == std::string::npos) {
            return std::nullopt;
        }

        return line.substr(
            open_bracket + 1,
            close_bracket - open_bracket - 1
        );
        };

    if (const auto value = read_bracket_value(itunes_file)) {
        auto_start = *value == "true";
    }

    if (const auto value = read_bracket_value(itunes_file)) {
        try {
            std::size_t parsed_length = 0;

            const int parsed_tab_end =
                std::stoi(*value, &parsed_length);

            if (parsed_length == value->size()) {
                tab_end = parsed_tab_end;
            }
        }
        catch (const std::invalid_argument&) {
            // Keep the existing/default tab_end value.
        }
        catch (const std::out_of_range&) {
            // Keep the existing/default tab_end value.
        }
    }
}