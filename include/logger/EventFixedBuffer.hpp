#pragma once
#include <array>
#include <span>
#include "LogEvent.h"

// 定义缓冲区大小：储存 64 个 LogEvent。
constexpr size_t c_k_event_count = 64;

template <size_t N = c_k_event_count>
class EventFixedBuffer
{
public:
    using EventArray = std::array<LogEvent, N>;
    // 记录缓冲区里面放了几个LogEvent
    EventFixedBuffer():count_(0){}
    // 尝试将 LogEvent 写入缓冲区
    auto append(LogEvent event) -> bool
    {
        if(count_ < c_k_event_count)
        {
            data_[count_] = std::move(event);    // 将 LogEvent 移动至数组，避免字符串深拷贝
            count_++;
            return true;
        }
        return false; // 缓冲区已满
    }

    // 已写入的事件数量
    [[nodiscard]] size_t count() const {return count_;}

    // 剩余可用空间（事件数）
    [[nodiscard]] auto available() const -> size_t {return N - count_;}

    // 获取连续内存的“透明视窗”，原生支持ranges的for_each
    [[nodiscard]] std::span<const LogEvent> getEventSpan() const {return std::span<const LogEvent>(data_.data(), count_);}
    
    // 清空缓冲区
    void reset() {count_ = 0;}
    
private:
    EventArray data_;
    size_t count_ = 0;  // 当前存储的事件数量，代替了原来的字节偏移
};