#include <memory>
#include <mutex>

#include "logger/Logger.h"
#include "logger/LoggerAppender.h"
#include "logger/LogManager.h"

LoggerManager::LoggerManager() : root_{new Logger("root")}, loggers_{{"root", root_}} {
    root_->addAppender(std::make_shared<AppenderProxy<StdoutAppender>>());
    init_();
}

void LoggerManager::init_(){}

auto LoggerManager::getLogger(std::string_view logger_name) -> Sptr<Logger>{

    auto _ = std::lock_guard<std::mutex>{mtx_};

    if(auto it = loggers_.find(std::string(logger_name)); it != loggers_.end())
        return it->second;

    auto logger = std::make_shared<Logger>(std::string{logger_name});

    loggers_.emplace(logger_name, logger);

    return logger;
}
