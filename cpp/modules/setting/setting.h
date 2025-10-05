/**
 * @file setting.h
 * @brief Setting类的头文件
 *
 * 该文件定义了Setting类，用于管理应用程序的配置信息。
 *
 * @author Sakurakugu
 * @date 2025-08-21 21:31:41(UTC+8) 周四
 * @change 2025-09-21 18:19:31(UTC+8) 周日
 */

#pragma once

#include "logger.h"

#include <QObject>
#include <QVariant>

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
    Setting(Setting &&) = delete;
    Setting &operator=(Setting &&) = delete;

    int getOsType() const;

    // 本地配置相关方法
    void save(const QString &key, const QVariant &value); ///< 保存设置到配置文件
    QVariant get(const QString &key,
                 const QVariant &defaultValue = QVariant()) const; ///< 从配置文件读取设置
    void remove(const QString &key);                               ///< 移除设置
    bool contains(const QString &key) const;                       ///< 检查设置是否存在
    void clear();                                                  ///< 清除所有设置
    bool openConfigFilePath() const;                               ///< 打开配置文件所在目录
    QString getConfigFilePath() const;                             ///< 获取配置文件路径

    // JSON 导入导出相关方法
    bool exportConfigToJsonFile(const QString &filePath);                      ///< 导出配置到JSON文件
    bool importConfigFromJsonFile(const QString &filePath, bool replaceAll);   ///< 从JSON文件导入配置
    bool exportDatabaseToJsonFile(const QString &filePath);                    ///< 导出数据库为JSON
    bool importDatabaseFromJsonFile(const QString &filePath, bool replaceAll); ///< 导入数据库JSON

    // 代理配置相关方法
    void setProxyType(int type);                    ///< 设置代理类型
    int getProxyType() const;                       ///< 获取代理类型
    void setProxyHost(const QString &host);         ///< 设置代理主机
    QString getProxyHost() const;                   ///< 获取代理主机
    void setProxyPort(int port);                    ///< 设置代理端口
    int getProxyPort() const;                       ///< 获取代理端口
    void setProxyUsername(const QString &username); ///< 设置代理用户名
    QString getProxyUsername() const;               ///< 获取代理用户名
    void setProxyPassword(const QString &password); ///< 设置代理密码
    QString getProxyPassword() const;               ///< 获取代理密码
    void setProxyEnabled(bool enabled);             ///< 设置是否启用代理
    bool getProxyEnabled() const;                   ///< 获取是否启用代理

    // 服务器配置相关
    bool isHttpsUrl(const QString &url) const;       // 检查URL是否使用HTTPS
    void updateServerConfig(const QString &baseUrl); // 更新服务器配置

    // 网络配置相关
    void setProxyConfig(bool enableProxy, int type, const QString &host, int port, const QString &username,
                        const QString &password); ///< 设置代理配置

    // 日志配置相关方法
    void setLogLevel(LogLevel logLevel);    ///< 设置日志级别
    LogLevel getLogLevel() const;           ///< 获取日志级别
    void setLogToFile(bool enabled);        ///< 设置是否记录到文件
    bool getLogToFile() const;              ///< 获取是否记录到文件
    void setLogToConsole(bool enabled);     ///< 设置是否输出到控制台
    bool getLogToConsole() const;           ///< 获取是否输出到控制台
    void setMaxLogFileSize(qint64 maxSize); ///< 设置最大日志文件大小
    qint64 getMaxLogFileSize() const;       ///< 获取最大日志文件大小
    void setMaxLogFiles(int maxFiles);      ///< 设置最大日志文件数量
    int getMaxLogFiles() const;             ///< 获取最大日志文件数量
    QString getLogFilePath() const;         ///< 获取日志文件路径
    void clearLogs();                       ///< 清除所有日志文件
    void initializeDefaultServerConfig();   ///< 初始化默认服务器配置

  signals:
    void baseUrlChanged(); // 服务器基础URL变化信号

  private:
    explicit Setting(QObject *parent = nullptr);
    ~Setting();

    Logger &m_logger; ///< 日志系统对象
    Config &m_config; ///< 配置文件对象
};
