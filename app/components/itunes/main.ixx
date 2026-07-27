/**
\file itunes_x.ixx
\brief Declarations for iTunes logging and process initialization.
*/
export module itunes_x;
import logger;
import logger_c;

export ac::Logger iTunes_logger("itunes");
export void update_iTunes_logger();
export void log_init();
