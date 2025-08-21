#include "logger.h"
#include "default_value.h"

#include <QCoreApplication>
#include <QDateTime> //
#include <QDir>
#include <QFileInfo>      //
#include <QStandardPaths> //

#include <array>      //
#include <chrono>     //
#include <filesystem> //
#include <iostream>
#include <mutex> //
#include <ranges>
#include <shared_mutex> //
#include <system_error> //

Logger::Logger(QObject *parent) noexcept
    : QObject(parent),                    // 初始化父对象
      m_logFile(nullptr),                 // 初始化日志文件管理
      m_logStream(nullptr),               // 初始化日志流管理
      m_logLevel(LogLevel::Info),         // 初始化日志级别为Info
      m_logToFile(true),                  // 初始化是否记录到文件为true
      m_logToConsole(true),               // 初始化是否记录到控制台为true
      m_maxLogFileSize(10 * 1024 * 1024), // 初始化最大日志文件大小为10MB
      m_maxLogFiles(5)                    // 初始化最大日志文件数量为5
{
    // 设置日志目录
    try {
#if defined(QT_DEBUG)
        m_logDir = QCoreApplication::applicationDirPath() + "/logs";
#else
        m_logDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/logs";
#endif
        m_logFileName = QString::fromStdString(std::string{DefaultValues::logFileName}) + ".log";

        // 使用 std::filesystem 确保目录存在
        std::filesystem::path logPath{m_logDir.toStdString()};
        if (!std::filesystem::exists(logPath)) {
            std::filesystem::create_directories(logPath);
        }

        // 初始化日志文件 - 忽略错误，在运行时处理
        static_cast<void>(initLogFile());
    } catch (const std::exception &) {
        // 设置为控制台模式
        m_logToFile.store(false);
        m_logToConsole.store(true);
    }
}

Logger::~Logger() noexcept {
    try {
        if (m_logStream) {
            m_logStream->flush();
        }
        if (m_logFile && m_logFile->isOpen()) {
            m_logFile->close();
        }
    } catch (...) {
    }
}

/**
 * @brief 消息处理函数
 *
 * @param type 消息类型
 * @param context 消息上下文
 * @param msg 消息内容
 */
void Logger::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) noexcept {
    try {
        Logger::GetInstance().writeLog(type, context, msg);
    } catch (...) {
    }
}

/**
 * @brief 设置日志级别
 *
 * @param level 日志级别
 * @return std::expected<void, LogError> 操作结果
 */
std::expected<void, LogError> Logger::setLogLevel(LogLevel level) noexcept {
    if (!isValidLevel(level)) {
        return std::unexpected(LogError::InvalidLogLevel);
    }

    m_logLevel.store(level, std::memory_order_relaxed);
    return {};
}

/**
 * @brief 设置是否将日志输出到文件
 *
 * @param enabled 是否输出到文件
 * @return std::expected<void, LogError> 操作结果
 */
std::expected<void, LogError> Logger::setLogToFile(bool enabled) noexcept {
    try {
        std::unique_lock lock(m_shared_mutex);
        const bool currentState = m_logToFile.load(std::memory_order_acquire);

        if (currentState == enabled) {
            return {}; // 状态未改变
        }

        m_logToFile.store(enabled, std::memory_order_release);

        if (enabled && !m_logFile) {
            auto result = initLogFile();
            if (!result) {
                m_logToFile.store(false, std::memory_order_release);
                return result;
            }
        } else if (!enabled && m_logFile) {
            if (m_logStream) {
                m_logStream->flush();
            }
            if (m_logFile && m_logFile->isOpen()) {
                m_logFile->close();
            }
            m_logStream.reset();
            m_logFile.reset();
        }

        return {};
    } catch (const std::exception &) {
        return std::unexpected(LogError::FileOpenFailed);
    }
}

/**
 * @brief 设置是否将日志输出到控制台
 *
 * @param enabled 是否输出到控制台
 * @return std::expected<void, LogError> 操作结果
 */
std::expected<void, LogError> Logger::setLogToConsole(bool enabled) noexcept {
    m_logToConsole.store(enabled, std::memory_order_relaxed);
    return {};
}

/**
 * @brief 获取日志文件路径
 *
 * @return QString 日志文件路径
 */
QString Logger::getLogFilePath() const noexcept {
    return QString("%1/%2").arg(m_logDir, m_logFileName);
}

/**
 * @brief 清空日志文件
 *
 * @return std::expected<void, LogError> 操作结果
 */
