/**
 * @file network_request.cpp
 * @brief 网络请求类实现
 * @author Sakurakugu
 * @date 2025-08-17 07:17:29(UTC+8) 周日
 * @version 2025-08-22 23:04:19(UTC+8) 周五
 */
#include "network_request.h"
#include "network_proxy.h"
#include "config.h"

#include <QCoreApplication>
#include <QDebug>
#include <QJsonParseError>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslError>
#include <QTimer>
#include <QUrl>

/**
 * @brief 构造函数
 *
 * 创建NetworkRequest实例，初始化网络访问管理器和连接监控定时器。
 *
 * @param parent 父对象指针，用于Qt对象树管理
 */
NetworkRequest::NetworkRequest(QObject *parent)
    : QObject(parent),
      m_networkRequest(new QNetworkAccessManager(this)),
      m_nextRequestId(1),
      m_isOnline(false),
      m_connectivityTimer(new QTimer(this)),
      m_proxyManager(NetworkProxy::GetInstance()) {
    // 设置默认配置
    m_networkRequest->setProxy(QNetworkProxy::NoProxy);

    // 设置网络连接检查定时器
    m_connectivityTimer->setInterval(CONNECTIVITY_CHECK_INTERVAL);
    connect(m_connectivityTimer, &QTimer::timeout, this, &NetworkRequest::checkNetworkConnectivity);
    m_connectivityTimer->start();

    // 初始网络状态检查
    checkNetworkConnectivity();
    
    // 应用代理配置到网络管理器
    m_proxyManager.applyProxyToManager(m_networkRequest);
}

/**
 * @brief 析构函数
 *
 * 清理所有待处理的请求，停止定时器，释放网络资源。
 */
NetworkRequest::~NetworkRequest() { cancelAllRequests(); }

/**
 * @brief 设置认证令牌
 * @param token 访问令牌字符串
 */
void NetworkRequest::setAuthToken(const QString &token) { m_authToken = token; }

/**
 * @brief 清除认证令牌
 */
void NetworkRequest::clearAuthToken() { m_authToken.clear(); }

/**
 * @brief 检查是否有有效的认证信息
 * @return 如果有有效认证令牌返回true，否则返回false
 */
bool NetworkRequest::hasValidAuth() const { return !m_authToken.isEmpty(); }

/**
 * @brief 设置服务器地址与api版本
 * @param baseUrl 服务器基础URL
 * @param apiVersion API版本，默认为"v1"
 */
void NetworkRequest::setServerConfig(const QString &baseUrl, const QString &apiVersion) {
    m_serverBaseUrl = baseUrl;
    m_apiVersion = apiVersion;

    // 确保URL以斜杠结尾
    if (!m_serverBaseUrl.endsWith('/')) {
        m_serverBaseUrl += '/';
    }
}

/**
 * @brief 获取完整的API URL
 * @param endpoint API端点路径
 * @return 完整的API URL
 */
QString NetworkRequest::getApiUrl(const QString &endpoint) const {
    QString url = m_serverBaseUrl;
    if (!m_apiVersion.isEmpty()) {
        url += QString("api/%1/").arg(m_apiVersion);
    }
    url += endpoint;
    return url;
}

void NetworkRequest::sendRequest(RequestType type, const RequestConfig &config) {
    // 检查网络连接
    if (!m_isOnline) {
        emit requestFailed(type, ConnectionError, "网络连接不可用");
        return;
    }

    // 检查是否为重复请求
    if (isDuplicateRequest(type)) {
        qDebug() << "忽略重复请求:" << type;
        return;
    }

    // 检查认证要求
    if (config.requiresAuth && !hasValidAuth()) {
        emit requestFailed(type, AuthenticationError, "缺少有效的认证令牌");
        return;
    }

    // 创建待处理请求
    PendingRequest request;
    request.type = type;
    request.config = config;
    request.currentRetry = 0;
    request.requestId = m_nextRequestId++;

    // 创建超时定时器
    request.timeoutTimer = new QTimer(this);
    request.timeoutTimer->setSingleShot(true);
    request.timeoutTimer->setInterval(config.timeout);
    connect(request.timeoutTimer, &QTimer::timeout, this, &NetworkRequest::onRequestTimeout);

    // 存储请求
    m_pendingRequests[request.requestId] = request;
    addActiveRequest(type, request.requestId);

    // 执行请求
    executeRequest(m_pendingRequests[request.requestId]);
}

