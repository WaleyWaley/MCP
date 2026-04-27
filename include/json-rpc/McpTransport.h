#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include "json.hpp"

using json = nlohmann::json;

class McpTransport{
public:
    // 定义一个回调类型，用于把收到的JSON字符串交给上层路由去处理
    using MessageCallback = std::function<void(const std::string&)>;
    
}