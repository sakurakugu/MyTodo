/**
 * @file setting.h
 * @brief Setting类的头文件
 *
 * 该文件定义了Setting类，用于管理应用程序的配置信息。
 *
 * @author Sakurakugu
 * @date 2025-08-21 21:31:41(UTC+8) 周四
 * @version 2025-08-23 21:09:00(UTC+8) 周六
 */

#pragma once

#include "foundation/logger.h"

#include <QObject>

class Config;
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
    Q_INVOKABLE void clear();                                                  ///< 清除所有设置
    Q_INVOKABLE bool openConfigFilePath() const;                               ///< 打开配置文件所在目录
    Q_INVOKABLE QString getConfigFilePath() const;                             ///< 获取配置文件路径

    // 代理配置相关方法
    Q_INVOKABLE void setProxyType(int type);                    ///< 设置代理类型
    Q_INVOKABLE int getProxyType() const;                       ///< 获取代理类型
    Q_INVOKABLE void setProxyHost(const QString &host);         ///< 设置代理主机
    Q_INVOKABLE QString getProxyHost() const;                   ///< 获取代理主机
    Q_INVOKABLE void setProxyPort(int port);                    ///< 设置代理端口
    Q_INVOKABLE int getProxyPort() const;                       ///< 获取代理端口
    Q_INVOKABLE void setProxyUsername(const QString &username); ///< 设置代理用户名
    Q_INVOKABLE QString getProxyUsername() const;               ///< 获取代理用户名
    Q_INVOKABLE void setProxyPassword(const QString &password); ///< 设置代理密码
    Q_INVOKABLE QString getProxyPassword() const;               ///< 获取代理密码
    Q_INVOKABLE void setProxyEnabled(bool enabled);             ///< 设置是否启用代理
    Q_INVOKABLE bool getProxyEnabled() const;                   ///< 获取是否启用代理

    // 服务器配置相关
    Q_INVOKABLE bool isHttpsUrl(const QString &url) const;       // 检查URL是否使用HTTPS
    Q_INVOKABLE void updateServerConfig(const QString &baseUrl); // 更新服务器配置

    // 日志配置相关方法
    Q_INVOKABLE void setLogLevel(Logger::LogLevel logLevel); ///< 设置日志级别
    Q_INVOKABLE Logger::LogLevel getLogLevel() const;        ///< 获取日志级别
    Q_INVOKABLE void setLogToFile(bool enabled);             ///< 设置是否记录到文件
    Q_INVOKABLE bool getLogToFile() const;                   ///< 获取是否记录到文件
    Q_INVOKABLE void setLogToConsole(bool enabled);          ///< 设置是否输出到控制台
    Q_INVOKABLE bool getLogToConsole() const;                ///< 获取是否输出到控制台
    Q_INVOKABLE void setMaxLogFileSize(qint64 maxSize);      ///< 设置最大日志文件大小
    Q_INVOKABLE qint64 getMaxLogFileSize() const;            ///< 获取最大日志文件大小
    Q_INVOKABLE void setMaxLogFiles(int maxFiles);           ///< 设置最大日志文件数量
    Q_INVOKABLE int getMaxLogFiles() const;                  ///< 获取最大日志文件数量
    Q_INVOKABLE QString getLogFilePath() const;              ///< 获取日志文件路径
    Q_INVOKABLE void clearLogs();                            ///< 清除所有日志文件
    Q_INVOKABLE void initializeDefaultServerConfig(); ///< 初始化默认服务器配置

  signals:
    void baseUrlChanged(const QString &newBaseUrl); // 服务器基础URL变化信号

  private:
    explicit Setting(QObject *parent = nullptr);
    ~Setting();

    Logger &m_logger; ///< 日志系统对象
    Config &m_config; ///< 配置文件对象
};
