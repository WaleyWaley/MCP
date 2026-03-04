#include "logger/Logger.h"
#include "logger/LoggerAppender.h"
#include

/*============================Logger==================================*/
// Logger::Logger(std::string name) : name_(name){}

// Logger::setLevel(LogLevel::Level level) {level_ = level;}

void Logger::addAppender(std::shared_ptr<Appender> appender){
    appenders_.push_back(appender);
}

void Logger::delAppender(std::shared_ptr<Appender> appender){
    for(auto it = appenders_.begin(); it != appenders_.end(); ++it){
        if(*it == appender){
            appenders_.erase(it);
            break;
        }
    }
}

void Logger::clearAppender(){
    appenders_.clear();
}

// 这个函数是对外暴露的接口，用户调用这个函数来输出日志事件，它会根据日志级别判断是否需要输出，并将日志事件传递给所有的Appender进行处理
void Logger::log(const LogEvent& event) const {
    if(event.getLevel() >= level_){
        for(auto appender : appenders_){
            appender->append(event);
        }
    }
}



