#include "logger/Logger.h"
#include "logger/AppenderFacade.h"

void Logger::addAppender(Sptr<AppenderFacade> appender) {
    if (appender) {
        appenders_.push_back(std::move(appender));
    }
}

void Logger::delAppender(Sptr<AppenderFacade> appender) {
    if (!appender) {
        return;
    }

    for (auto it = appenders_.begin(); it != appenders_.end();) {
        if (*it == appender) {
            it = appenders_.erase(it);
        } else {
            ++it;
        }
    }
}

void Logger::clearAppender() { appenders_.clear(); }

void Logger::log(const LogEvent& event) const {
    if (!isLevelEnable(event.getLevel())) {
        return;
    }

    for (const auto& appender : appenders_) {
        if (appender) {
            appender->log(event);
        }
    }
}
