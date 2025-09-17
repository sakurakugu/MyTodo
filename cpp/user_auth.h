/**
 * @file user_auth.h
 * @brief AuthManager类的头文件
 *
 * 该文件定义了AuthManager类，用于管理用户认证功能。
 * 从TodoManager中拆分出来，专门负责用户登录、注销、令牌管理等认证相关操作。
 *
 * @author Sakurakugu
 * @date 2025-08-24 21:43:31(UTC+8) 周日
 * @change 2025-09-02 16:02:06(UTC+8) 周二
 * @version 0.4.0
 */

#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QUuid>
#include <memory>

#include "./foundation/network_request.h"

// 前向声明
class Setting;
class Database;

/**
 * @class UserAuth
 * @brief 用户认证管理类
 *
 * AuthManager类负责处理所有与用户认证相关的功能：
 *
 * **核心功能：**
 * - 用户登录和注销
 * - 访问令牌和刷新令牌管理
 * - 用户信息存储和获取
 * - 认证状态管理
 * - 令牌过期处理
 *
 * **特点：**
 * - 单例模式，确保全局唯一的认证状态
 * - 自动令牌持久化存储
 * - 与NetworkRequest集成，自动处理认证头
 * - 信号驱动的状态通知机制
 *
 * @note 该类是线程安全的，所有网络操作都通过NetworkRequest处理
 * @see NetworkRequest, Setting
 */
class UserAuth : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isOnline READ isOnline WRITE setIsOnline NOTIFY isOnlineChanged)
    Q_PROPERTY(QString username READ getUsername NOTIFY usernameChanged)
    Q_PROPERTY(QString email READ getEmail NOTIFY emailChanged)
    Q_PROPERTY(QUuid uuid READ getUuid NOTIFY uuidChanged)
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY isLoggedInChanged)

  public:
    /**
     * @brief 获取UserAuth单例实例
     * @return UserAuth实例引用
     */
    static UserAuth &GetInstance() noexcept {
        static UserAuth instance;
        return instance;
    }

    /**
     * @brief 析构函数
     */
    ~UserAuth();

    // 认证操作
    Q_INVOKABLE void login(const QString &username, const QString &password); // 使用用户凭据登录服务器
    Q_INVOKABLE void logout();                                                // 注销当前用户
    Q_INVOKABLE bool isLoggedIn() const;                                      // 检查用户是否已登录

    // 用户信息获取
    Q_INVOKABLE QString getUsername() const; // 获取用户名
    Q_INVOKABLE QString getEmail() const;    // 获取邮箱
    Q_INVOKABLE QUuid getUuid() const;       // 获取用户UUID

    // 令牌管理
    QString getAccessToken() const;                // 获取访问令牌
    QString getRefreshToken() const;               // 获取刷新令牌
    void setAuthToken(const QString &accessToken); // 设置访问令牌
    Q_INVOKABLE void refreshAccessToken();         // 手动刷新访问令牌
    bool isTokenExpiringSoon() const;              // 检查令牌是否即将过期

    // 网络连接状态管理
    bool isOnline() const;         // 获取当前在线状态
    void setIsOnline(bool online); // 设置在线状态

    // 服务器配置
    void setAuthApiEndpoint(const QString &endpoint); // 设置认证API端点
    QString getAuthApiEndpoint() const;               // 获取认证API端点

  signals:
    void usernameChanged();                        // 用户名变化信号
    void emailChanged();                           // 邮箱变化信号
    void uuidChanged();                            // UUID变化信号
    void isLoggedInChanged();                      // 登录状态变化信号
    void isOnlineChanged();                        // 在线状态变化信号
    void loginSuccessful(const QString &username); // 登录成功信号
    void loginFailed(const QString &errorMessage); // 登录失败信号
    void loginRequired();                          // 需要登录信号
    void logoutSuccessful();                       // 退出登录成功信号
    void authTokenExpired();                       // 认证令牌过期信号
    void tokenRefreshStarted();                    // 令牌刷新开始信号
    void tokenRefreshSuccessful();                 // 令牌刷新成功信号
    void tokenRefreshFailed(const QString &error); // 令牌刷新失败信号

  public slots:
    void onNetworkRequestCompleted(NetworkRequest::RequestType type,
                                   const QJsonObject &response); // 处理网络请求成功
    void onNetworkRequestFailed(NetworkRequest::RequestType type, NetworkRequest::NetworkError error,
                                const QString &message); // 处理网络请求失败
    void onAuthTokenExpired();                           // 处理认证令牌过期

  private slots:
    void onTokenExpiryCheck(); // 定时检查令牌过期
    void onBaseUrlChanged(const QString &newBaseUrl); // 服务器基础URL变化槽

  private:
    /**
     * @brief 私有构造函数（单例模式）
     * @param parent 父对象
     */
    explicit UserAuth(QObject *parent = nullptr);

    // 禁用拷贝构造和赋值操作
    UserAuth(const UserAuth &) = delete;
    UserAuth &operator=(const UserAuth &) = delete;
    UserAuth(UserAuth &&) = delete;
    UserAuth &operator=(UserAuth &&) = delete;

    // 私有方法
    void handleLoginSuccess(const QJsonObject &response);        // 处理登录成功
    void handleTokenRefreshSuccess(const QJsonObject &response); // 处理令牌刷新成功
    void loadStoredCredentials();                                // 加载存储的凭据
    void saveCredentials();                                      // 保存凭据到本地存储
    void clearCredentials();                                     // 清除凭据
    void validateStoredToken();                                  // 验证存储的令牌是否有效
    QString getApiUrl(const QString &endpoint) const;            // 获取完整的API URL
    void initializeServerConfig();                               // 初始化服务器配置
    void onNetworkStatusChanged(bool isOnline);                  // 处理网络状态变化
    void startTokenExpiryTimer();                                // 启动令牌过期检查定时器
    void stopTokenExpiryTimer();                                 // 停止令牌过期检查定时器
    void performSilentRefresh();                                 // 执行静默刷新
    qint64 getTokenExpiryTime() const;                           // 获取令牌过期时间
    void setTokenExpiryTime(qint64 expiryTime);                  // 设置令牌过期时间

    // 成员变量
    NetworkRequest &m_networkRequest; ///< 网络管理器引用
    Setting &m_setting;               ///< 应用设置引用
    Database &m_database;             ///< 数据库管理器引用

    QString m_accessToken;  ///< 访问令牌
    QString m_refreshToken; ///< 刷新令牌
    QString m_username;     ///< 用户名
    QString m_email;        ///< 邮箱
    QUuid m_uuid;           ///< 用户UUID

    bool m_isOnline; ///< 是否在线

    // 令牌管理
    QTimer *m_tokenExpiryTimer;                     ///< 令牌过期检查定时器
    qint64 m_tokenExpiryTime;                       ///< 令牌过期时间戳
    bool m_isRefreshing;                            ///< 是否正在刷新令牌
    static const int TOKEN_REFRESH_THRESHOLD = 300; ///< 令牌刷新阈值（秒）

    // 服务器配置
    QString m_serverBaseUrl;   ///< 服务器基础URL
    QString m_authApiEndpoint; ///< 认证API端点
};