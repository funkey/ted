#ifndef TED_PYTHON_LOGGING_H__
#define TED_PYTHON_LOGGING_H__

#include <util/Logger.h>

namespace pyted {

extern logger::LogChannel pylog;

/**
 * Get the log level of the python wrappers.
 */
logger::LogLevel getLogLevel();

/**
 * Set the log level of the python wrappers.
 */
void setLogLevel(logger::LogLevel logLevel);

} // namespace pyted

#endif // TED_PYTHON_LOGGING_H__

