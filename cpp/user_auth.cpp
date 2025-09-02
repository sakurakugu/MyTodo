/**
 * @file user_auth.cpp
 * @brief AuthManager类的实现文件
 *
 * 该文件实现了AuthManager类的所有方法，提供完整的用户认证功能。
 *
 * @author Sakurakugu
 * @date 2025-08-24 20:05:55(UTC+8) 周六
 * @version 2025-08-24 23:04:19(UTC+8) 周五
 */

#include "user_auth.h"
#include "default_value.h"
#include "setting.h"

#include <QDateTime>
#include <QDebug>
#include <QJsonObject>

UserAuth::UserAuth(QObject *parent)
    : QObject(parent),                                 //
      m_networkRequest(NetworkRequest::GetInstance()), //
      m_setting(Setting::GetInstance()),               //
      m_isOnline(false),                               //
      m_tokenExpiryTimer(new QTimer(this)),            //
      m_tokenExpiryTime(0),                            //
      m_isRefreshing(false) {                          //

    // 连接网络请求信号
    connect(&m_networkRequest, &NetworkRequest::requestCompleted, this, &UserAuth::onNetworkRequestCompleted);
    connect(&m_networkRequest, &NetworkRequest::requestFailed, this, &UserAuth::onNetworkRequestFailed);
    connect(&m_networkRequest, &NetworkRequest::authTokenExpired, this, &UserAuth::onAuthTokenExpired);
    connect(&m_networkRequest, &NetworkRequest::networkStatusChanged, this, &UserAuth::onNetworkStatusChanged);
    
    // 监听服务器配置变化
    connect(&m_setting, &Setting::baseUrlChanged, this, &UserAuth::onBaseUrlChanged);

    // 连接令牌过期检查定时器
    connect(m_tokenExpiryTimer, &QTimer::timeout, this, &UserAuth::onTokenExpiryCheck);
    m_tokenExpiryTimer->setSingleShot(false);
    m_tokenExpiryTimer->setInterval(60000); // 每分钟检查一次

    // 初始化服务器配置
    initializeServerConfig();

    // 加载存储的凭据
    loadStoredCredentials();
}

UserAuth::~UserAuth() {
    // 停止定时器
    stopTokenExpiryTimer();

    // 保存当前状态
    saveCredentials();
    qDebug() << "用户数据 已保存";
}

/**
 * @brief 使用用户凭据登录服务器
 * @param username 用户名
 * @param password 密码
 *
 * 登录结果会通过loginSuccessful或loginFailed信号通知。
 */
void UserAuth::login(const QString &username, const QString &password) {
    if (username.isEmpty() || password.isEmpty()) {
        emit loginFailed("用户名和密码不能为空");
        return;
    }

    qDebug() << "尝试登录用户:" << username;

    // 准备请求配置
    NetworkRequest::RequestConfig config;
    config.url = getApiUrl(m_authApiEndpoint) + "?action=login";
    config.method = "POST";      // 登录使用POST方法
    config.requiresAuth = false; // 登录请求不需要认证

    // 创建登录数据
    config.data["username"] = username;
    config.data["password"] = password;

    // 发送登录请求
    m_networkRequest.sendRequest(NetworkRequest::RequestType::Login, config);
}

/**
 * @brief 注销当前用户
 *
 * 清除存储的凭据并将所有项标记为未同步。
 */
void UserAuth::logout() {
    qDebug() << "用户" << m_username << "开始注销";

    // 清除凭据
    clearCredentials();

    // 发出信号
    emit usernameChanged();
    emit emailChanged();
    emit uuidChanged();
    emit isLoggedInChanged();
    emit logoutSuccessful();

    qDebug() << "用户注销完成";
}

bool UserAuth::isLoggedIn() const {
    return !m_accessToken.isEmpty();
}

QString UserAuth::getUsername() const {
    return m_username;
}

QString UserAuth::getEmail() const {
    return m_email;
}

