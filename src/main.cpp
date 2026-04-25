#include "common/LogMacros.h"
#include "logger/AppenderFacade.h"
#include "logger/LogEvent.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

class SimpleFileAppender final : public AppenderFacade {
public:
    explicit SimpleFileAppender(std::string file_name) : out_(std::move(file_name), std::ios::app) {}

    void log(const LogEvent& event) override {
        const auto lock = std::lock_guard<std::mutex>{mutex_};
        out_ << "[" << LevelToString(event.getLevel()) << "] "
             << event.getLoggerName() << " "
             << event.getFilename() << ":" << event.getLine() << " "
             << event.getContent() << '\n';
    }

private:
    std::mutex mutex_;
    std::ofstream out_;
};

int main() {
    auto logger = std::make_shared<AsyncLogger>(3);
    logger->setLogLevel(LogLevel::INFO);

    logger->addAppender(std::make_shared<SimpleFileAppender>("async_main.log"));

    logger->start();

    for (int i = 0; i < 30; ++i) {
        LOG_INFO(logger, "async log message, index={}", i);
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
    }

    // Wait long enough to observe the 3-second periodic flush.
    std::this_thread::sleep_for(std::chrono::seconds{4});
    logger->stop();

    std::cout << "done. check async_main.log" << std::endl;
    return 0;
}
