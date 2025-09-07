/**
 * @file logger.h
 * @brief 日志记录器类的头文件
 *
 * 该文件定义了Logger类，用于记录应用程序的运行时日志信息。
 *
 * @author Sakurakugu
 * @date 2025-08-19 05:57:09(UTC+8) 周二
 * @change 2025-08-31 00:49:25(UTC+8) 周日
 * @version 0.4.0
 */

#pragma once

#include <QDateTime>
#include <QFileInfo>
#include <QObject>

#include <expected>
#include <fstream>
#include <shared_mutex>

// 强类型枚举比较
template <typename T>
concept LogLevelType = std::is_enum_v<T> && requires(T level) {
    static_cast<std::underlying_type_t<T>>(level);
    requires std::same_as<T, typename std::remove_cv_t<T>>;
};

// 文件大小类型概念 - 支持有符号和无符号整数
template <typename T>
concept FileSizeType = std::integral<T> && (sizeof(T) >= sizeof(std::int32_t));

// 日志操作结果类型
enum class LogError : std::uint8_t {
    FileOpenFailed = 1,        // 文件打开失败
    WritePermissionDenied = 2, // 没有写入权限
    DiskSpaceInsufficient = 3, // 磁盘空间不足
    InvalidLogLevel = 4,       // 日志级别无效
    RotationFailed = 5,        // 轮转日志失败
};

class Logger : public QObject {
    Q_OBJECT

  public:
    enum class LogLevel : std::uint8_t {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Critical = 3,
        Fatal = 4
    };

    // 枚举比较概念
    template <LogLevelType T> static constexpr bool isValidLevel(T level) noexcept {
        constexpr auto min_level = static_cast<std::underlying_type_t<T>>(LogLevel::Debug);
        constexpr auto max_level = static_cast<std::underlying_type_t<T>>(LogLevel::Fatal);
        const auto level_value = static_cast<std::underlying_type_t<T>>(level);
        return level_value >= min_level && level_value <= max_level;
    }

    // 单例模式
    static Logger &GetInstance() noexcept {
        static Logger instance;
        return instance;
    }
    // 删除拷贝和移动操作 - 单例模式
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;
    Logger(Logger &&) = delete;
    Logger &operator=(Logger &&) = delete;

    // 消息处理函数
    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) noexcept;

    // C++23 优化的配置方法 - 使用 std::expected 错误处理
    std::expected<void, LogError> setLogLevel(LogLevel level) noexcept;   // 设置日志级别
    std::expected<void, LogError> setLogToFile(bool enabled) noexcept;    // 设置是否将日志输出到文件
    std::expected<void, LogError> setLogToConsole(bool enabled) noexcept; // 设置是否将日志输出到控制台

    template <FileSizeType T>
    std::expected<void, LogError> setMaxLogFileSize(T maxSize) noexcept; // 最大日志文件大小（字节）
    template <std::integral T> std::expected<void, LogError> setMaxLogFiles(T maxFiles) noexcept; // 最大日志文件数量

    std::string getLogFilePath() const noexcept;        // 获取日志文件路径
    std::expected<void, LogError> clearLogs() noexcept; // 清空日志文件

  public slots:
    std::expected<void, LogError> rotateLogFile() noexcept; // 轮转日志文件

  private:
    explicit Logger(QObject *parent = nullptr) noexcept;
    ~Logger() noexcept override;

    void writeLog(QtMsgType type, const QMessageLogContext &context, const QString &msg) noexcept; // 写入日志
    std::expected<void, LogError> initLogFile() noexcept;                                          // 初始化日志文件
    std::expected<void, LogError> checkLogRotation() noexcept;                                     // 检查日志轮转

    // 格式化日志消息
    std::string formatLogMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg) const noexcept;
    // 格式化带颜色的控制台日志消息
    std::string formatColoredLogMessage(QtMsgType type, const std::string &msg) const noexcept;

    std::unique_ptr<std::ofstream> m_logFile; // 日志文件流
    mutable std::shared_mutex m_shared_mutex; // 读写分离的共享互斥锁
    std::atomic<LogLevel> m_logLevel;         // 日志级别
    std::atomic<bool> m_logToFile;            // 是否将日志输出到文件
    std::atomic<bool> m_logToConsole;         // 是否将日志输出到控制台
    std::atomic<qint64> m_maxLogFileSize;     // 最大日志文件大小（字节）
    std::atomic<int> m_maxLogFiles;           // 最大日志文件数量
    std::string m_logDir;                     // 日志目录路径
    std::string m_logFileName;                // 日志文件名
    std::string m_appName;                    // 应用程序名称
};

// 最大日志文件大小（字节）
template <FileSizeType T> std::expected<void, LogError> Logger::setMaxLogFileSize(T maxSize) noexcept {
    if constexpr (std::is_signed_v<T>) {
        if (maxSize <= 0) {
            return std::unexpected(LogError::InvalidLogLevel);
        }
    }
    std::shared_lock lock(m_shared_mutex);
    m_maxLogFileSize = static_cast<qint64>(maxSize);
    return {};
}

// 最大日志文件数量
template <std::integral T> std::expected<void, LogError> Logger::setMaxLogFiles(T maxFiles) noexcept {
    if constexpr (std::is_signed_v<T>) {
        if (maxFiles <= 0) {
            return std::unexpected(LogError::InvalidLogLevel);
        }
    }
    std::shared_lock lock(m_shared_mutex);
    m_maxLogFiles = static_cast<int>(maxFiles);
    return {};
}