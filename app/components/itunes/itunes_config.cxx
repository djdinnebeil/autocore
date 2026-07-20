module itunes_c;
import visual;

void iTunes::set_config() {
    ifstream itunes_file(R"(.\config\itunes.ini)");
    if (!itunes_file) {
        return;
    }
    string line;
    getline(itunes_file, line);
    auto open_bracket = line.find('[');
    auto close_bracket = line.find(']');
    string value = line.substr(open_bracket + 1, close_bracket - open_bracket - 1);
    auto_start = value == "true";
    getline(itunes_file, line);
    open_bracket = line.find('[');
    close_bracket = line.find(']');
    value = line.substr(open_bracket + 1, close_bracket - open_bracket - 1);
    tab_end = stoi(value);
    itunes_file.close();
}
