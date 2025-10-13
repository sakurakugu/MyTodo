/**
 * @file network_request.h
 * @brief NetworkRequest类的头文件
 *
 * 该文件定义了NetworkRequest类，用于管理应用程序的网络请求。
 *
 * @author Sakurakugu
 * @date 2025-08-17 07:17:29(UTC+8) 周日
 * @change 2025-10-05 21:22:10(UTC+8) 周日
 */

#pragma once

#include <QNetworkReply>
#include <QObject>
#include <QTimer>
#include <functional>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>

// 前向声明
class NetworkProxy;

/**
 * @class NetworkRequest
 * @brief 网络请求管理器，提供统一的网络请求处理和错误管理
 *
 * NetworkRequest类是应用程序网络层的核心组件，提供了完整的网络请求管理功能：
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
 * @see TodoManager
 */
namespace Network {
// 请求类型枚举
enum RequestType {
    Login,           // 登录请求
    Sync,            // 同步请求
    FetchTodos,      // 获取待办事项请求
    PushTodos,       // 推送待办事项请求
    Logout,          // 退出登录请求
    RefreshToken,    // 刷新令牌请求
    FetchCategories, // 获取类别列表请求
    CreateCategory,  // 创建类别请求
    PushCategories,  // 批量同步类别请求
    UpdateCategory,  // 更新类别请求
    DeleteCategory,  // 删除类别请求

    Other = 100,  // 其他请求，大于等于它的都是自定义请求
    UpdateCheck,  // 更新检查请求
    CheckHoliday, // 检查节假日请求
};

// 网络错误类型
enum Error {
    NoError,             // 无错误
    TimeoutError,        // 超时错误
    ConnectionError,     // 连接错误
    AuthenticationError, // 认证错误
    ServerError,         // 服务器错误
    ParseError,          // 解析错误
    UnknownError         // 未知错误
};

// 请求类型的中文名称映射
static const std::map<RequestType, QString> RequestTypeNameMap = //
    {{Login, "登录请求"},
     {Sync, "同步请求"},
     {FetchTodos, "获取待办事项请求"},
     {PushTodos, "推送待办事项请求"},
     {Logout, "退出登录请求"},
     {RefreshToken, "刷新令牌请求"},
     {FetchCategories, "获取类别列表请求"},
     {CreateCategory, "创建类别请求"},
     {PushCategories, "批量类别同步请求"},
     {UpdateCategory, "更新类别请求"},
     {DeleteCategory, "删除类别请求"},
     {Other, "其他请求"},
     {UpdateCheck, "更新检查请求"},
     {CheckHoliday, "检查节假日请求"}};
} // namespace Network

class NetworkRequest : public QObject {
    Q_OBJECT

  public:
    // 单例模式
    static NetworkRequest &GetInstance() noexcept {
        static NetworkRequest instance;
        return instance;
    }
    // 禁用拷贝构造和赋值操作
    NetworkRequest(const NetworkRequest &) = delete;
    NetworkRequest &operator=(const NetworkRequest &) = delete;
    NetworkRequest(NetworkRequest &&) = delete;
    NetworkRequest &operator=(NetworkRequest &&) = delete;

    /**
     * @struct RequestConfig
     * @brief 请求配置结构
     */
    struct RequestConfig {
        std::string url;                            // 请求URL
        std::string method = "GET";                 // HTTP方法，默认GET
        nlohmann::json data;                        // 请求数据，JSON格式
        std::map<std::string, std::string> headers; // 请求头，键值对格式
        int timeout = 10000;                        // 默认10秒超时
        int maxRetries = 3;                         // 默认最大重试3次
        bool requiresAuth = true;                   // 是否需要认证
    };

    /**
     * @brief 自定义响应处理器函数类型
     * @param rawResponse 原始响应数据（QByteArray）
     * @param httpStatusCode HTTP状态码
     * @return 解析后的JSON对象，如果解析失败返回空对象
     */
    using CustomResponseHandler = std::function<nlohmann::json(const QByteArray &rawResponse, int httpStatusCode)>;

