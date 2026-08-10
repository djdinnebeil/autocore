export module main_log;

import std;
import pipes;

export {
	void update_main_log_file();
	void write_to_main_log(
		const ac::pipes::LogEvent& event
	);
}