std::expected<void, LogError> Logger::clearLogs() noexcept {
    try {
        std::unique_lock lock(m_shared_mutex);

        // 关闭当前日志文件
        if (m_logStream) {
            m_logStream->flush();
        }
        if (m_logFile && m_logFile->isOpen()) {
            m_logFile->close();
        }
        m_logStream.reset();
        m_logFile.reset();

        // 删除所有日志文件
        std::filesystem::path logDirPath{m_logDir.toStdString()};
        if (!std::filesystem::exists(logDirPath)) {
            return std::unexpected(LogError::FileOpenFailed);
        }

        std::error_code ec;
        for (const auto &entry : std::filesystem::directory_iterator(logDirPath, ec)) {
            if (ec)
                break;

            if (entry.is_regular_file() && entry.path().extension() == ".log") {
                std::filesystem::remove(entry.path(), ec);
                if (ec) {
                    return std::unexpected(LogError::WritePermissionDenied);
                }
            }
        }

        // 重新初始化日志文件
        if (m_logToFile.load(std::memory_order_acquire)) {
            return initLogFile();
        }

        return {};
    } catch (const std::exception &) {
        return std::unexpected(LogError::FileOpenFailed);
    }
}

/**
 * @brief 轮转日志文件
 *
 * @return std::expected<void, LogError> 操作结果
 * 
 * 检查日志文件是否超过最大大小限制，如果超过则进行轮转。
 */
std::expected<void, LogError> Logger::rotateLogFile() noexcept {
    std::unique_lock lock(m_shared_mutex);
    return checkLogRotation();
}

/**
 * @brief 写入日志
 *
 * @param type 日志类型
 * @param context 日志上下文
 * @param msg 日志消息
 */
void Logger::writeLog(QtMsgType type, const QMessageLogContext &context, const QString &msg) noexcept {
    try {
        // 快速级别检查 - 无锁操作
        const auto currentLevel = m_logLevel.load(std::memory_order_acquire);
        const auto msgLevel = [type]() -> LogLevel {
            switch (type) {
            case QtDebugMsg:
                return LogLevel::Debug;
            case QtInfoMsg:
                return LogLevel::Info;
            case QtWarningMsg:
                return LogLevel::Warning;
            case QtCriticalMsg:
                return LogLevel::Critical;
            case QtFatalMsg:
                return LogLevel::Fatal;
            default:
                return LogLevel::Info;
            }
        }();

        if (static_cast<std::underlying_type_t<LogLevel>>(msgLevel) <
            static_cast<std::underlying_type_t<LogLevel>>(currentLevel)) {
            return; // 早期返回，避免不必要的处理
        }

        // 使用共享锁进行读操作
        std::shared_lock lock(m_shared_mutex);

        // 日志格式化
        QString formattedMsg = formatLogMessage(type, context, msg);

        // 原子读取输出标志
        const bool toConsole = m_logToConsole.load(std::memory_order_acquire);
        const bool toFile = m_logToFile.load(std::memory_order_acquire);

        // 输出到控制台 - 无锁操作
        if (toConsole) {
            std::cout << formattedMsg.toLocal8Bit().toStdString() << std::endl;
        }

        // 输出到文件 - 需要升级为独占锁
        if (toFile && m_logStream) {
            lock.unlock();                               // 释放共享锁
            std::unique_lock uniqueLock(m_shared_mutex); // 获取独占锁

            if (m_logStream) { // 双重检查
                *m_logStream << formattedMsg << Qt::endl;
                m_logStream->flush();

                // 检查是否需要轮转日志文件
                static_cast<void>(checkLogRotation());
            }
        }
    } catch (...) {
    }
}

/**
 * @brief 初始化日志文件
 *
 * 初始化日志文件，创建日志文件目录和文件流。
 * 
 * @return std::expected<void, LogError> 操作结果
 */
std::expected<void, LogError> Logger::initLogFile() noexcept {
    try {
        if (!m_logToFile.load(std::memory_order_acquire)) {
            return {};
        }

        const QString logFilePath = getLogFilePath();

        // 检查磁盘空间
        std::filesystem::path filePath{logFilePath.toStdString()};
        std::error_code ec;
        auto space = std::filesystem::space(filePath.parent_path(), ec);
        if (ec || space.available < 1024 * 1024) { // 至少需要 1MB 空间
            return std::unexpected(LogError::DiskSpaceInsufficient);
        }

        m_logFile = std::make_unique<QFile>(logFilePath);

        if (!m_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
            m_logFile.reset();
            return std::unexpected(LogError::FileOpenFailed);
        }

        m_logStream = std::make_unique<QTextStream>(m_logFile.get());
        m_logStream->setEncoding(QStringConverter::Utf8);

        // 优化的启动标记
        const auto now = QDateTime::currentDateTime();
        const auto startMsg = QString::fromStdString(
            std::format("\n=== MyTodo 应用启动 [{}] ===", now.toString("yyyy-MM-dd hh:mm:ss").toStdString()));

        *m_logStream << startMsg << Qt::endl;
        m_logStream->flush();

        return {};
    } catch (const std::exception &) {
        m_logFile.reset();
        m_logStream.reset();
        return std::unexpected(LogError::FileOpenFailed);
    }
}

