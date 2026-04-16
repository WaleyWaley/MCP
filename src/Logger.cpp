#include "logger/Logger.h"
#include "logger/LoggerAppender.h"
#include "logger/LogLevel.h"

#include <algorithm>
#include <cstdlib>
/*============================Logger==================================*/
Logger::Logger(std::string name) : name_(name){}

Logger::setLevel(LogLevel::Level level) {level_ = level;}

void Logger::addAppender(std::shared_ptr<AppenderFacade> appender){
    appenders_.push_back(appender);
}

void Logger::delAppender(std::shared_ptr<AppenderFacade> appender){
    auto it = appenders_.begin();
    while(it != appenders_.end())
    {
        if(*it == appender){
            appenders_.erase(it);
        } else {
            it++;
        }
    }
}

void Logger::clearAppender(){
    appenders_.clear();
}

void Logger::log(const LogEvent& event, std::error_code &ec) const{
    ec = {};
    if(event.getLevel() < level_)
       return;  // 不算错误直接忽略
    if(appenders_.empty())
    {
        ec = LogErrcCode::NoAppender;
        return;
    }
    /** @todo 判断isLevelEnabel、是否是SYSFATAL等级*/
    
}
// 这个函数是对外暴露的接口，用户调用这个函数来输出日志事件，它会根据日志级别判断是否需要输出，并将日志事件传递给所有的Appender进行处理
void Logger::log(const LogEvent& event) const {
    if(event.getLevel() >= level_){
        // 这里是从appenders_里面遍历“皮”来调用对应的实际干活的Impl程序
        for(auto appender : appenders_){
            appender->log(event);        // 调用基类的 log 方法，实际执行的是 AppenderProxy 的 log 方法，进而调用具体的 Appender 实现的 log 方法
        }
    }
}



