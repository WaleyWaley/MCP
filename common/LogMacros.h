#pragma

#include "logger/Logger.h"
#include "logger/LogLevel.h"
#include <format>


#define LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level) \
        LogEventWrap(Sptr(new LogEvent("Root", level, 0, (), "Thread Name", time(0))))