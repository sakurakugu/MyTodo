/**
 * @file logger.h
 * @brief 日志记录器类的头文件
 *
 * 该文件定义了Logger类，用于记录应用程序的运行时日志信息。
 *
 * @author Sakurakugu
 * @date 2025
 */

#pragma once

#include <QFile>
#include <QObject>

// 强类型枚举比较
template <typename T>
concept LogLevelType = std::is_enum_v<T> && requires(T level) { static_cast<std::underlying_type_t<T>>(level); };

// 字符串类型概念
template <typename T>
concept StringLike =
    std::convertible_to<T, std::string_view> || std::same_as<T, QString> || std::same_as<T, std::string>;

// 文件大小类型概念
template <typename T>
concept FileSizeType = std::integral<T> && std::signed_integral<T>;

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
        return static_cast<std::underlying_type_t<T>>(level) >= 0 && static_cast<std::underlying_type_t<T>>(level) <= 4;
    }

    // 单例模式
    static Logger &GetInstance() {
        static Logger instance;
        return instance;
    }
    // 删除拷贝构造函数和赋值运算符
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg); // 消息处理函数

    // 设置日志级别
    void setLogLevel(LogLevel level);
    void setLogToFile(bool enabled);                              // 设置是否将日志输出到文件
    void setLogToConsole(bool enabled);                           // 设置是否将日志输出到控制台
    template <FileSizeType T> void setMaxLogFileSize(T maxSize) { // 最大日志文件大小（字节）
        static_assert(std::is_integral_v<T>, "文件大小必须是整数类型");
        std::lock_guard<std::mutex> lock(m_mutex);
        m_maxLogFileSize = static_cast<qint64>(maxSize);
    }
    template <std::integral T> void setMaxLogFiles(T maxFiles) { // 最大日志文件数量
        static_assert(std::is_integral_v<T>, "最大文件数量必须是整型数据类型");
        std::lock_guard<std::mutex> lock(m_mutex);
        m_maxLogFiles = static_cast<int>(maxFiles);
    }

    QString getLogFilePath() const; // 获取日志文件路径
    void clearLogs();               // 清空日志文件

  public slots:
    void rotateLogFile(); // 轮转日志文件

  private:
    explicit Logger(QObject *parent = nullptr);
    ~Logger();

    void writeLog(QtMsgType type, const QMessageLogContext &context, const QString &msg);            // 写入日志
    void initLogFile();                                                                              // 初始化日志文件
    void checkLogRotation();                                                                         // 检查日志轮转
    QString formatLogMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg); // 格式化日志消息
    QString messageTypeToString(QtMsgType type); // 消息类型转换为字符串

    std::unique_ptr<QFile> m_logFile;         // 日志文件
    std::unique_ptr<QTextStream> m_logStream; // 日志流
    mutable std::mutex m_mutex;               // 互斥锁
    LogLevel m_logLevel;                      // 日志级别
    bool m_logToFile;                         // 是否将日志输出到文件
    bool m_logToConsole;                      // 是否将日志输出到控制台
    qint64 m_maxLogFileSize;                  // 最大日志文件大小（字节）
    int m_maxLogFiles;                        // 最大日志文件数量
    QString m_logDir;                         // 日志目录
    QString m_logFileName;                    // 日志文件名
};
