/**
\file itunes_x.ixx
\brief Declarations for iTunes logging and process initialization.
*/
export module itunes_x;
import logger;
import logger_c;

export extern Logger iTunes_logger;
export void update_iTunes_logger();
export void log_init();
