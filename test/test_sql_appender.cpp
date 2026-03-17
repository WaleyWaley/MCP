#include "logger/AppenderProxy.hpp"
#include "logger/LoggerAppender.h"
#include "logger/LogEvent.h"
#include "logger/LogFormatter.h"
#include "common/alias.h"

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <cassert>

// ============================================================
//  工具：一个内存数据库，用于收集 SqlAppender 输出的 SQL 语句
// ============================================================
struct FakeDb {
    std::mutex              mtx;
    std::vector<std::string> records;   // 收到的 INSERT 语句

    // 作为 SqlExecutor 回调传给 SqlAppender
    void exec(const std::string& sql) {
        auto lock = std::unique_lock{mtx};
        records.emplace_back(sql);
    }

    size_t count() {
        auto lock = std::unique_lock{mtx};
        return records.size();
    }

    // 打印所有收到的 SQL（方便肉眼核查）
    void dump() {
        auto lock = std::unique_lock{mtx};
        for (size_t i = 0; i < records.size(); ++i)
            std::cout << "  [" << i << "] " << records[i] << "\n";
    }

    // 检查第 idx 条 SQL 中是否包含某个子串
    bool contains(size_t idx, const std::string& sub) {
        auto lock = std::unique_lock{mtx};
        if (idx >= records.size()) return false;
        return records[idx].find(sub) != std::string::npos;
    }
};

// ============================================================
//  辅助：构造一个 LogEvent
// ============================================================
static LogEvent makeEvent(const std::string& logger_name,
                          LogLevel           level,
                          const std::string& message)
{
    uint32_t tid    = static_cast<uint32_t>(
        std::hash<std::thread::id>{}(std::this_thread::get_id()));
    auto     now_t  = SystemClock::to_time_t(SystemClock::now());
    LogEvent event{logger_name, level, 0, tid, "TestThread", now_t, 0};
    event.print("{}", message);
    return event;
}

// ============================================================
//  测试 1：基础写入 —— 单条日志能否到达 FakeDb
// ============================================================
void test_basic_write()
{
    std::cout << "[TEST 1] 基础写入\n";

    FakeDb db;
    AppenderProxy<SqlAppender> appender{
        LogFormatter{},
        "logs",
        [&db](const std::string& sql){ db.exec(sql); },
        /*batch_size=*/1,         // 每条立即触发 flush
        /*flush_interval_ms=*/200
    };

    auto event = makeEvent("BasicLogger", LogLevel::INFO, "hello sql appender");
    appender.log(event);

    // 等待后台线程处理
    std::this_thread::sleep_for(std::chrono::milliseconds{400});

    assert(db.count() == 1 && "应该收到 1 条 SQL");
    assert(db.contains(0, "INSERT INTO logs") && "SQL 应以 INSERT INTO logs 开头");
    assert(db.contains(0, "hello sql appender") && "SQL 应包含日志消息");
    assert(db.contains(0, "INFO") && "SQL 应包含日志级别");
    assert(db.contains(0, "BasicLogger") && "SQL 应包含 logger 名称");

    std::cout << "  收到的 SQL:\n";
    db.dump();
    std::cout << "  PASS\n";
}

// ============================================================
//  测试 2：批量写入 —— 多条日志全部到达 FakeDb
// ============================================================
void test_batch_write()
{
    std::cout << "[TEST 2] 批量写入\n";

    constexpr int N = 10;
    FakeDb db;
    AppenderProxy<SqlAppender> appender{
        LogFormatter{},
        "logs",
        [&db](const std::string& sql){ db.exec(sql); },
        /*batch_size=*/N,
        /*flush_interval_ms=*/500
    };

    for (int i = 0; i < N; ++i)
    {
        auto event = makeEvent("BatchLogger", LogLevel::DEBUG,
                               "batch message " + std::to_string(i));
        appender.log(event);
    }

    // 达到 batch_size，后台线程应被主动唤醒
    std::this_thread::sleep_for(std::chrono::milliseconds{600});

    assert(db.count() == static_cast<size_t>(N) && "应该收到 N 条 SQL");
    for (int i = 0; i < N; ++i)
        assert(db.contains(static_cast<size_t>(i),
                           "batch message " + std::to_string(i)) &&
               "每条 SQL 应包含对应的消息");

    std::cout << "  收到的 SQL 条数：" << db.count() << "\n";
    std::cout << "  PASS\n";
}

