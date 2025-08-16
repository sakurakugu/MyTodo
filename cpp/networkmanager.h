/**
 * @file networkmanager.h
 * @brief NetworkManager类的头文件
 * 
 * 该文件定义了NetworkManager类，用于管理应用程序的网络请求。
 * 
 * @author MyTodo Team
 * @date 2024
 */

#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QQueue>
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
class NetworkManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @enum RequestType
     * @brief 请求类型枚举
     */
    enum RequestType {
        Login,
        Sync,
        FetchTodos,
        PushTodos,
        Logout
    };
    Q_ENUM(RequestType)

    /**
     * @enum NetworkError
     * @brief 网络错误类型
     */
    enum NetworkError {
        NoError,
        TimeoutError,
        ConnectionError,
        AuthenticationError,
        ServerError,
        ParseError,
        UnknownError
    };
    Q_ENUM(NetworkError)

    /**
     * @struct RequestConfig
     * @brief 请求配置结构
     */
    struct RequestConfig {
        QString url;
        QJsonObject data;
        QMap<QString, QString> headers;
        int timeout = 10000;  // 默认10秒超时
        int maxRetries = 3;   // 默认最大重试3次
        bool requiresAuth = true;  // 是否需要认证
    };

    /**
     * @brief 构造函数
     * 
     * 创建NetworkManager实例，初始化网络访问管理器和连接监控定时器。
     * 
     * @param parent 父对象指针，用于Qt对象树管理
     */
    explicit NetworkManager(QObject *parent = nullptr);
    
    /**
     * @brief 析构函数
     * 
     * 清理所有待处理的请求，停止定时器，释放网络资源。
     */
    ~NetworkManager();

    // 认证管理
    /**
     * @brief 设置认证令牌
     * @param token 访问令牌字符串
     */
    void setAuthToken(const QString &token);
    
    /**
     * @brief 清除认证令牌
     */
    void clearAuthToken();
    
    /**
     * @brief 检查是否有有效的认证信息
     * @return 如果有有效认证令牌返回true，否则返回false
     */
    bool hasValidAuth() const;

    // 服务器配置
    /**
     * @brief 设置服务器配置
     * @param baseUrl 服务器基础URL
     * @param apiVersion API版本，默认为"v1"
     */
    void setServerConfig(const QString &baseUrl, const QString &apiVersion = "v1");
    
    /**
     * @brief 获取完整的API URL
     * @param endpoint API端点路径
     * @return 完整的API URL
     */
    QString getApiUrl(const QString &endpoint) const;

    // 网络请求方法
    void sendRequest(RequestType type, const RequestConfig &config);
    void cancelRequest(RequestType type);
    void cancelAllRequests();

    // 网络状态检查
    bool isNetworkAvailable() const;
    void checkNetworkConnectivity();

signals:
    void requestCompleted(RequestType type, const QJsonObject &response);
    void requestFailed(RequestType type, NetworkError error, const QString &message);
    void networkStatusChanged(bool isOnline);
    void authTokenExpired();

private slots:
    void onReplyFinished();
    void onRequestTimeout();
    void onSslErrors(const QList<QSslError> &errors);

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
    void executeRequest(PendingRequest &request);
    void retryRequest(PendingRequest &request);
    void completeRequest(qint64 requestId, bool success, const QJsonObject &response = QJsonObject(), const QString &error = QString());
    void cleanupRequest(qint64 requestId);
    
    QNetworkRequest createNetworkRequest(const RequestConfig &config) const;
    NetworkError mapQNetworkError(QNetworkReply::NetworkError error) const;
    QString getErrorMessage(NetworkError error, const QString &details = QString()) const;
    bool shouldRetry(NetworkError error) const;
    
    void setupDefaultHeaders(QNetworkRequest &request) const;
    void addAuthHeader(QNetworkRequest &request) const;
    
    // 请求去重
    bool isDuplicateRequest(RequestType type) const;
    void addActiveRequest(RequestType type, qint64 requestId);
    void removeActiveRequest(RequestType type);

    // 成员变量
    QNetworkAccessManager *m_networkManager;
    QString m_authToken;
    QString m_serverBaseUrl;
    QString m_apiVersion;
    
    QMap<qint64, PendingRequest> m_pendingRequests;  // 待处理请求映射
    QMap<RequestType, qint64> m_activeRequests;      // 活跃请求映射（用于去重）
    qint64 m_nextRequestId;
    
    bool m_isOnline;
    
    // 配置常量
    static const int DEFAULT_TIMEOUT_MS = 10000;
    static const int MAX_CONCURRENT_REQUESTS = 5;
    static const int CONNECTIVITY_CHECK_INTERVAL = 30000;  // 30秒检查一次网络连接
    
    QTimer *m_connectivityTimer;
};

#endif // NETWORKMANAGER_H