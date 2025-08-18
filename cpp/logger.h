#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QDir>

class Logger : public QObject
{
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

    static Logger& GetInstance() {
        static Logger instance;
        return instance;
    }
    // 删除拷贝构造函数和赋值运算符
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg); // 消息处理函数
    
    void setLogLevel(LogLevel level);
    void setLogToFile(bool enabled);
    void setLogToConsole(bool enabled);
    void setMaxLogFileSize(qint64 maxSize); // 最大日志文件大小（字节）
    void setMaxLogFiles(int maxFiles); // 最大日志文件数量
    
    QString getLogFilePath() const;
    void clearLogs();
    
public slots:
    void rotateLogFile();
    
private:
    explicit Logger(QObject *parent = nullptr);
    ~Logger();
    
    void writeLog(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    void initLogFile();
    void checkLogRotation();
    QString formatLogMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    QString messageTypeToString(QtMsgType type);
    
    static Logger* s_instance;
    
    QFile* m_logFile;
    QTextStream* m_logStream;
    QMutex m_mutex;
    
    LogLevel m_logLevel;
    bool m_logToFile;
    bool m_logToConsole;
    qint64 m_maxLogFileSize;
    int m_maxLogFiles;
    QString m_logDir;
    QString m_logFileName;
};

#endif // LOGGER_H