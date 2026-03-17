# 1. 自动导航：确保脚本无论在哪里运行，都先回到 cotton 根目录
cd "$(dirname "$0")/.."

# 2. 终极编译命令
g++ -std=c++23 -pthread \
    test/test_sql_appender.cpp \
    src/LoggerAppender.cpp \
    src/LogEvent.cpp \
    src/LogFormatter.cpp \
    src/LogLevel.cpp \
    src/PatternItemImpl.cpp \
    -Iinclude \
    -I. \
    -o test_sql_appender && ./test_sql_appender