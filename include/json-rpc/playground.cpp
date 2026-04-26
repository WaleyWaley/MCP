#include <iostream>
#include <string>
#include "json.hpp"


using json = nlohmann::json;

/** @brief 反序列化，听懂AI的指令 */
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
        std::string method  = req["method"];

        std::cout << "\n[C++逻辑层]正在解析 ID 为" << id << " 的方法：" << method << '\n';

        if(method == "initialize")
            std::cout << "[C++逻辑层] 发生握手请求，准备构建回复..." << '\n';
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

        // std::cout << "[解析成功] 收到来自 " << client_name << " 的指令: " << method << " (ID: " << id << ")\n";
    } catch(const json::parse_error& e){
        std::cerr << "[解析失败] 格式错误: " << e.what() << '\n';
    } catch(const json::type_error& e) {
        std::cerr << "[类型错误] 数据类型不匹配: " << e.what() << '\n';
    }
}

/** @brief 序列化，处理完AI的指令，按照MCP的格式，把结果打包成Json字符串发回去 */
auto build_mcp_response(int msg_id) -> std::string {
    json res;
    res["jsonrpc"]                          = "2.0";
    res["id"]                               = msg_id;
    // 构建嵌套的result对象
    // res["result"]["protocolVersion"]        = "2024-11-05";
    res["result"]["serverinfo"]["name"]     = "Jiang-Log-MCP-Server";
    res["result"]["serverinfo"]["version"]  = "1.0.0";
    // 空对象可以用json::object()表示
    res["result"]["capabilities"]["tools"]  = json::object();

    // 序列化: JSON对象->纯文本字符串
    return res.dump(4);

}

int main()
{
    std::string line;
    std::cout << "===MCP C++ 原生解析实验室 ===\n";
    std::cout << "请输入一个JSON字符串(例如: {\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\"})\n";
    while(std::getline(std::cin, line)){
        if(line == "exit")
            break;

        parse_mcp_request(line);
        std::cout << "\n[C++ 往外发送的 JSON]:\n" << build_mcp_response(1) << "\n\n";
        std::cout << "等待下一条指令（输入exit退出）:";
    }
    return 0;
}