void NetworkRequest::onReplyFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) {
        return;
    }

    // 查找对应的请求
    qint64 requestId = -1;
    for (auto it = m_pendingRequests.begin(); it != m_pendingRequests.end(); ++it) {
        if (it->reply == reply) {
            requestId = it.key();
            break;
        }
    }

    if (requestId == -1) {
        reply->deleteLater();
        return;
    }

    PendingRequest &request = m_pendingRequests[requestId];

    // 停止超时定时器
    if (request.timeoutTimer) {
        request.timeoutTimer->stop();
    }

    bool success = false;
    QJsonObject responseData;
    QString errorMessage;

    try {
        if (reply->error() == QNetworkReply::NoError) {
            // 解析响应数据
            QByteArray responseBytes = reply->readAll();
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(responseBytes, &parseError);

            if (parseError.error != QJsonParseError::NoError) {
                throw QString("JSON解析错误: %1").arg(parseError.errorString());
            }

            responseData = doc.object();
            success = true;

            qDebug() << "请求成功:" << request.type;

        } else {
            // 处理网络错误
            NetworkError networkError = mapQNetworkError(reply->error());
            errorMessage = getErrorMessage(networkError, reply->errorString());

            qWarning() << "网络请求失败:" << request.type << "-" << errorMessage;

            // 检查是否需要重试
            if (shouldRetry(networkError) && request.currentRetry < request.config.maxRetries) {
                request.currentRetry++;
                qDebug() << "重试请求:" << request.type << "(" << request.currentRetry << "/"
                         << request.config.maxRetries << ")";

                // 延迟重试
                QTimer::singleShot(1000 * request.currentRetry, [this, requestId]() {
                    if (m_pendingRequests.contains(requestId)) {
                        executeRequest(m_pendingRequests[requestId]);
                    }
                });

                reply->deleteLater();
                return;
            }
        }
    } catch (const QString &error) {
        errorMessage = error;
        qWarning() << "处理响应时发生错误:" << error;
    } catch (...) {
        errorMessage = "处理响应时发生未知错误";
        qWarning() << "处理响应时发生未知错误";
    }

    // 完成请求
    completeRequest(requestId, success, responseData, errorMessage);
    reply->deleteLater();
}

void NetworkRequest::onRequestTimeout() {
    QTimer *timer = qobject_cast<QTimer *>(sender());
    if (!timer) {
        return;
    }

    // 查找对应的请求
    qint64 requestId = -1;
    for (auto it = m_pendingRequests.begin(); it != m_pendingRequests.end(); ++it) {
        if (it->timeoutTimer == timer) {
            requestId = it.key();
            break;
        }
    }

    if (requestId == -1) {
        return;
    }

    PendingRequest &request = m_pendingRequests[requestId];

    qWarning() << "请求超时:" << request.type;

    // 取消网络请求
    if (request.reply) {
        request.reply->abort();
    }

    // 检查是否需要重试
    if (request.currentRetry < request.config.maxRetries) {
        request.currentRetry++;
        qDebug() << "超时重试:" << request.type << "(" << request.currentRetry << "/" << request.config.maxRetries
                 << ")";

        // 延迟重试
        QTimer::singleShot(2000 * request.currentRetry, [this, requestId]() {
            if (m_pendingRequests.contains(requestId)) {
                executeRequest(m_pendingRequests[requestId]);
            }
        });
        return;
    }

    // 完成请求（失败）
    completeRequest(requestId, false, QJsonObject(), "请求超时");
}

void NetworkRequest::onSslErrors(const QList<QSslError> &errors) {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) {
        return;
    }

    qWarning() << "SSL错误:";
    for (const QSslError &error : errors) {
        qWarning() << " -" << error.errorString();
    }

    // TODO: 处理SSL错误
    reply->ignoreSslErrors();
}

void NetworkRequest::executeRequest(PendingRequest &request) {
    QNetworkRequest networkRequest = createNetworkRequest(request.config);

    // 发送POST请求
    QByteArray requestData;
    if (!request.config.data.isEmpty()) {
        requestData = QJsonDocument(request.config.data).toJson(QJsonDocument::Compact);
    }

    qDebug() << "发送网络请求:" << request.type << "到" << networkRequest.url().toString();

    request.reply = m_networkRequest->post(networkRequest, requestData);

    // 连接信号
    connect(request.reply, &QNetworkReply::finished, this, &NetworkRequest::onReplyFinished);
    connect(request.reply, &QNetworkReply::sslErrors, this, &NetworkRequest::onSslErrors);

    // 启动超时定时器
    request.timeoutTimer->start();
}

void NetworkRequest::completeRequest(qint64 requestId, bool success, const QJsonObject &response,
                                     const QString &error) {
    if (!m_pendingRequests.contains(requestId)) {
        return;
    }

    PendingRequest request = m_pendingRequests[requestId];

    if (success) {
        emit requestCompleted(request.type, response);
    } else {
        NetworkError networkError = error.contains("超时")   ? TimeoutError
                                    : error.contains("连接") ? ConnectionError
                                    : error.contains("认证") ? AuthenticationError
                                    : error.contains("解析") ? ParseError
                                                             : UnknownError;
        emit requestFailed(request.type, networkError, error);
    }

    cleanupRequest(requestId);
}

