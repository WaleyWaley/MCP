#include "logger/LoggerAppender.h"
#include "logger/AppenderProxy.hpp"
#include "logger/LogEvent.h"
#include "logger/LogFormatter.h"
#include "common/alias.h"

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cassert>
#include <vector>
#include <atomic>
#include <memory>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <sys/select.h>

// ============================================================
//  FakeServer
// ============================================================
struct FakeServer {
    int      listen_fd{-1};
    uint16_t port{0};
    std::thread server_thread_;

    struct State {
        std::mutex              mtx;
        std::vector<std::string> received;
        std::atomic<bool>       running{false};
    };
    std::shared_ptr<State> st_ = std::make_shared<State>();

    uint16_t startTcp() {
        listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        assert(listen_fd >= 0);
        int opt = 1;
        ::setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        struct sockaddr_in addr{};
        addr.sin_family = AF_INET; addr.sin_addr.s_addr = INADDR_ANY; addr.sin_port = 0;
        assert(::bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) == 0);
        assert(::listen(listen_fd, 8) == 0);
        socklen_t len = sizeof(addr);
        ::getsockname(listen_fd, (struct sockaddr*)&addr, &len);
        port = ntohs(addr.sin_port);
        st_->running = true;
        int lfd = listen_fd;
        auto st = st_;
        server_thread_ = std::thread{[lfd, st]{
            while (st->running) {
                fd_set fds; FD_ZERO(&fds); FD_SET(lfd, &fds);
                struct timeval tv{0, 200000};
                if (::select(lfd+1, &fds, nullptr, nullptr, &tv) <= 0) continue;
                struct sockaddr_in cli{}; socklen_t cl = sizeof(cli);
                int conn = ::accept(lfd, (struct sockaddr*)&cli, &cl);
                if (conn < 0) continue;
                // 读完整个连接（对端关闭后 recv 返回 0）
                std::string buf;
                char tmp[4096];
                while (true) {
                    fd_set rfds; FD_ZERO(&rfds); FD_SET(conn, &rfds);
                    struct timeval rtv{1, 0};
                    if (::select(conn+1, &rfds, nullptr, nullptr, &rtv) <= 0) break;
                    ssize_t n = ::recv(conn, tmp, sizeof(tmp)-1, 0);
                    if (n <= 0) break;
                    buf.append(tmp, n);
                }
                ::close(conn);
                if (!buf.empty()) {
                    auto lock = std::unique_lock{st->mtx};
                    st->received.emplace_back(std::move(buf));
                }
            }
        }};
        return port;
    }

    uint16_t startUdp() {
        listen_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
        assert(listen_fd >= 0);
        struct sockaddr_in addr{};
        addr.sin_family = AF_INET; addr.sin_addr.s_addr = INADDR_ANY; addr.sin_port = 0;
        assert(::bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) == 0);
        socklen_t len = sizeof(addr);
        ::getsockname(listen_fd, (struct sockaddr*)&addr, &len);
        port = ntohs(addr.sin_port);
        st_->running = true;
        int lfd = listen_fd;
        auto st = st_;
        server_thread_ = std::thread{[lfd, st]{
            char buf[4096];
            while (st->running) {
                fd_set fds; FD_ZERO(&fds); FD_SET(lfd, &fds);
                struct timeval tv{0, 200000};
                if (::select(lfd+1, &fds, nullptr, nullptr, &tv) <= 0) continue;
                ssize_t n = ::recvfrom(lfd, buf, sizeof(buf)-1, 0, nullptr, nullptr);
                if (n <= 0) continue;
                buf[n] = '\0';
                auto lock = std::unique_lock{st->mtx};
                st->received.emplace_back(buf, n);
            }
        }};
        return port;
    }

    void stop() {
        st_->running = false;
        if (listen_fd >= 0) { ::close(listen_fd); listen_fd = -1; }
        if (server_thread_.joinable()) server_thread_.join();
    }
    ~FakeServer() { stop(); }

    size_t count() { auto l = std::unique_lock{st_->mtx}; return st_->received.size(); }
    std::string all() {
        auto l = std::unique_lock{st_->mtx};
        std::string r; for (auto& s : st_->received) r += s; return r;
    }
};

// ============================================================
//  辅助
// ============================================================
static std::shared_ptr<LogEvent> makeEvent(const std::string& logger, LogLevel level, const std::string& msg)
{
    uint32_t tid = static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    auto now_t   = SystemClock::to_time_t(SystemClock::now());
    auto ev = std::make_shared<LogEvent>(logger, level, 0, tid, "T", now_t, 0);
    ev->print("{}", msg);
    return ev;
}

// ============================================================
//  TEST 1: TCP 基础发送
// ============================================================
void test_tcp_basic()
{
    std::cout << "[TEST 1] TCP 基础发送\n" << std::flush;
    FakeServer server;
    uint16_t port = server.startTcp();
    std::cout << "  port=" << port << "\n" << std::flush;
    {
        AppenderProxy<SocketAppender> appender{
            LogFormatter{}, "127.0.0.1", port,
            SocketAppender::Protocol::TCP, 4096, 100};
        std::this_thread::sleep_for(std::chrono::milliseconds{300});
        appender.log(*makeEvent("TcpLogger", LogLevel::INFO, "hello socket appender"));
        std::cout << "  log() sent\n" << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds{500});
    } // appender 析构 -> 连接关闭 -> server recv 返回 0 -> 数据入队
    std::this_thread::sleep_for(std::chrono::milliseconds{400});
    std::string data = server.all();
    std::cout << "  received " << data.size() << " bytes\n" << std::flush;
    std::cout << "  data=[" << data << "]\n" << std::flush;
    assert(!data.empty() && "服务端应收到数据");
    assert(data.find("hello socket appender") != std::string::npos && "应含消息");
    assert(data.find("INFO") != std::string::npos && "应含级别");
    server.stop();
    std::cout << "  PASS\n";
}

