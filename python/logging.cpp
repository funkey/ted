#include "logging.h"

namespace pyted {

logger::LogChannel pylog("pylog", "[pyted] ");

logger::LogLevel getLogLevel() {
	return logger::LogManager::getGlobalLogLevel();
}

void setLogLevel(logger::LogLevel logLevel) {
	logger::LogManager::setGlobalLogLevel(logLevel);
}

} // namespace pyted
