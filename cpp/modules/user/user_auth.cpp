/**
 * @file user_auth.cpp
 * @brief UserAuth 类实现文件
 *
 * 实现 UserAuth 用户认证管理类的全部逻辑，包括：
 * - 登录/注销流程与输入校验
 * - 刷新令牌驱动的访问令牌自动刷新机制
 * - 令牌过期定时检测与失败回退策略
 * - 首次完成认证的单次信号派发（用于触发同步等后续动作）
 * - 与数据库交互以持久化及恢复用户状态
 *
 * @author Sakurakugu
 * @date 2025-08-24 20:05:55 (UTC+8) 周日
 * @change 2025-10-06 02:13:23(UTC+8) 周一
 */

#include "user_auth.h"
#include "config.h"
#include "database.h"
#include "default_value.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <chrono>
#include <regex>

UserAuth::UserAuth(QObject *parent)
    : QObject(parent),                                 //
      m_networkRequest(NetworkRequest::GetInstance()), //
      m_database(Database::GetInstance()),             //
      m_tokenExpiryTimer(new QTimer(this)),            //
      m_tokenExpiryTime(0),                            //
      m_isRefreshing(false)                            //
{
    // 初始化用户表
    if (!初始化用户表()) {
        qCritical() << "用户表初始化失败";
    }

    // 连接网络请求信号
    connect(&m_networkRequest, &NetworkRequest::requestCompleted, this, &UserAuth::onNetworkRequestCompleted);
    connect(&m_networkRequest, &NetworkRequest::requestFailed, this, &UserAuth::onNetworkRequestFailed);
    connect(&m_networkRequest, &NetworkRequest::authTokenExpired, this, &UserAuth::onAuthTokenExpired);

    // 连接令牌过期检查定时器
    connect(m_tokenExpiryTimer, &QTimer::timeout, this, &UserAuth::onTokenExpiryCheck);
    m_tokenExpiryTimer->setSingleShot(true); // 单次触发；触发后如需继续会在逻辑里重新安排

    // 初始化服务器配置
    加载数据();

    // 注册到数据库导出器
    m_database.registerDataExporter("users", this);
}

void UserAuth::加载数据() {
    // 从设置中加载服务器配置
    m_authApiEndpoint = Config::GetInstance()
                            .get("server/authApiEndpoint", QString(DefaultValues::userAuthApiEndpoint))
                            .toString()
                            .toStdString();

    // 尝试从数据库中加载存储的凭据
    auto query = m_database.createQuery();

    if (query->exec("SELECT uuid, username, email, refreshToken FROM users LIMIT 1")) {
        auto rows = query->fetchAll();
        if (!rows.empty()) {
            const auto &row = rows[0];
            for (const auto &[columnName, value] : row) {
                if (columnName == "uuid") {
                    m_uuid = sqlValueCast<uuids::uuid>(value);
                } else if (columnName == "username") {
                    m_username = sqlValueCast<std::string>(value);
                } else if (columnName == "email") {
                    m_email = sqlValueCast<std::string>(value);
                } else if (columnName == "refreshToken") {
                    m_refreshToken = sqlValueCast<std::string>(value);
                }
            }
        }
    } else {
        qWarning() << "查询用户凭据失败:" << query->lastErrorQt();
    }

    刷新访问令牌(); // 使用刷新令牌获取新的访问令牌

    qDebug() << "服务器配置: " << m_networkRequest.getApiUrl(QString::fromStdString(m_authApiEndpoint));
}

UserAuth::~UserAuth() {
    停止令牌过期计时器();

    // 从数据库导出器中注销
    m_database.unregisterDataExporter("users");
}

/**
 * @brief 使用用户凭据登录服务器
 * @param account 用户名或邮箱
 * @param password 密码
 *
 * 登录结果会通过loginSuccessful或loginFailed信号通知。
 */