QUuid UserAuth::getUuid() const {
    return m_uuid;
}

QString UserAuth::getAccessToken() const {
    return m_accessToken;
}

QString UserAuth::getRefreshToken() const {
    return m_refreshToken;
}

void UserAuth::setAuthToken(const QString &accessToken) {
    if (m_accessToken != accessToken) {
        m_accessToken = accessToken;
        m_networkRequest.setAuthToken(accessToken);
        saveCredentials();
    }
}

void UserAuth::refreshAccessToken() {
    if (m_refreshToken.isEmpty()) {
        qWarning() << "无法刷新令牌：刷新令牌为空";
        emit tokenRefreshFailed("刷新令牌不存在");
        return;
    }

    if (m_isRefreshing) {
        qDebug() << "令牌刷新已在进行中，跳过重复请求";
        return;
    }

    m_isRefreshing = true;
    emit tokenRefreshStarted();

    qDebug() << "开始刷新访问令牌...";

    // 准备刷新请求
    NetworkRequest::RequestConfig config;
    config.url = getApiUrl(m_authApiEndpoint) + "?action=refresh";
    config.method = "POST";
    config.requiresAuth = false; // 刷新请求不需要访问令牌认证
    config.data["refresh_token"] = m_refreshToken;

    // 发送刷新请求
    m_networkRequest.sendRequest(NetworkRequest::RequestType::RefreshToken, config);
}

bool UserAuth::isTokenExpiringSoon() const {
    if (m_tokenExpiryTime == 0) {
        return false;
    }

    qint64 currentTime = QDateTime::currentSecsSinceEpoch();
    qint64 timeUntilExpiry = m_tokenExpiryTime - currentTime;

    return timeUntilExpiry <= TOKEN_REFRESH_THRESHOLD;
}

/**
 * @brief 获取当前在线状态
 * @return 是否在线
 */
bool UserAuth::isOnline() const {
    return m_isOnline;
}

/**
 * @brief 设置在线状态
 * @param online 新的在线状态
 */
void UserAuth::setIsOnline(bool online) {
    // TODO： 这个函数好像没什么用，以前是自动同步，现在是否在线好像和是否登录冲突了
    // 如果已经是目标状态，则不做任何操作
    if (m_isOnline == online) {
        return;
    }

    if (online) {
        // TODO: 这部分是不是可以复用 和 network_request中的checkNetworkConnectivity

        // 尝试连接服务器，验证是否可以切换到在线模式
        NetworkRequest::RequestConfig config;
        config.url = getApiUrl(m_authApiEndpoint);
        config.method = "GET"; // 明确指定GET方法
        config.requiresAuth = UserAuth::GetInstance().isLoggedIn();
        config.timeout = 5000; // 5秒超时

        // 发送测试请求
        m_networkRequest.sendRequest(NetworkRequest::FetchTodos, config);

        // TODO: 暂时设置为在线状态，实际状态将在请求回调中确定
        m_isOnline = online;
        emit isOnlineChanged();
        // 保存到设置，保持与autoSync一致
        // m_setting.save(QStringLiteral("sync/autoSyncEnabled"), m_isOnline);
    } else {
        // 切换到离线模式不需要验证，直接更新状态
        m_isOnline = online;
        emit isOnlineChanged();
        // 保存到设置，保持与autoSync一致
        // m_setting.save(QStringLiteral("sync/autoSyncEnabled"), m_isOnline);
    }
}

void UserAuth::onNetworkStatusChanged(bool isOnline) {
    if (m_isOnline != isOnline) {
        m_isOnline = isOnline;
        emit isOnlineChanged();
        qDebug() << "网络状态变更:" << (isOnline ? "在线" : "离线");
    }
}

void UserAuth::setAuthApiEndpoint(const QString &endpoint) {
    m_authApiEndpoint = endpoint;
}

QString UserAuth::getAuthApiEndpoint() const {
    return m_authApiEndpoint;
}