    // 认证管理
    void setAuthToken(const std::string &token); // 设置认证令牌
    void clearAuthToken();                       // 清除认证令牌
    bool hasValidAuth() const;                   // 检查是否有有效的认证信息

    // 服务器配置
    void setServerConfig(const std::string &baseUrl, const std::string &apiVersion = ""); // 设置服务器地址与api版本
    std::string getServerBaseUrl() const;                                                 // 获取服务器基础URL
    std::string getApiUrl(const std::string &endpoint) const;                             // 获取完整的API URL

    // 网络请求方法
    void sendRequest(Network::RequestType type, const RequestConfig &config,
                     const std::optional<CustomResponseHandler> &customHandler = std::nullopt); // 发送网络请求

    void cancelRequest(Network::RequestType type); // 取消指定类型的请求
    void cancelAllRequests();                      // 取消所有请求

    std::string RequestTypeToString(Network::RequestType type) const; // 请求类型转字符串

  signals:
    void requestCompleted(Network::RequestType type, const nlohmann::json &response);                // 请求完成信号
    void requestFailed(Network::RequestType type, Network::Error error, const std::string &message); // 请求失败信号
    void authTokenExpired();                                                                         // 认证令牌过期信号

  private slots:
    void onReplyFinished();  // 回复完成槽函数
    void onRequestTimeout(); // 请求超时槽函数

  private:
    explicit NetworkRequest(QObject *parent = nullptr);
    ~NetworkRequest();

    /**
     * @struct PendingRequest
     * @brief 待处理请求结构
     */
    struct PendingRequest {
        Network::RequestType type;           // 请求类型
        RequestConfig config;                // 请求配置
        QNetworkReply *reply = nullptr;      // 网络回复对象
        QTimer *timeoutTimer = nullptr;      // 超时定时器
        int currentRetry = 0;                // 当前重试次数
        int64_t requestId;                   // 请求ID
        CustomResponseHandler customHandler; // 自定义响应处理器（仅用于Other类型）
    };

    // 内部方法
    void executeRequest(PendingRequest &request); // 执行请求
    void completeRequest(int64_t requestId, bool success, const nlohmann::json &response = nlohmann::json(),
                         const std::string &error = std::string()); // 完成请求
    void cleanupRequest(int64_t requestId);                         // 清理请求
    void onSslErrors(const QList<QSslError> &errors);               // SSL错误槽函数(Qt自带信号)

    QNetworkRequest createNetworkRequest(const RequestConfig &config) const;                             // 创建网络请求
    Network::Error mapQNetworkError(QNetworkReply::NetworkError error) const;                            // 映射网络错误
    std::string getErrorMessage(Network::Error error, const std::string &details = std::string()) const; // 获取错误信息
    bool shouldRetry(Network::Error error) const;                                                        // 是否应该重试

    void setupDefaultHeaders(QNetworkRequest &request) const; // 设置默认请求头
    void addAuthHeader(QNetworkRequest &request) const;       // 添加认证头

    // 请求去重
    bool isDuplicateRequest(Network::RequestType type) const;            // 检查是否为重复请求
    void addActiveRequest(Network::RequestType type, int64_t requestId); // 添加活跃请求
    void removeActiveRequest(Network::RequestType type);                 // 移除活跃请求

    // 成员变量
    QNetworkAccessManager *m_networkRequest; // 网络访问管理器
    std::string m_authToken;                 // 认证令牌
    std::string m_serverBaseUrl;             // 服务器基础URL
    std::string m_apiVersion;                // API版本
    std::string m_computerName;              // 计算机名称

    std::map<int64_t, PendingRequest> m_pendingRequests;      // 待处理请求映射
    std::map<Network::RequestType, int64_t> m_activeRequests; // 活跃请求映射（用于去重）
    int64_t m_nextRequestId;                                  // 下一个请求ID

    // 配置常量
    static const int DEFAULT_TIMEOUT_MS = 10000;
    static const int MAX_CONCURRENT_REQUESTS = 5;
    static const int CONNECTIVITY_CHECK_INTERVAL = 30000; // 30秒检查一次网络连接

    NetworkProxy &m_proxyManager; // 代理管理器
};
