#ifndef LOGGER_H
#define LOGGER_H

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QLoggingCategory>
#include <QMutex>
#include <QObject>
#include <QStandardPaths>
#include <QTextStream>

class Logger : public QObject {
    Q_OBJECT

  public:
    enum LogLevel {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Critical = 3,
        Fatal = 4
    };
    Q_ENUM(LogLevel)

    static Logger &GetInstance() {
        static Logger instance;
        return instance;
    }
    // 删除拷贝构造函数和赋值运算符
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg); // 消息处理函数

    void setLogLevel(LogLevel level);       // 设置日志级别
    void setLogToFile(bool enabled);        // 设置是否将日志输出到文件
    void setLogToConsole(bool enabled);     // 设置是否将日志输出到控制台
    void setMaxLogFileSize(qint64 maxSize); // 最大日志文件大小（字节）
    void setMaxLogFiles(int maxFiles);      // 最大日志文件数量

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

    static Logger *s_instance; // 单例实例
    QFile *m_logFile;          // 日志文件
    QTextStream *m_logStream;  // 日志流
    QMutex m_mutex;            // 互斥锁
    LogLevel m_logLevel;       // 日志级别
    bool m_logToFile;          // 是否将日志输出到文件
    bool m_logToConsole;       // 是否将日志输出到控制台
    qint64 m_maxLogFileSize;   // 最大日志文件大小（字节）
    int m_maxLogFiles;         // 最大日志文件数量
    QString m_logDir;          // 日志目录
    QString m_logFileName;     // 日志文件名
};

#endif // LOGGER_H