/**
 * @brief 检查日志文件是否需要轮转
 *
 * 检查日志文件是否超过最大大小限制，如果超过则进行轮转。
 * 
 * @return std::expected<void, LogError> 操作结果
 */
std::expected<void, LogError> Logger::checkLogRotation() noexcept {
    try {
        if (!m_logFile || !m_logToFile.load(std::memory_order_acquire)) {
            return {};
        }

        const auto maxSize = m_maxLogFileSize.load(std::memory_order_acquire);
        if (m_logFile->size() <= maxSize) {
            return {}; // 无需轮转
        }

        if (m_logStream) {
            m_logStream->flush();
        }
        if (m_logFile && m_logFile->isOpen()) {
            m_logFile->close();
        }
        m_logStream.reset();
        m_logFile.reset();

        // 优化的文件轮转
        const QString currentLogPath = getLogFilePath();
        const auto timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        const auto rotatedLogPath = QString::fromStdString(
            std::format("{}/{}_{}.log", m_logDir.toStdString(), DefaultValues::logFileName, timestamp.toStdString()));

        // 使用 std::filesystem 进行文件操作
        std::error_code ec;
        std::filesystem::rename(std::filesystem::path{currentLogPath.toStdString()},
                                std::filesystem::path{rotatedLogPath.toStdString()}, ec);

        if (ec) {
            return std::unexpected(LogError::RotationFailed);
        }

        // 清理旧的日志文件
        const auto maxFiles = m_maxLogFiles.load(std::memory_order_acquire);
        std::filesystem::path logDirPath{m_logDir.toStdString()};

        std::vector<std::filesystem::directory_entry> logFiles;
        for (const auto &entry : std::filesystem::directory_iterator(logDirPath, ec)) {
            if (ec)
                break;

            const auto filename = entry.path().filename().string();
            if (entry.is_regular_file() && filename.starts_with(DefaultValues::logFileName) &&
                filename.ends_with(".log")) {
                logFiles.push_back(entry);
            }
        }

        // 按修改时间排序，删除最旧的文件
        if (logFiles.size() >= static_cast<size_t>(maxFiles)) {
            std::ranges::sort(logFiles, [](const auto &a, const auto &b) {
                std::error_code ec1, ec2;
                auto timeA = std::filesystem::last_write_time(a, ec1);
                auto timeB = std::filesystem::last_write_time(b, ec2);
                return !ec1 && !ec2 && timeA < timeB;
            });

            const auto filesToRemove = logFiles.size() - maxFiles + 1;
            for (size_t i = 0; i < filesToRemove && i < logFiles.size(); ++i) {
                std::filesystem::remove(logFiles[i].path(), ec);
            }
        }

        // 重新初始化日志文件
        return initLogFile();
    } catch (const std::exception &) {
        return std::unexpected(LogError::RotationFailed);
    }
}

/**
 * @brief 格式化日志消息
 *
 * @param type 日志类型
 * @param context 日志上下文
 * @param msg 日志消息
 * @return QString 格式化后的日志消息
 */
QString Logger::formatLogMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg) const noexcept {
    try {
        // C++23 constexpr 级别映射
        constexpr std::array<std::string_view, 5> levelNames = {"调试", "警告", "错误", "致命", "信息"};

        const auto levelIndex = static_cast<size_t>(type);
        const auto levelStr = (levelIndex < levelNames.size()) ? levelNames[levelIndex] : "未知";

        // 高性能时间戳生成
        const auto now = std::chrono::system_clock::now();
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        // 文件名提取
        const auto fileName = context.file ? std::filesystem::path{context.file}.filename().string() : "未知";

        const auto result =
            std::format("[{:%Y-%m-%d %H:%M:%S}.{:03d}] [{}] [{}:{}] {}", std::chrono::floor<std::chrono::seconds>(now),
                        ms.count(), levelStr, fileName, context.line, msg.toStdString());

        return QString::fromStdString(result);
    } catch (const std::exception &) {
        // 回退到简单格式
        auto timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        auto typeStr = messageTypeToString(type);
        auto fileName = QFileInfo(context.file ? context.file : "").baseName();
        return QString("[%1] [%2] [%3:%4] %5").arg(timestamp, typeStr, fileName, QString::number(context.line), msg);
    }
}

/**
 * @brief 将日志类型转换为字符串
 *
 * @param type 日志类型
 * @return QString 日志类型的字符串表示
 */
QString Logger::messageTypeToString(QtMsgType type) noexcept {
    switch (type) {
    case QtDebugMsg:
        return "调试";
    case QtInfoMsg:
        return "信息";
    case QtWarningMsg:
        return "警告";
    case QtCriticalMsg:
        return "错误";
    case QtFatalMsg:
        return "致命";
    default:
        return "未知";
    }
}