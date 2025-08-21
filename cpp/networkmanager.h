/**
 * @file networkmanager.h
 * @brief NetworkManager类的头文件
 *
 * 该文件定义了NetworkManager类，用于管理应用程序的网络请求。
 *
 * @author Sakurakugu
 * @date 2025
 */

#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QQueue>
#include <QTimer>
#include <functional>

/**
 * @class NetworkManager
 * @brief 网络请求管理器，提供统一的网络请求处理和错误管理
 *
 * NetworkManager类是应用程序网络层的核心组件，提供了完整的网络请求管理功能：
 *
 * **核心功能：**
 * - HTTP请求的发送和响应处理
 * - 自动重试机制（可配置重试次数和策略）
 * - 请求超时管理和取消机制
 * - 统一的错误处理和分类
 * - 网络连接状态监控
 * - 认证令牌管理
 *
 * **高级特性：**
 * - 请求去重，避免重复请求
 * - 并发请求数量控制
 * - SSL错误处理
 * - 网络连接状态自动检测
 * - 认证令牌过期自动处理
 *
 * **使用场景：**
 * - 用户登录和认证
 * - 待办事项数据同步
 * - 服务器API调用
 * - 网络状态监控
 *
 * @note 所有网络操作都是异步的，通过信号槽机制通知结果
 * @see TodoModel
 */
class NetworkManager : public QObject {
    Q_OBJECT

  public:
    /**
     * @enum RequestType
     * @brief 请求类型枚举
     */
    enum RequestType {
        Login,      // 登录请求
        Sync,       // 同步请求
        FetchTodos, // 获取待办事项请求
        PushTodos,  // 推送待办事项请求
        Logout,     // 退出登录请求
    };
    Q_ENUM(RequestType)

    /**
     * @enum NetworkError
     * @brief 网络错误类型
     */
    enum NetworkError {
        NoError,             // 无错误
        TimeoutError,        // 超时错误
        ConnectionError,     // 连接错误
        AuthenticationError, // 认证错误
        ServerError,         // 服务器错误
        ParseError,          // 解析错误
        UnknownError         // 未知错误
    };
    Q_ENUM(NetworkError)

    /**
     * @struct RequestConfig
     * @brief 请求配置结构
     */
    struct RequestConfig {
        QString url;                    // 请求URL
        QJsonObject data;               // 请求数据，JSON格式
        QMap<QString, QString> headers; // 请求头，键值对格式
        int timeout = 10000;            // 默认10秒超时
        int maxRetries = 3;             // 默认最大重试3次
        bool requiresAuth = true;       // 是否需要认证
    };

    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager();

    // 认证管理
    void setAuthToken(const QString &token); // 设置认证令牌
    void clearAuthToken();                   // 清除认证令牌
    bool hasValidAuth() const;               // 检查是否有有效的认证信息

    // 服务器配置
    void setServerConfig(const QString &baseUrl, const QString &apiVersion = "v1"); // 设置服务器配置
    QString getApiUrl(const QString &endpoint) const;                               // 获取完整的API URL

    // 网络请求方法
    void sendRequest(RequestType type, const RequestConfig &config); // 发送网络请求
    void cancelRequest(RequestType type);                            // 取消指定类型的请求
    void cancelAllRequests();                                        // 取消所有请求

    // 网络状态检查
    bool isNetworkAvailable() const; // 检查网络是否可用
    void checkNetworkConnectivity(); // 检查网络连接状态

  signals:
    void requestCompleted(RequestType type, const QJsonObject &response);             // 请求完成信号
    void requestFailed(RequestType type, NetworkError error, const QString &message); // 请求失败信号
    void networkStatusChanged(bool isOnline);                                         // 网络状态改变信号
    void authTokenExpired();                                                          // 认证令牌过期信号

  private slots:
    void onReplyFinished();                           // 回复完成槽函数
    void onRequestTimeout();                          // 请求超时槽函数
    void onSslErrors(const QList<QSslError> &errors); // SSL错误槽函数

  private:
    /**
     * @struct PendingRequest
     * @brief 待处理请求结构
     */
    struct PendingRequest {
        RequestType type;
        RequestConfig config;
        QNetworkReply *reply = nullptr;
        QTimer *timeoutTimer = nullptr;
        int currentRetry = 0;
        qint64 requestId;
    };

    // 内部方法
    void executeRequest(PendingRequest &request); // 执行请求
    void completeRequest(qint64 requestId, bool success, const QJsonObject &response = QJsonObject(),
                         const QString &error = QString()); // 完成请求
    void cleanupRequest(qint64 requestId);                  // 清理请求

    QNetworkRequest createNetworkRequest(const RequestConfig &config) const;               // 创建网络请求
    NetworkError mapQNetworkError(QNetworkReply::NetworkError error) const;                // 映射网络错误
    QString getErrorMessage(NetworkError error, const QString &details = QString()) const; // 获取错误信息
    bool shouldRetry(NetworkError error) const;                                            // 是否应该重试

    void setupDefaultHeaders(QNetworkRequest &request) const; // 设置默认请求头
    void addAuthHeader(QNetworkRequest &request) const;       // 添加认证头

    // 请求去重
    bool isDuplicateRequest(RequestType type) const;           // 检查是否为重复请求
    void addActiveRequest(RequestType type, qint64 requestId); // 添加活跃请求
    void removeActiveRequest(RequestType type);                // 移除活跃请求

    // 成员变量
    QNetworkAccessManager *m_networkManager; // 网络访问管理器
    QString m_authToken;                     // 认证令牌
    QString m_serverBaseUrl;                 // 服务器基础URL
    QString m_apiVersion;                    // API版本

    QMap<qint64, PendingRequest> m_pendingRequests; // 待处理请求映射
    QMap<RequestType, qint64> m_activeRequests;     // 活跃请求映射（用于去重）
    qint64 m_nextRequestId;                         // 下一个请求ID

    bool m_isOnline; // 是否在线

    // 配置常量
    static const int DEFAULT_TIMEOUT_MS = 10000;
    static const int MAX_CONCURRENT_REQUESTS = 5;
    static const int CONNECTIVITY_CHECK_INTERVAL = 30000; // 30秒检查一次网络连接

    QTimer *m_connectivityTimer; // 连接定时器
};
