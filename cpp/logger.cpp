#include "logger.h"
#include <QCoreApplication>
#include <QDebug>
#include <iostream>

Logger *Logger::s_instance = nullptr;

Logger::Logger(QObject *parent)
    : QObject(parent),                    // 初始化父对象
      m_logFile(nullptr),                 // 初始化日志文件指针为nullptr
      m_logStream(nullptr),               // 初始化日志流指针为nullptr
      m_logLevel(Debug),                  // 初始化日志级别为Debug
      m_logToFile(true),                  // 初始化是否记录到文件为true
      m_logToConsole(true),               // 初始化是否记录到控制台为true
      m_maxLogFileSize(10 * 1024 * 1024), // 初始化最大日志文件大小为10MB
      m_maxLogFiles(5)                    // 初始化最大日志文件数量为5
{
    // 设置日志目录
    // m_logDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/logs";
    // m_logDir = QCoreApplication::applicationDirPath() + "/logs";
    m_logDir = QDir::currentPath() + "/logs";
    m_logFileName = "mytodo.log";

    // 确保日志目录存在
    QDir dir;
    if (!dir.exists(m_logDir)) {
        dir.mkpath(m_logDir);
    }

    initLogFile();
}

Logger::~Logger() {
    if (m_logStream) {
        delete m_logStream;
    }
    if (m_logFile) {
        m_logFile->close();
        delete m_logFile;
    }
}

void Logger::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    Logger::GetInstance().writeLog(type, context, msg);
}

void Logger::setLogLevel(LogLevel level) {
    QMutexLocker locker(&m_mutex);
    m_logLevel = level;
}

void Logger::setLogToFile(bool enabled) {
    QMutexLocker locker(&m_mutex);
    m_logToFile = enabled;

    if (enabled && !m_logFile) {
        initLogFile();
    } else if (!enabled && m_logFile) {
        m_logStream->flush();
        m_logFile->close();
        delete m_logStream;
        delete m_logFile;
        m_logStream = nullptr;
        m_logFile = nullptr;
    }
}

void Logger::setLogToConsole(bool enabled) {
    QMutexLocker locker(&m_mutex);
    m_logToConsole = enabled;
}

void Logger::setMaxLogFileSize(qint64 maxSize) {
    QMutexLocker locker(&m_mutex);
    m_maxLogFileSize = maxSize;
}

void Logger::setMaxLogFiles(int maxFiles) {
    QMutexLocker locker(&m_mutex);
    m_maxLogFiles = maxFiles;
}

QString Logger::getLogFilePath() const {
    return m_logDir + "/" + m_logFileName;
}

void Logger::clearLogs() {
    QMutexLocker locker(&m_mutex);

    // 关闭当前日志文件
    if (m_logStream) {
        delete m_logStream;
        m_logStream = nullptr;
    }
    if (m_logFile) {
        m_logFile->close();
        delete m_logFile;
        m_logFile = nullptr;
    }

    // 删除所有日志文件
    QDir logDir(m_logDir);
    QStringList logFiles = logDir.entryList(QStringList() << "*.log", QDir::Files);
    for (const QString &file : logFiles) {
        logDir.remove(file);
    }

    // 重新初始化日志文件
    if (m_logToFile) {
        initLogFile();
    }
}

void Logger::rotateLogFile() {
    QMutexLocker locker(&m_mutex);
    checkLogRotation();
}

void Logger::writeLog(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QMutexLocker locker(&m_mutex);

    // 检查日志级别
    LogLevel msgLevel;
    switch (type) {
    case QtDebugMsg:
        msgLevel = Debug;
        break;
    case QtInfoMsg:
        msgLevel = Info;
        break;
    case QtWarningMsg:
        msgLevel = Warning;
        break;
    case QtCriticalMsg:
        msgLevel = Critical;
        break;
    case QtFatalMsg:
        msgLevel = Fatal;
        break;
    }

    if (msgLevel < m_logLevel) {
        return;
    }

    // 格式化日志消息
    QString formattedMsg = formatLogMessage(type, context, msg);

    // 输出到控制台
    if (m_logToConsole) {
        std::cout << formattedMsg.toLocal8Bit().toStdString() << std::endl;
        // std::cout << formattedMsg.toLocal8Bit().constData() << std::endl;
    }

    // 输出到文件
    if (m_logToFile && m_logStream) {
        *m_logStream << formattedMsg << Qt::endl;
        m_logStream->flush();

        // 检查是否需要轮转日志文件
        checkLogRotation();
    }
}

void Logger::initLogFile() {
    if (!m_logToFile) {
        return;
    }

    QString logFilePath = getLogFilePath();
    m_logFile = new QFile(logFilePath);

    if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        m_logStream = new QTextStream(m_logFile);
        m_logStream->setEncoding(QStringConverter::Utf8);

        // 写入启动标记
        QString startMsg =
            QString("\n=== MyTodo 应用启动 [%1] ===").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        *m_logStream << startMsg << Qt::endl;
        m_logStream->flush();
    } else {
        delete m_logFile;
        m_logFile = nullptr;
        std::cerr << "无法打开日志文件: " << logFilePath.toStdString() << std::endl;
    }
}

void Logger::checkLogRotation() {
    if (!m_logFile || !m_logToFile) {
        return;
    }

    // 检查文件大小
    if (m_logFile->size() > m_maxLogFileSize) {
        // 关闭当前文件
        m_logStream->flush();
        delete m_logStream;
        m_logFile->close();
        delete m_logFile;
        m_logStream = nullptr;
        m_logFile = nullptr;

        // 轮转日志文件
        QString currentLogPath = getLogFilePath();
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        QString rotatedLogPath = m_logDir + "/mytodo_" + timestamp + ".log";

        QFile::rename(currentLogPath, rotatedLogPath);

        // 清理旧的日志文件
        QDir logDir(m_logDir);
        QStringList logFiles = logDir.entryList(QStringList() << "mytodo_*.log", QDir::Files, QDir::Time);

        // 删除超出数量限制的旧文件
        while (logFiles.size() >= m_maxLogFiles) {
            QString oldestFile = logFiles.takeLast();
            logDir.remove(oldestFile);
        }

        // 重新初始化日志文件
        initLogFile();
    }
}

QString Logger::formatLogMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString typeStr = messageTypeToString(type);
    QString fileName = QFileInfo(context.file ? context.file : "").baseName();

    return QString("[%1] [%2] [%3:%4] %5").arg(timestamp).arg(typeStr).arg(fileName).arg(context.line).arg(msg);
}

QString Logger::messageTypeToString(QtMsgType type) {
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