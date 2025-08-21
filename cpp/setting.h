/**
 * @file setting.h
 * @brief Setting类的头文件
 *
 * 该文件定义了Setting类，用于管理应用程序的配置信息。
 *
 * @author Sakurakugu
 * @date 2025
 */

#pragma once

#include "config.h"
#include "default_value.h"
#include "logger.h"

#include <QObject>

class Setting : public QObject {
    Q_OBJECT
  public:
    static Setting &GetInstance() {
        static Setting instance;
        return instance;
    }

    // 禁用拷贝构造和赋值操作
    Setting(const Setting &) = delete;
    Setting &operator=(const Setting &) = delete;

    Q_INVOKABLE int getOsType() const;

    // 本地配置相关方法
    Q_INVOKABLE bool save(const QString &key, const QVariant &value); ///< 保存设置到配置文件
    Q_INVOKABLE QVariant get(const QString &key,
                             const QVariant &defaultValue = QVariant()) const; ///< 从配置文件读取设置
    Q_INVOKABLE void remove(const QString &key);                               ///< 移除设置
    Q_INVOKABLE bool contains(const QString &key);                             ///< 检查设置是否存在
    Q_INVOKABLE QStringList allKeys();                                         ///< 获取所有设置的键名
    Q_INVOKABLE void clear();                                          ///< 清除所有设置

    // 存储类型和路径管理相关方法
    bool openConfigFilePath() const;   ///< 打开配置文件所在目录
    QString getConfigFilePath() const; ///< 获取配置文件路径

    // 日志配置相关方法
    Q_INVOKABLE void setLogLevel(int level);            ///< 设置日志级别
    Q_INVOKABLE int getLogLevel() const;                ///< 获取日志级别
    Q_INVOKABLE void setLogToFile(bool enabled);        ///< 设置是否记录到文件
    Q_INVOKABLE bool getLogToFile() const;              ///< 获取是否记录到文件
    Q_INVOKABLE void setLogToConsole(bool enabled);     ///< 设置是否输出到控制台
    Q_INVOKABLE bool getLogToConsole() const;           ///< 获取是否输出到控制台
    Q_INVOKABLE void setMaxLogFileSize(qint64 maxSize); ///< 设置最大日志文件大小
    Q_INVOKABLE qint64 getMaxLogFileSize() const;       ///< 获取最大日志文件大小
    Q_INVOKABLE void setMaxLogFiles(int maxFiles);      ///< 设置最大日志文件数量
    Q_INVOKABLE int getMaxLogFiles() const;             ///< 获取最大日志文件数量
    Q_INVOKABLE QString getLogFilePath() const;         ///< 获取日志文件路径
    Q_INVOKABLE void clearLogs();                       ///< 清除所有日志文件

    Q_INVOKABLE void initializeDefaultServerConfig(); ///< 初始化默认服务器配置

  private:
    explicit Setting(QObject *parent = nullptr);
    ~Setting();

    Logger &m_logger; ///< 日志系统对象
    Config &m_config; ///< 配置文件对象
};