void UserAuth::onNetworkRequestCompleted(NetworkRequest::RequestType type, const QJsonObject &response) {
    switch (type) {
    case NetworkRequest::RequestType::Login:
        handleLoginSuccess(response);
        break;
    case NetworkRequest::RequestType::RefreshToken:
        handleTokenRefreshSuccess(response);
        break;
    case NetworkRequest::RequestType::Logout:
        // 注销成功处理
        emit logoutSuccessful();
        break;
    case NetworkRequest::RequestType::FetchTodos:
        // 如果是token验证请求成功，说明token有效
        qDebug() << "存储的访问令牌验证成功，用户已自动登录：" << m_username;
        emit loginSuccessful(m_username);
        break;
    default:
        // 其他请求类型不在此处理
        break;
    }
}

void UserAuth::onNetworkRequestFailed(NetworkRequest::RequestType type, NetworkRequest::NetworkError error,
                                      const QString &message) {

    switch (type) {
    case NetworkRequest::RequestType::Login:
        qWarning() << "登录失败:" << message;
        emit loginFailed(message);
        break;
    case NetworkRequest::RequestType::RefreshToken:
        m_isRefreshing = false;
        qWarning() << "令牌刷新失败:" << message;
        emit tokenRefreshFailed(message);

        // 如果刷新失败，触发重新登录
        if (error == NetworkRequest::NetworkError::AuthenticationError) {
            qWarning() << "刷新令牌无效，需要重新登录";
            emit authTokenExpired();
        }
        break;
    case NetworkRequest::RequestType::Logout:
        qWarning() << "注销失败:" << message;
        // 即使注销失败，也清除本地凭据
        clearCredentials();
        emit logoutSuccessful();
        break;
    case NetworkRequest::RequestType::FetchTodos:
        // 如果是token验证请求失败，说明token无效
        if (error == NetworkRequest::NetworkError::AuthenticationError) {
            qWarning() << "存储的访问令牌无效，尝试静默刷新";
            if (!m_isRefreshing && !m_refreshToken.isEmpty()) {
                performSilentRefresh();
            } else {
                clearCredentials();
                emit loginRequired();
            }
            return;
        }
        break;
    default:
        // 其他请求类型的错误处理
        if (error == NetworkRequest::NetworkError::AuthenticationError) {
            qWarning() << "认证错误，尝试静默刷新:" << message;
            if (!m_isRefreshing && !m_refreshToken.isEmpty()) {
                performSilentRefresh();
            } else {
                emit authTokenExpired();
            }
        }
        break;
    }
}

void UserAuth::onAuthTokenExpired() {
    qWarning() << "认证令牌已过期或无效";
    
    // 尝试使用refresh token自动刷新
    if (!m_refreshToken.isEmpty() && !m_isRefreshing) {
        qDebug() << "尝试使用refresh token自动刷新访问令牌";
        refreshAccessToken();
    } else {
        qWarning() << "无法自动刷新令牌，需要重新登录";
        clearCredentials();
        emit usernameChanged();
        emit emailChanged();
        emit uuidChanged();
        emit isLoggedInChanged();
        emit loginRequired();
    }
}

void UserAuth::handleLoginSuccess(const QJsonObject &response) {
    // 验证响应中包含必要的字段
    if (!response.contains("access_token") || !response.contains("refresh_token") || !response.contains("user")) {
        emit loginFailed("服务器响应缺少必要字段");
        return;
    }

    // 提取认证信息
    m_accessToken = response["access_token"].toString();
    m_refreshToken = response["refresh_token"].toString();

    QJsonObject userObj = response["user"].toObject();
    m_username = userObj["username"].toString();
    m_email = userObj.value("email").toString();
    m_uuid = QUuid::fromString(userObj.value("uuid").toString());

    // 设置令牌过期时间
    if (response.contains("expires_in")) {
        qint64 expiresIn = response["expires_in"].toInt();
        setTokenExpiryTime(QDateTime::currentSecsSinceEpoch() + expiresIn);
    }

    // 设置网络管理器的认证令牌
    m_networkRequest.setAuthToken(m_accessToken);

    // 启动令牌过期检查定时器
    startTokenExpiryTimer();

    // 保存凭据
    saveCredentials();

    qDebug() << "用户" << m_username << "登录成功";

    // 发出信号
    emit usernameChanged();
    emit emailChanged();
    emit uuidChanged();
    emit isLoggedInChanged();
    emit loginSuccessful(m_username);
}

