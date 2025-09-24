/**
 * @file user_auth.h
 * @brief AuthManager类的头文件
 *
 * 该文件定义了AuthManager类，用于管理用户认证功能。
 * 从TodoManager中拆分出来，专门负责用户登录、注销、令牌管理等认证相关操作。
 *
 * @author Sakurakugu
 * @date 2025-08-24 21:43:31(UTC+8) 周日
 * @change 2025-09-22 16:33:30(UTC+8) 周一
 */

#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QUuid>
#include <memory>

#include "./foundation/network_request.h"
#include "default_value.h"

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
    Q_PROPERTY(QString username READ getUsername NOTIFY usernameChanged)
    Q_PROPERTY(QString email READ getEmail NOTIFY emailChanged)
    Q_PROPERTY(QUuid uuid READ getUuid NOTIFY uuidChanged)
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY isLoggedInChanged)

  public:
    explicit UserAuth(QObject *parent = nullptr);
    ~UserAuth();

    // 认证操作
    Q_INVOKABLE void login(const QString &account, const QString &password); // 使用用户凭据登录服务器
    Q_INVOKABLE void logout();                                               // 注销当前用户
    Q_INVOKABLE bool isLoggedIn() const;                                     // 检查用户是否已登录

    // 用户信息获取
    QString getUsername() const; // 获取用户名
    QString getEmail() const;    // 获取邮箱
    QUuid getUuid() const;       // 获取用户UUID

  signals:
    void usernameChanged();                        // 用户名变化信号
    void emailChanged();                           // 邮箱变化信号
    void uuidChanged();                            // UUID变化信号
    void isLoggedInChanged();                      // 登录状态变化信号
    void loginSuccessful(const QString &username); // 登录成功信号
    void loginFailed(const QString &errorMessage); // 登录失败信号
    void loginRequired();                          // 需要登录信号
    void logoutSuccessful();                       // 退出登录成功信号
    void authTokenExpired();                       // 认证令牌过期信号
    void tokenRefreshStarted();                    // 令牌刷新开始信号
    void tokenRefreshSuccessful();                 // 令牌刷新成功信号
    void tokenRefreshFailed(const QString &error); // 令牌刷新失败信号
    void firstAuthCompleted();                     // 首次完成认证（登录或首次刷新）信号，仅触发一次

  public slots:
    void onNetworkRequestCompleted(NetworkRequest::RequestType type, const QJsonObject &response); // 处理网络请求成功
    void onNetworkRequestFailed(NetworkRequest::RequestType type, NetworkRequest::NetworkError error,
                                const QString &message); // 处理网络请求失败
    void onAuthTokenExpired();                           // 处理认证令牌过期

  private slots:
    void onTokenExpiryCheck(); // 定时检查令牌过期
    void onBaseUrlChanged();   // 服务器基础URL变化槽

  private:
    // 私有方法
    void 处理登录成功(const QJsonObject &response);
    void 处理令牌刷新成功(const QJsonObject &response);
    void 保存凭据();
    void 清除凭据();
    void 加载数据();
    void 开启令牌过期计时器();
    void 停止令牌过期计时器();
    void 是否发送首次认证信号();

    // 令牌管理
    void 刷新访问令牌();               // 刷新访问令牌
    bool 访问令牌是否即将过期() const; // 检查令牌是否即将过期

    // 成员变量
    NetworkRequest &m_networkRequest; ///< 网络管理器引用
    Setting &m_setting;               ///< 应用设置引用
    Database &m_database;             ///< 数据库管理器引用

    QString m_accessToken;  ///< 访问令牌
    QString m_refreshToken; ///< 刷新令牌
    QString m_username;     ///< 用户名
    QString m_email;        ///< 邮箱
    QUuid m_uuid;           ///< 用户UUID

    // 令牌管理
    QTimer *m_tokenExpiryTimer;      ///< 令牌过期检查定时器
    qint64 m_tokenExpiryTime;        ///< 令牌过期时间戳
    bool m_isRefreshing;             ///< 是否正在刷新令牌
    bool m_firstAuthEmitted = false; ///< 首次认证信号是否已经发出

    static const int TOKEN_REFRESH_THRESHOLD = DefaultValues::令牌刷新间隔; // (秒)

    // 服务器配置
    QString m_authApiEndpoint; ///< 认证API端点
};