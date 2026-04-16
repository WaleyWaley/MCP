#pragma once

class LogEvent;

/* Abstract base class for appenders */

// 一个抽象类，用来当一个接口
class AppenderFacade{

protected:
    AppenderFacade() = default;
    auto operator=(const AppenderFacade&) -> AppenderFacade& = delete;
    auto operator=(AppenderFacade&&) -> AppenderFacade& = delete;

public:
    AppenderFacade(const AppenderFacade&) = default;
    AppenderFacade(AppenderFacade&&) = default;

    virtual void log(const LogEvent& event) = 0;    //重点：提供了一个纯虚函数（接口），用于定义日志追加器的行为。

    virtual ~AppenderFacade() = default;
    
};