// ============================================================
//  测试 3：特殊字符转义 —— 消息中包含单引号
// ============================================================
void test_escape()
{
    std::cout << "[TEST 3] 单引号转义\n";

    FakeDb db;
    AppenderProxy<SqlAppender> appender{
        LogFormatter{},
        "logs",
        [&db](const std::string& sql){ db.exec(sql); },
        /*batch_size=*/1,
        /*flush_interval_ms=*/200
    };

    // 消息中包含单引号，应被转义为 ''
    auto event = makeEvent("EscapeLogger", LogLevel::WARN, "it\'s a test & O\'Brien");
    appender.log(event);

    std::this_thread::sleep_for(std::chrono::milliseconds{400});

    assert(db.count() == 1);
    // 转义后不应出现未转义的单引号破坏 SQL 结构
    assert(db.contains(0, "it''s") && "单引号应被转义为 ''");
    assert(db.contains(0, "O''Brien") && "单引号应被转义为 ''");

    std::cout << "  转义后的 SQL 片段：\n";
    db.dump();
    std::cout << "  PASS\n";
}

// ============================================================
//  测试 4：多线程并发写入
// ============================================================
void test_concurrent_write()
{
    std::cout << "[TEST 4] 多线程并发写入\n";

    constexpr int THREADS = 4;
    constexpr int PER_THREAD = 25;
    constexpr int TOTAL = THREADS * PER_THREAD;

    FakeDb db;
    AppenderProxy<SqlAppender> appender{
        LogFormatter{},
        "logs",
        [&db](const std::string& sql){ db.exec(sql); },
        /*batch_size=*/20,
        /*flush_interval_ms=*/300
    };

    std::vector<std::thread> threads;
    threads.reserve(THREADS);
    for (int t = 0; t < THREADS; ++t)
    {
        threads.emplace_back([&appender, t]{
            for (int i = 0; i < PER_THREAD; ++i)
            {
                auto event = makeEvent(
                    "ConcurrentLogger",
                    LogLevel::INFO,
                    "thread " + std::to_string(t) + " msg " + std::to_string(i)
                );
                appender.log(event);
            }
        });
    }
    for (auto& th : threads) th.join();

    // 等待后台线程将所有批次提交完毕
    std::this_thread::sleep_for(std::chrono::milliseconds{800});

    assert(db.count() == static_cast<size_t>(TOTAL) &&
           "所有线程的日志应全部到达 FakeDb");
    std::cout << "  收到的 SQL 条数：" << db.count()
              << " / " << TOTAL << "\n";
    std::cout << "  PASS\n";
}

// ============================================================
//  测试 5：executor 抛异常时不崩溃
// ============================================================
void test_executor_exception()
{
    std::cout << "[TEST 5] executor 抛异常时不崩溃\n";

    std::atomic<int> call_count{0};
    AppenderProxy<SqlAppender> appender{
        LogFormatter{},
        "logs",
        [&call_count](const std::string&){
            ++call_count;
            throw std::runtime_error("模拟数据库连接断开");
        },
        /*batch_size=*/1,
        /*flush_interval_ms=*/200
    };

    auto event = makeEvent("ExLogger", LogLevel::ERROR, "will trigger exception");
    appender.log(event);

    std::this_thread::sleep_for(std::chrono::milliseconds{400});

    // executor 被调用了一次，但 appender 本身不应崩溃
    assert(call_count.load() == 1 && "executor 应被调用一次");
    std::cout << "  executor 被调用次数：" << call_count.load() << "\n";
    std::cout << "  PASS（stderr 中应有一行错误提示，属正常现象）\n";
}

// ============================================================
//  main
// ============================================================
int main()
{
    std::cout << "========== SqlAppender 测试 ==========\n\n";

    test_basic_write();
    std::cout << "\n";

    test_batch_write();
    std::cout << "\n";

    test_escape();
    std::cout << "\n";

    test_concurrent_write();
    std::cout << "\n";

    test_executor_exception();
    std::cout << "\n";

    std::cout << "========== 全部测试通过 ==========\n";
    return 0;
}