// ============================================================
//  TEST 2: TCP 批量发送
// ============================================================
void test_tcp_batch()
{
    std::cout << "[TEST 2] TCP 批量发送\n" << std::flush;
    constexpr int N = 10;
    FakeServer server;
    uint16_t port = server.startTcp();
    {
        AppenderProxy<SocketAppender> appender{
            LogFormatter{}, "127.0.0.1", port,
            SocketAppender::Protocol::TCP, 4096, 100};
        std::this_thread::sleep_for(std::chrono::milliseconds{300});
        for (int i = 0; i < N; ++i)
            appender.log(*makeEvent("Batch", LogLevel::DEBUG, "tcp batch msg "+std::to_string(i)));
        std::this_thread::sleep_for(std::chrono::milliseconds{600});
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{400});
    std::string data = server.all();
    std::cout << "  received " << data.size() << " bytes\n" << std::flush;
    for (int i = 0; i < N; ++i)
        assert(data.find("tcp batch msg "+std::to_string(i)) != std::string::npos);
    server.stop();
    std::cout << "  PASS\n";
}

// ============================================================
//  TEST 3: UDP 基础发送
// ============================================================
void test_udp_basic()
{
    std::cout << "[TEST 3] UDP 基础发送\n" << std::flush;
    FakeServer server;
    uint16_t port = server.startUdp();
    {
        AppenderProxy<SocketAppender> appender{
            LogFormatter{}, "127.0.0.1", port,
            SocketAppender::Protocol::UDP, 4096, 100};
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        appender.log(*makeEvent("UdpLogger", LogLevel::WARN, "hello udp appender"));
        std::this_thread::sleep_for(std::chrono::milliseconds{500});
    }
    std::string data = server.all();
    std::cout << "  received " << data.size() << " bytes\n" << std::flush;
    assert(!data.empty() && "UDP 应收到数据");
    assert(data.find("hello udp appender") != std::string::npos && "应含消息");
    server.stop();
    std::cout << "  PASS\n";
}

// ============================================================
//  TEST 4: 多线程并发 TCP
// ============================================================
void test_tcp_concurrent()
{
    std::cout << "[TEST 4] TCP 多线程并发\n" << std::flush;
    constexpr int THREADS = 4, PER = 20, TOTAL = THREADS * PER;
    FakeServer server;
    uint16_t port = server.startTcp();
    {
        AppenderProxy<SocketAppender> appender{
            LogFormatter{}, "127.0.0.1", port,
            SocketAppender::Protocol::TCP, 4096, 100};
        std::this_thread::sleep_for(std::chrono::milliseconds{300});
        std::vector<std::thread> ths;
        for (int t = 0; t < THREADS; ++t)
            ths.emplace_back([&appender, t]{
                for (int i = 0; i < PER; ++i)
                    appender.log(*makeEvent("Conc", LogLevel::INFO,
                        "thread "+std::to_string(t)+" msg "+std::to_string(i)));
            });
        for (auto& th : ths) th.join();
        std::this_thread::sleep_for(std::chrono::milliseconds{800});
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{400});
    std::string data = server.all();
    int found = 0;
    for (int t = 0; t < THREADS; ++t)
        for (int i = 0; i < PER; ++i)
            if (data.find("thread "+std::to_string(t)+" msg "+std::to_string(i)) != std::string::npos)
                ++found;
    std::cout << "  found " << found << "/" << TOTAL << "\n" << std::flush;
    assert(found == TOTAL && "所有消息均应送达");
    server.stop();
    std::cout << "  PASS\n";
}

// ============================================================
//  TEST 5: 队列背压
// ============================================================
void test_queue_backpressure()
{
    std::cout << "[TEST 5] 队列背压\n" << std::flush;
    // 故意连一个不存在的端口，验证 log() 不阻塞
    AppenderProxy<SocketAppender> appender{
        LogFormatter{}, "127.0.0.1", 19999,
        SocketAppender::Protocol::TCP, 5, 100};
    auto t1 = std::chrono::steady_clock::now();
    for (int i = 0; i < 20; ++i)
        appender.log(*makeEvent("Bp", LogLevel::ERROR, "msg "+std::to_string(i)));
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t1).count();
    std::cout << "  20 次 log() 耗时 " << ms << "ms\n" << std::flush;
    assert(ms < 1000 && "log() 不应阻塞");
    std::cout << "  PASS\n";
}

// ============================================================
//  main
// ============================================================
int main()
{
    std::cout << "========== SocketAppender 测试 ==========\n\n" << std::flush;
    test_tcp_basic();    std::cout << "\n";
    test_tcp_batch();    std::cout << "\n";
    test_udp_basic();    std::cout << "\n";
    test_tcp_concurrent(); std::cout << "\n";
    test_queue_backpressure(); std::cout << "\n";
    std::cout << "========== 全部测试通过 ==========\n";
    return 0;
}
