cd "$(dirname "$0")/.."

g++ -std=c++23 -pthread \
    test/test_socket_appender.cpp \
    src/LoggerAppender.cpp \
    src/LogEvent.cpp \
    src/LogFormatter.cpp \
    src/LogLevel.cpp \
    src/PatternItemImpl.cpp \
    -Iinclude \
    -I. \
    -o test_socket_appender && ./test_socket_appender