void UserAuth::登录(const std::string &account, const std::string &password) {
    if (password.empty()) {
        emit loginFailed("密码不能为空");
        return;
    }

    // 简单区分用户名和邮箱
    if (account.find('@') != std::string::npos) {
        // 简单的邮箱格式验证
        if (account.find('.') == std::string::npos || account.find('@') == 0 ||
            account.find('@') == account.size() - 1) {
            emit loginFailed("无效的邮箱格式");
            return;
        }
    } else {
        // 用户名严格验证：仅字母、数字、下划线，长度 3-20
        static const std::regex re("^[A-Za-z0-9_]{3,20}$");
        if (!std::regex_match(account, re)) {
            emit loginFailed("用户名只能包含字母、数字或下划线，长度应为3-20个字符");
            return;
        }
    }

    qDebug() << "尝试登录账户:" << account;

    // 准备请求配置
    NetworkRequest::RequestConfig config;
    config.url = m_networkRequest.getApiUrl(QString::fromStdString(m_authApiEndpoint)) + "?action=login";
    config.method = "POST";      // 登录使用POST方法
    config.requiresAuth = false; // 登录请求不需要认证

    // 创建登录数据
    config.data["account"] = QString::fromStdString(account);
    config.data["password"] = QString::fromStdString(password);

    // 发送登录请求
    m_networkRequest.sendRequest(Network::RequestType::Login, config);
}

/**
 * @brief 注销当前用户
 *
 * 清除存储的凭据并将所有项标记为未同步。
 */
void UserAuth::注销() {
    清除凭据();
    emit logoutSuccessful();
}

bool UserAuth::是否已登录() const {
    // 检查访问令牌是否存在
    if (m_accessToken.empty()) {
        return false;
    } else [[likely]] {
        return true;
    }
}

std::string UserAuth::获取用户名() const {
    return m_username;
}

std::string UserAuth::获取邮箱() const {
    return m_email;
}

uuids::uuid UserAuth::获取UUID() const {
    return m_uuid;
}

