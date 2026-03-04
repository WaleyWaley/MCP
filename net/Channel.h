#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include "Timestamp.h"
#include <sys/epoll.h>

class EventLoop;

namespace{
    // 定义各种成员函数的探测概念
    template<typename T>
    concept HasHandleRead = requires(T t, Timestamp ts){
        {t.handleRead_(ts)} -> std::same_as<void>;
    };

    template<typename T>
    concept HasHandleWrite = requires(T t){
        {t.handleWrite_()} -> std::same_as<void>;
    };

    
}