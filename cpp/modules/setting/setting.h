/**
 * @file setting.h
 * @brief Setting类的头文件
 *
 * 该文件定义了Setting类，用于管理应用程序的配置信息。
 *
 * @author Sakurakugu
 * @date 2025-08-21 21:31:41(UTC+8) 周四
 * @change 2025-10-06 02:13:23(UTC+8) 周一
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
    void 保存(const QString &key, const QVariant &value);
    QVariant 读取(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void 移除(const QString &key);
    bool 包含(const QString &key) const;
    void 清除所有();
    bool 打开配置文件路径() const;
    QString 获取配置文件路径() const;

    // JSON 导入导出相关方法
    bool 导出配置到JSON文件(const QString &filePath);
    bool 导入配置从JSON文件(const QString &filePath, bool replaceAll);
    bool 导出数据库到JSON文件(const QString &filePath);
    bool 导入数据库从JSON文件(const QString &filePath, bool replaceAll);

    // 代理配置相关方法
    void 设置代理类型(int type);
    int 获取代理类型() const;
    void 设置代理主机(const QString &host);
    QString 获取代理主机() const;
    void 设置代理端口(int port);
    int 获取代理端口() const;
    void 设置代理用户名(const QString &username);
    QString 获取代理用户名() const;
    void 设置代理密码(const QString &password);
    QString 获取代理密码() const;
    void 设置代理是否启用(bool enabled);
    bool 获取代理是否启用() const;

    // 服务器配置相关
    bool 是否是HttpsURL(const QString &url) const;
    void 更新服务器配置(const QString &baseUrl);

    // 网络配置相关
    void 设置代理配置(bool enableProxy, int type, const QString &host, //
                      int port, const QString &username, const QString &password);

    // 日志配置相关方法
    void 设置日志级别(LogLevel logLevel);
    LogLevel 获取日志级别() const;
    void 设置日志是否记录到文件(bool enabled);
    bool 获取日志是否记录到文件() const;
    void 设置日志是否输出到控制台(bool enabled);
    bool 获取日志是否输出到控制台() const;
    void 设置最大日志文件大小(qint64 maxSize);
    qint64 获取最大日志文件大小() const;
    void 设置最大日志文件数量(int maxFiles);
    int 获取最大日志文件数量() const;
    QString 获取日志文件路径() const;
    void 清除所有日志文件();
    void 初始化默认服务器配置();

  signals:
    void baseUrlChanged(); // 服务器基础URL变化信号

  private:
    explicit Setting(QObject *parent = nullptr);
    ~Setting();

    Logger &m_logger; ///< 日志系统对象
    Config &m_config; ///< 配置文件对象
};