void UserAuth::刷新访问令牌() {
    if (m_uuid.is_nil()) {
        qDebug() << "无法刷新令牌：用户未登录";
        emit tokenRefreshFailed("用户未登录");
        return;
    }

    if (m_refreshToken.empty()) {
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
    config.url = m_networkRequest.getApiUrl(QString::fromStdString(m_authApiEndpoint)) + "?action=refresh";
    config.method = "POST";
    config.requiresAuth = false; // 刷新请求不需要访问令牌认证
    config.data["refresh_token"] = QString::fromStdString(m_refreshToken);

    // 发送刷新请求
    m_networkRequest.sendRequest(Network::RequestType::RefreshToken, config);
}

void UserAuth::onNetworkRequestCompleted(Network::RequestType type, const QJsonObject &response) {
    switch (type) {
    case Network::RequestType::Login:
        处理登录成功(response);
        break;
    case Network::RequestType::RefreshToken:
        处理令牌刷新成功(response);
        break;
    case Network::RequestType::Logout:
        // 注销成功处理
        emit logoutSuccessful();
        break;
    case Network::RequestType::FetchTodos:
        // 如果是token验证请求成功，说明token有效
        qDebug() << "存储的访问令牌验证成功，用户已自动登录：" << m_username;
        break;
    default:
        // 其他请求类型不在此处理
        break;
    }
}

void UserAuth::onNetworkRequestFailed(Network::RequestType type, Network::Error error, const QString &message) {

    switch (type) {
    case Network::RequestType::Login:
        qWarning() << message;
        emit loginFailed(message);
        break;
    case Network::RequestType::RefreshToken:
        m_isRefreshing = false;
        qWarning() << "令牌刷新失败:" << message << "错误类型:" << static_cast<int>(error);
        emit tokenRefreshFailed(message);

        // 根据错误类型进行不同处理
        if (error == Network::Error::AuthenticationError) {
            qWarning() << "刷新令牌无效或已过期，清理凭据并要求重新登录";
            清除凭据();
            emit loginRequired();
        } else {
            qWarning() << "令牌刷新网络错误，将在下次同步时重试";
            // 网络错误时不清理凭据，保留用户状态
        }
        break;
    case Network::RequestType::Logout:
        qWarning() << "注销失败:" << message;
        // 即使注销失败，也清除本地凭据
        清除凭据();
        emit logoutSuccessful();
        break;
    case Network::RequestType::FetchTodos:
        // 如果是token验证请求失败，说明token无效
        if (error == Network::Error::AuthenticationError) {
            qWarning() << "存储的访问令牌无效，尝试静默刷新";
            if (!m_isRefreshing && !m_refreshToken.empty()) {
                刷新访问令牌();
            } else {
                清除凭据();
                emit loginRequired();
            }
            return;
        }
        break;
    default:
        // 其他请求类型的错误处理
        if (error == Network::Error::AuthenticationError) {
            qWarning() << "认证错误，尝试静默刷新:" << message;
            if (!m_isRefreshing && !m_refreshToken.empty()) {
                刷新访问令牌();
            } else {
                emit authTokenExpired();
            }
        }
        break;
    }
}

void UserAuth::onAuthTokenExpired() {
    qWarning() << "认证令牌已过期或无效，当前时间:" << QDateTime::currentSecsSinceEpoch()
               << "令牌过期时间:" << m_tokenExpiryTime;

    停止令牌过期计时器();

    // 尝试使用refresh token自动刷新
    if (!m_refreshToken.empty() && !m_isRefreshing) {
        qDebug() << "尝试使用refresh token自动刷新访问令牌";
        刷新访问令牌();
    } else {
        if (m_refreshToken.empty()) {
            qWarning() << "刷新令牌为空，无法自动刷新，需要重新登录";
        } else if (m_isRefreshing) {
            qWarning() << "令牌刷新已在进行中，等待刷新结果";
            return; // 避免重复清理
        }

        qWarning() << "无法自动刷新令牌，清理用户状态并要求重新登录";
        清除凭据();
        emit loginRequired();
    }
}

void UserAuth::处理登录成功(const QJsonObject &response) {
    // 验证响应中包含必要的字段（允许 user.email 缺失）
    if (!response.contains("access_token") || !response.contains("refresh_token") || !response.contains("user")) {
        emit loginFailed("服务器响应缺少必要字段");
        return;
    }

    // 提取认证信息
    m_accessToken = response["access_token"].toString().toStdString();
    m_refreshToken = response["refresh_token"].toString().toStdString();

    QJsonObject userObj = response["user"].toObject();
    m_username = userObj["username"].toString().toStdString();
    if (m_username.empty()) {
        emit loginFailed("服务器响应缺少用户名");
        return;
    }

    // email 允许为空
    if (!userObj.contains("email")) {
        qWarning() << "登录响应中缺少 email 字段，使用空字符串";
    }
    m_email = userObj["email"].toString().toStdString();
    m_uuid = uuids::uuid::from_string(userObj["uuid"].toString().toStdString()).value_or(uuids::uuid{});

    if (m_uuid.is_nil()) {
        emit loginFailed("服务器响应缺少有效的用户UUID");
        return;
    }

    // 设置令牌过期时间（从服务端返回过期时间，若未返回则使用默认值）
    if (response.contains("expires_in")) {
        qint64 expiresIn = response["expires_in"].toInt();
        if (expiresIn <= 0 || expiresIn > ACCESS_TOKEN_LIFETIME) {
            expiresIn = ACCESS_TOKEN_LIFETIME; // 防御：限制在预期范围
        }
        m_tokenExpiryTime = QDateTime::currentSecsSinceEpoch() + expiresIn;
    } else {
        m_tokenExpiryTime = QDateTime::currentSecsSinceEpoch() + ACCESS_TOKEN_LIFETIME;
    }

    m_networkRequest.setAuthToken(m_accessToken.c_str());
    开启令牌过期计时器();
    保存凭据();

    // qDebug() << "用户" << m_username << "登录成功 (email=" << m_email << ")";
    qDebug() << "用户" << m_username;

    emit isLoggedInChanged();
    emit loginSuccessful(QString::fromStdString(m_username));
    是否发送首次认证信号();
}

void UserAuth::处理令牌刷新成功(const QJsonObject &response) {
    m_isRefreshing = false;

    // 验证响应中包含必要的字段
    if (!response.contains("access_token")) {
        qWarning() << "令牌刷新响应中缺少access_token字段";
        emit tokenRefreshFailed("服务器响应缺少访问令牌");
        return;
    }

    // 更新访问令牌
    m_accessToken = response["access_token"].toString().toStdString();
    std::string refreshToken = response["refresh_token"].toString().toStdString();
    if (!refreshToken.empty()) {
        m_refreshToken = refreshToken;
    }

    // 设置令牌过期时间（若服务端未返回，用默认访问令牌生命周期）
    int64_t now =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if (response.contains("expires_in")) {
        int64_t expiresIn = response["expires_in"].toInt();
        if (expiresIn <= 0 || expiresIn > ACCESS_TOKEN_LIFETIME) {
            expiresIn = ACCESS_TOKEN_LIFETIME;
        }
        m_tokenExpiryTime = now + expiresIn;
        qDebug() << "令牌过期时间已更新:" << m_tokenExpiryTime << "有效期:" << expiresIn << "秒";
    } else {
        m_tokenExpiryTime = now + ACCESS_TOKEN_LIFETIME;
        qDebug() << "令牌过期时间使用默认生命周期 ACCESS_TOKEN_LIFETIME:" << ACCESS_TOKEN_LIFETIME;
    }

    // 更新刷新令牌（如果提供）
    if (response.contains("refresh_token")) {
        m_refreshToken = response["refresh_token"].toString().toStdString();
        保存凭据();
        qDebug() << "刷新令牌已更新";
    }

    // 设置网络管理器的认证令牌
    m_networkRequest.setAuthToken(m_accessToken.c_str());

    // 重新启动令牌过期检查定时器
    开启令牌过期计时器();

    qDebug() << "访问令牌刷新成功，定时器已重新启动";
    emit isLoggedInChanged();
    emit tokenRefreshSuccessful();
    是否发送首次认证信号();
}

void UserAuth::保存凭据() {
    // 保存凭据到数据库
    if (!m_refreshToken.empty() && !m_uuid.is_nil()) {
        auto query = m_database.createQuery();
        query->prepare(R"(REPLACE INTO users (uuid, username, email, refreshToken) VALUES (?, ?, ?, ?))");
        query->bindValues(m_uuid,        //
                          m_username,    //
                          m_email,       //
                          m_refreshToken //
        );

        if (!query->exec()) {
            qWarning() << "保存用户凭据到数据库失败:" << query->lastErrorQt();
        }

        emit usernameChanged();
        emit emailChanged();
        emit uuidChanged();
    }
}

void UserAuth::清除凭据() {
    停止令牌过期计时器();

    // 重置刷新状态
    m_isRefreshing = false;

    // 重置首次认证信号状态
    m_firstAuthEmitted = false;

    // 清除内存中的凭据
    m_accessToken.clear();
    m_refreshToken.clear();
    m_username.clear();
    m_email.clear();
    m_uuid = uuids::uuid{};
    m_tokenExpiryTime = 0;

    // 清除数据库中的凭据
    auto query = m_database.createQuery();
    if (!query->exec("DELETE FROM users")) {
        qWarning() << "清除数据库中的用户凭据失败:" << query->lastErrorQt();
    }

    // 清除网络管理器的认证令牌
    m_networkRequest.setAuthToken("");

    emit usernameChanged();
    emit emailChanged();
    emit uuidChanged();
    emit isLoggedInChanged();

    qDebug() << "已清除用户凭据";
}

void UserAuth::是否发送首次认证信号() {
    if (!m_firstAuthEmitted && !m_accessToken.empty()) {
        m_firstAuthEmitted = true;
        emit firstAuthCompleted();
        qDebug() << "首次认证完成信号已发出";
    }
}

void UserAuth::onTokenExpiryCheck() {
    // 到达预定刷新窗口，若仍未刷新则执行刷新；若已经刷新，计时器会在刷新成功里重新安排
    if (!m_isRefreshing) {
        qint64 now = QDateTime::currentSecsSinceEpoch();
        if (m_tokenExpiryTime > 0 && m_tokenExpiryTime - now <= ACCESS_TOKEN_REFRESH_AHEAD) {
            qDebug() << "到达访问令牌预刷新窗口，执行刷新";
            刷新访问令牌();
        } else if (m_tokenExpiryTime <= now) {
            qWarning() << "访问令牌已过期，触发过期处理";
            emit authTokenExpired();
        } else {
            // 未到窗口（极少出现，除非外部直接调用 start 了一个不匹配的计时），重新调度
            开启令牌过期计时器();
        }
    } else {
        qDebug() << "刷新进行中，忽略本次 onTokenExpiryCheck";
    }
}

void UserAuth::开启令牌过期计时器() {
    if (!m_tokenExpiryTimer)
        return;

    m_tokenExpiryTimer->stop(); // 重置任何旧的调度

    if (m_tokenExpiryTime == 0) {
        qDebug() << "未设置 token 过期时间，不安排刷新";
        return;
    }

    int64_t now =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t timeUntilExpiry = m_tokenExpiryTime - now;
    if (timeUntilExpiry <= 0) {
        qWarning() << "访问令牌已过期或时间异常，立即尝试刷新";
        onTokenExpiryCheck();
        return;
    }

    int64_t refreshDelaySec = timeUntilExpiry - ACCESS_TOKEN_REFRESH_AHEAD;
    if (refreshDelaySec < 0) {
        // 已经进入预刷新窗口，立即（稍微延迟 100ms）刷新，避免紧贴事件循环造成阻塞
        refreshDelaySec = 0;
    }

    int delayMs;
    // 防御：最大不超过 24h 单次（Qt int 限制），但这里 1h 足够
    if (refreshDelaySec > 24 * 3600) {
        refreshDelaySec = 24 * 3600;
    }
    delayMs = static_cast<int>(refreshDelaySec * 1000);

    m_tokenExpiryTimer->start(delayMs);
    // qDebug() << "计划在" << refreshDelaySec << "秒后进入预刷新窗口 (token 剩余" << timeUntilExpiry << "秒)";
}

void UserAuth::停止令牌过期计时器() {
    if (m_tokenExpiryTimer) {
        m_tokenExpiryTimer->stop();
    }
}

/**
 * @brief 初始化用户表
 * @return 初始化是否成功
 */
bool UserAuth::初始化用户表() {
    // 确保数据库已初始化
    if (!m_database.initialize()) {
        return false;
    }

    return 创建用户表();
}

/**
 * @brief 创建用户表
 * @return 创建是否成功
 */
bool UserAuth::创建用户表() {
    auto query = m_database.createQuery();
    const std::string createTableQuery = R"(
        CREATE TABLE IF NOT EXISTS users (
            uuid TEXT PRIMARY KEY NOT NULL,
            username TEXT NOT NULL,
            email TEXT NOT NULL,
            refreshToken TEXT NOT NULL
        )
    )";

    if (!query->exec(createTableQuery)) {
        qCritical() << "创建用户表失败:" << query->lastErrorQt();
        return false;
    }

    qDebug() << "用户表初始化成功";
    return true;
}