void UserAuth::handleTokenRefreshSuccess(const QJsonObject &response) {
    m_isRefreshing = false;

    // 验证响应中包含必要的字段
    if (!response.contains("access_token")) {
        emit tokenRefreshFailed("服务器响应缺少访问令牌");
        return;
    }

    // 更新访问令牌
    m_accessToken = response["access_token"].toString();

    // 设置令牌过期时间
    if (response.contains("expires_in")) {
        qint64 expiresIn = response["expires_in"].toInt();
        setTokenExpiryTime(QDateTime::currentSecsSinceEpoch() + expiresIn);
    }

    // 设置网络管理器的认证令牌
    m_networkRequest.setAuthToken(m_accessToken);

    // 保存凭据
    saveCredentials();

    qDebug() << "访问令牌刷新成功";
    emit tokenRefreshSuccessful();
}

void UserAuth::loadStoredCredentials() {
    // 尝试从设置中加载存储的凭据
    if (m_setting.contains(QStringLiteral("user/accessToken"))) {
        m_accessToken = m_setting.get(QStringLiteral("user/accessToken")).toString();
        m_refreshToken = m_setting.get(QStringLiteral("user/refreshToken")).toString();
        m_username = m_setting.get(QStringLiteral("user/username")).toString();
        m_email = m_setting.get(QStringLiteral("user/email")).toString();
        m_uuid = QUuid::fromString(m_setting.get(QStringLiteral("user/uuid")).toString());

        // 加载令牌过期时间
        m_tokenExpiryTime = m_setting.get(QStringLiteral("user/tokenExpiryTime"), 0).toLongLong();

        // 设置网络管理器的认证令牌
        if (!m_accessToken.isEmpty()) {
            m_networkRequest.setAuthToken(m_accessToken);
            qDebug() << "加载存储的凭据，用户：" << m_username;

            // 启动令牌过期检查定时器
            startTokenExpiryTimer();

            // 验证令牌是否仍然有效
            validateStoredToken();
        }
    }
}

void UserAuth::validateStoredToken() {
    if (m_accessToken.isEmpty()) {
        return;
    }

    qDebug() << "验证存储的访问令牌有效性...";
    
    // 简单检查令牌格式（JWT应该有3个点分隔的部分）
    QStringList parts = m_accessToken.split('.');
    if (parts.size() != 3) {
        qWarning() << "访问令牌格式错误，包含" << parts.size() << "个部分，应该是3个";
        // 清除损坏的令牌
        logout();
        return;
    }
    
    // 检查每个部分是否为空
    for (int i = 0; i < parts.size(); ++i) {
        if (parts[i].isEmpty()) {
            qWarning() << "访问令牌第" << (i+1) << "部分为空";
            logout();
            return;
        }
    }
    
    qDebug() << "令牌格式检查通过，发送验证请求到服务器...";

    // 发送一个简单的验证请求到服务器
    NetworkRequest::RequestConfig config;
    config.url = getApiUrl("/todo/todo_api.php/health"); // 使用health端点验证
    config.method = "GET";
    config.requiresAuth = true;
    config.timeout = 5000; // 5秒超时

    // 这个请求会通过onNetworkRequestCompleted/Failed处理
    m_networkRequest.sendRequest(NetworkRequest::RequestType::FetchTodos, config);
}