void NetworkRequest::cleanupRequest(qint64 requestId) {
    if (!m_pendingRequests.contains(requestId)) {
        return;
    }

    PendingRequest &request = m_pendingRequests[requestId];

    // 清理定时器
    if (request.timeoutTimer) {
        request.timeoutTimer->deleteLater();
    }

    // 从活跃请求中移除
    removeActiveRequest(request.type);

    // 移除待处理请求
    m_pendingRequests.remove(requestId);
}

void NetworkRequest::cancelRequest(RequestType type) {
    if (!m_activeRequests.contains(type)) {
        return;
    }

    qint64 requestId = m_activeRequests[type];
    if (m_pendingRequests.contains(requestId)) {
        PendingRequest &request = m_pendingRequests[requestId];
        if (request.reply) {
            request.reply->abort();
        }
        cleanupRequest(requestId);
    }
}

void NetworkRequest::cancelAllRequests() {
    QList<qint64> requestIds = m_pendingRequests.keys();
    for (qint64 requestId : requestIds) {
        PendingRequest &request = m_pendingRequests[requestId];
        if (request.reply) {
            request.reply->abort();
        }
        cleanupRequest(requestId);
    }
}

bool NetworkRequest::isNetworkAvailable() const { return m_isOnline; }

void NetworkRequest::checkNetworkConnectivity() {
    // TODO: 检查网络连接状态
    bool online = true;  // 简化实现，实际应用中可以发送测试请求

    if (online != m_isOnline) {
        m_isOnline = online;
        emit networkStatusChanged(m_isOnline);
        qDebug() << "当前网络状态:" << (m_isOnline ? "在线" : "离线");
    }
}

QNetworkRequest NetworkRequest::createNetworkRequest(const RequestConfig &config) const {
    QNetworkRequest request(QUrl(config.url));

    // 设置默认头部
    setupDefaultHeaders(request);

    // 添加认证头部
    if (config.requiresAuth) {
        addAuthHeader(request);
    }

    // 添加自定义头部
    for (auto it = config.headers.begin(); it != config.headers.end(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    return request;
}

void NetworkRequest::setupDefaultHeaders(QNetworkRequest &request) const {
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    // TODO: 应用名 / 版本 (平台)
    request.setRawHeader("User-Agent", "MyTodoApp/1.0 (Qt)");
    request.setRawHeader("Accept", "application/json");
    // TODO: 干脆删掉还是伪造成一个浏览器来源? 一般来说，对于客户端/移动端是不用加的，到时候如果有网页版，要不要添加？到时候c++的后端会不会改？
    request.setRawHeader("Origin", "https://example.com");
}

void NetworkRequest::addAuthHeader(QNetworkRequest &request) const {
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_authToken).toUtf8());
    }
}

NetworkRequest::NetworkError NetworkRequest::mapQNetworkError(QNetworkReply::NetworkError error) const {
    switch (error) {
        case QNetworkReply::TimeoutError:
            return TimeoutError;
        case QNetworkReply::ConnectionRefusedError:
        case QNetworkReply::RemoteHostClosedError:
        case QNetworkReply::HostNotFoundError:
        case QNetworkReply::NetworkSessionFailedError:
            return ConnectionError;
        case QNetworkReply::AuthenticationRequiredError:
        case QNetworkReply::ProxyAuthenticationRequiredError:
            return AuthenticationError;
        case QNetworkReply::InternalServerError:
        case QNetworkReply::ServiceUnavailableError:
            return ServerError;
        default:
            return UnknownError;
    }
}

QString NetworkRequest::getErrorMessage(NetworkError error, const QString &details) const {
    QString baseMessage;
    switch (error) {
        case TimeoutError:
            baseMessage = "请求超时";
            break;
        case ConnectionError:
            baseMessage = "连接错误";
            break;
        case AuthenticationError:
            baseMessage = "认证失败";
            break;
        case ServerError:
            baseMessage = "服务器错误";
            break;
        case ParseError:
            baseMessage = "数据解析错误";
            break;
        default:
            baseMessage = "未知错误";
            break;
    }

    if (!details.isEmpty()) {
        return QString("%1: %2").arg(baseMessage, details);
    }
    return baseMessage;
}

bool NetworkRequest::shouldRetry(NetworkError error) const {
    // 只对特定类型的错误进行重试
    return error == TimeoutError || error == ConnectionError || error == ServerError;
}

bool NetworkRequest::isDuplicateRequest(RequestType type) const { return m_activeRequests.contains(type); }

void NetworkRequest::addActiveRequest(RequestType type, qint64 requestId) { m_activeRequests[type] = requestId; }

void NetworkRequest::removeActiveRequest(RequestType type) { m_activeRequests.remove(type); }
