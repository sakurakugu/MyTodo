/**
 * @brief 日志流类
 * 用于在日志中输出格式化的信息。
 * 支持 << 运算符，用于输出不同类型的信息。
 * 支持 std::endl 等流操作符。
 * 日志流对象在析构时，会将流中的内容输出到日志中。
 *
 * 具体使用方法在logger.h中。
 *
 * @author Sakurakugu
 * @date 2025-10-11 21:42:09(UTC+8) 周二
 * @change 2025-10-12 19:24:05 (UTC+8) 周日

 * @file log_stream.h*/

#include "logger.h"
#include <source_location>
#include <sstream>

class LogStream {
  public:
    LogStream(LogLevel type, const LogContext &context = LogContext()) //
        : m_type(type),                                                //
          m_context(context) {}

    LogStream(LogLevel type, const char *file, int line, const char *func) //
        : m_type(type),                                                    //
          m_context(file, line, func) {}

    template <typename T> LogStream &operator<<(const T &value) {
        m_stream << value;
        return *this;
    }

    // 支持 std::endl 等
    using StreamManipulator = std::ostream &(*)(std::ostream &);
    LogStream &operator<<(StreamManipulator manip) {
        manip(m_stream);
        return *this;
    }

    ~LogStream() { Logger::GetInstance().messageHandler(m_type, m_context, m_stream.str()); }

  private:
    LogLevel m_type;
    LogContext m_context;
    std::ostringstream m_stream;
};

inline LogStream logStream(LogLevel type, const std::source_location &loc = std::source_location::current()) {
    return LogStream(type, loc.file_name(), loc.line(), loc.function_name());
}
inline LogStream logStream(const std::source_location &loc = std::source_location::current()) {
    return logStream(LogLevel::None, loc);
}
inline LogStream logDebug(const std::source_location &loc = std::source_location::current()) {
    return logStream(LogLevel::Debug, loc);
}
inline LogStream logInfo(const std::source_location &loc = std::source_location::current()) {
    return logStream(LogLevel::Info, loc);
}
inline LogStream logWarning(const std::source_location &loc = std::source_location::current()) {
    return logStream(LogLevel::Warning, loc);
}
inline LogStream logError(const std::source_location &loc = std::source_location::current()) {
    return logStream(LogLevel::Critical, loc);
}
inline LogStream logCritical(const std::source_location &loc = std::source_location::current()) {
    return logStream(LogLevel::Critical, loc);
}
inline LogStream logFatal(const std::source_location &loc = std::source_location::current()) {
    return logStream(LogLevel::Fatal, loc);
}

#ifdef QT_CORE_LIB
// << 运算符重载
inline std::ostream &operator<<(std::ostream &os, const QString &dt) {
    os << dt.toStdString();
    return os;
}
#endif
