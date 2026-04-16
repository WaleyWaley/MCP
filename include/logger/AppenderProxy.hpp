#pragma once

#include "AppenderFacade.h"
#include "LogFormatter.h"

class LogFormatter;
class LogEvent;


// 用模板类接住所有满足 x.log(y,z);的类
// 用concept来限制模板参数
template<typename T>
concept IsAppenderImpl = requires(T x, const LogFormatter& fmter, const LogEvent& event) {x.log(fmter, event); } -> std::same_as<void>; // 返回值是void

/**
 * @brief thread-safe, actually a proxy of the concrete appender
 */

// 也可以像下面这样写
// template<IsAppenderImpl Impl>
// requires IsAppenderImpl<Impl>
template<IsAppenderImpl Impl>
class AppenderProxy : public AppenderFacade {
public:

    AppenderProxy(): formatter_{LogFormatter{}}{}


    AppenderProxy(const AppenderProxy&) = delete;
    AppenderProxy(AppenderProxy&&) = delete;
    auto operator=(const AppenderProxy&) -> AppenderProxy& = delete;
    auto operator=(AppenderProxy&&) -> AppenderProxy& = delete; 

    // 完美转发
    template <typename... Ts>
    // 这里的Ts&&... 不是右值引用而是完美转发
    explicit AppenderProxy(LogFormatter fmter, Ts&&... ts) : formatter_(std::move(fmter)), impl_(std::forward<Ts>(ts)...){}

    // [[nodiscard]]意思是不要忽略我的返回值，如果调用者调用了这个函数，但是没有使用它的返回值。请务必给他一个警告。
    [[nodiscard]] auto GetFormater() const -> const LogFormatter&
    {
        return formatter_;
    }

    auto SetFormatterPattern(LogFormatter formatter) -> void
    {
        formatter_ = std::move(formatter);
    }

    void log(const LogEvent& event) override
    {
        impl_.log(formatter_, event);
    }

    ~AppenderProxy() override = default;

private:
    LogFormatter formatter_;
    // 这个Impl类型是必须满足log(y,z)这样的类型
    Impl impl_;
};

