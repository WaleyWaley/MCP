#include <iostream>
#include <string>
#include <nlohmann/json>


using json = nlohmann::json;

void parse_mcp_request(const std::string& raw_input)
{
    try{
        // 字符串 -> json 对象
        json req = json::parse(raw_input);
        // json的版本号
        std::string version = req["jsonrpc"];
        // 消息流水号，让Ai知道发送的消息是第几条
        auto id              = req["id"];
        // 调用的函数命
        std::string method  = req["method"]

        /*
            AI应用发送的指令
            {
                "jsonrpc": "2.0",
                "id": 1,
                "method": "initialize",
                "params": {
                    "protocolVersion": "2024-11-05",
                    "clientInfo": {
                    "name": "Cursor",
                    "version": "1.0.0"
                    },
                    "capabilities": {} 
                }
            }
        */
       // 提取嵌套的对象(params)
       // 使用 .value() 可以提供默认值，防止由于 key 不存在导致程序崩溃
        std::string client_name = req["params"]["clientinfo"].value("name","Unknown");

        std::cout << "[解析成功] 收到来自 " << client_name 
                  << " 的指令: " << method << " (ID: " << id << ")\n";

    } catch(const json::parse_error& e){
        std::cerr << "[解析失败] 格式错误: " << e.what() << '\n';
    } catch(const json::type_error& e) {
        std::cerr << "[类型错误] 数据类型不匹配: " << e.what() << '\n';
    }
}