/**
 * @brief 导出用户数据到JSON对象
 */
bool UserAuth::exportToJson(QJsonObject &output) {
    auto query = m_database.createQuery();
    if (!query->exec("SELECT uuid, username, email FROM users")) {
        qWarning() << "查询用户数据失败:" << query->lastErrorQt();
        return false;
    }

    SqlMapResultSet resultMap = query->fetchAllMap();

    if (resultMap.empty()) {
        qWarning() << "查询用户数据失败:" << query->lastErrorQt();
        return false;
    }

    QJsonArray usersArray;
    for (const auto &row : resultMap) {
        QJsonObject userObj;
        userObj["uuid"] = QString::fromStdString(sqlValueCast<std::string>(row.at("uuid")));
        userObj["username"] = QString::fromStdString(sqlValueCast<std::string>(row.at("username")));
        userObj["email"] = QString::fromStdString(sqlValueCast<std::string>(row.at("email")));
        usersArray.append(userObj);
    }

    output["users"] = usersArray;
    return true;
}

/**
 * @brief 从JSON对象导入用户数据
 */
bool UserAuth::importFromJson(const QJsonObject &input, bool replaceAll) {
    if (!input.contains("users") || !input["users"].isArray()) {
        // 没有用户数据或格式错误，但不视为错误
        return true;
    }

    // 如果是替换模式，先清空表
    if (replaceAll) {
        auto query = m_database.createQuery();
        if (!query->exec("DELETE FROM users")) {
            qWarning() << "清空用户表失败:" << query->lastErrorQt();
            return false;
        }
    }

    // 导入用户数据
    QJsonObject userObj = input["users"].toObject();

    std::string insertQuery = "INSERT OR REPLACE INTO users (uuid, username, email) VALUES (?, ?, ?)";
    auto query = m_database.createQuery();
    query->bindValues(userObj["uuid"].toString().toStdString(),     //
                      userObj["username"].toString().toStdString(), //
                      userObj["email"].toString().toStdString());

    if (!query->exec(insertQuery)) {
        qWarning() << "导入用户数据失败:" << query->lastErrorQt();
        return false;
    }

    qInfo() << "成功导入用户记录";
    return true;
}