void UserAuth::saveCredentials() {
    // 保存凭据到本地设置
    if (!m_accessToken.isEmpty()) {
        m_setting.save(QStringLiteral("user/accessToken"), m_accessToken);
        m_setting.save(QStringLiteral("user/refreshToken"), m_refreshToken);
        m_setting.save(QStringLiteral("user/username"), m_username);
        m_setting.save(QStringLiteral("user/email"), m_email);
        m_setting.save(QStringLiteral("user/uuid"), m_uuid);
        m_setting.save(QStringLiteral("user/tokenExpiryTime"), m_tokenExpiryTime);
    }
}

void UserAuth::clearCredentials() {
    // 停止令牌过期检查定时器
    stopTokenExpiryTimer();

    // 清除内存中的凭据
    m_accessToken.clear();
    m_refreshToken.clear();
    m_username.clear();
    m_email.clear();
    m_uuid = QUuid();
    m_tokenExpiryTime = 0;
    m_isRefreshing = false;

    // 清除设置中的凭据
    m_setting.remove(QStringLiteral("user/accessToken"));
    m_setting.remove(QStringLiteral("user/refreshToken"));
    m_setting.remove(QStringLiteral("user/username"));
    m_setting.remove(QStringLiteral("user/email"));
    m_setting.remove(QStringLiteral("user/uuid"));
    m_setting.remove(QStringLiteral("user/tokenExpiryTime"));

    // 清除网络管理器的认证令牌
    m_networkRequest.setAuthToken("");
}

void UserAuth::onTokenExpiryCheck() {
    if (isTokenExpiringSoon() && !m_isRefreshing) {
        performSilentRefresh();
    }
}

void UserAuth::startTokenExpiryTimer() {
    if (m_tokenExpiryTimer && !m_tokenExpiryTimer->isActive()) {
        m_tokenExpiryTimer->start(60000); // 每分钟检查一次
    }
}

void UserAuth::stopTokenExpiryTimer() {
    if (m_tokenExpiryTimer && m_tokenExpiryTimer->isActive()) {
        m_tokenExpiryTimer->stop();
    }
}

void UserAuth::performSilentRefresh() {
    if (m_refreshToken.isEmpty() || m_isRefreshing) {
        return;
    }

    m_isRefreshing = true;
    emit tokenRefreshStarted();

    qDebug() << "开始静默刷新访问令牌";
    refreshAccessToken();
}

qint64 UserAuth::getTokenExpiryTime() const {
    return m_tokenExpiryTime;
}

void UserAuth::setTokenExpiryTime(qint64 expiryTime) {
    m_tokenExpiryTime = expiryTime;
}

QString UserAuth::getApiUrl(const QString &endpoint) const {
    if (m_serverBaseUrl.isEmpty()) {
        return endpoint;
    }

    QString baseUrl = m_serverBaseUrl;
    if (!baseUrl.endsWith('/')) {
        baseUrl += '/';
    }

    QString cleanEndpoint = endpoint;
    if (cleanEndpoint.startsWith('/')) {
        cleanEndpoint = cleanEndpoint.mid(1);
    }

    return baseUrl + cleanEndpoint;
}

void UserAuth::initializeServerConfig() {
    // 从设置中加载服务器配置
    m_serverBaseUrl =
        m_setting.get(QStringLiteral("server/baseUrl"), QString::fromStdString(std::string{DefaultValues::baseUrl}))
            .toString();
    m_authApiEndpoint = m_setting
                            .get(QStringLiteral("server/authApiEndpoint"),
                                 QString::fromStdString(std::string{DefaultValues::userAuthApiEndpoint}))
                            .toString();

    qDebug() << "服务器配置 - 基础URL:" << m_serverBaseUrl << ", 认证端点:" << m_authApiEndpoint;
}

void UserAuth::onBaseUrlChanged(const QString &newBaseUrl) {
    qDebug() << "UserAuth: 服务器基础URL已更新:" << m_serverBaseUrl << "->" << newBaseUrl;
    m_serverBaseUrl = newBaseUrl;
    m_networkRequest.setServerConfig(newBaseUrl);
    logout();
}