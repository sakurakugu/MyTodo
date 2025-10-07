/**
 * @file network_request.cpp
 * @brief 网络请求类实现
 *
 * 该文件实现了NetworkRequest类，提供统一的HTTP网络请求管理功能，
 * 支持请求超时、错误处理、SSL验证、请求重试和并发控制。
 *
 * @author Sakurakugu
 * @date 2025-08-17 07:17:29(UTC+8) 周日
 * @change 2025-10-05 15:48:18(UTC+8) 周日
 */
#include "network_request.h"
#include "config.h"
#include "network_proxy.h"
#include "version.h"

#include <QCoreApplication>
#include <QHostInfo>
#include <QJsonParseError>
#include <QNetworkRequest>
#include <QSslError>
#include <QUrl>

/**
 * @brief 构造函数
 *
 * 创建NetworkRequest实例，初始化网络访问管理器和连接监控定时器。
 *
 * @param parent 父对象指针，用于Qt对象树管理
 */
NetworkRequest::NetworkRequest(QObject *parent)
    : QObject(parent),                                   //
      m_networkRequest(new QNetworkAccessManager(this)), //
      m_nextRequestId(1),                                //
      m_proxyManager(NetworkProxy::GetInstance()) {      //
    // 设置默认配置
    m_networkRequest->setProxy(QNetworkProxy::NoProxy);

    // 应用代理配置到网络管理器
    m_proxyManager.applyProxyToManager(m_networkRequest);

    // 获取计算机名称
    std::string computerName = QHostInfo::localHostName().toStdString();
    m_computerName = computerName.empty() ? "Unknown" : computerName;
}

/**
 * @brief 析构函数
 *
 * 清理所有待处理的请求，停止定时器，释放网络资源。
 */
NetworkRequest::~NetworkRequest() {
    cancelAllRequests();
}

/**
 * @brief 设置认证令牌
 * @param token 访问令牌字符串
 */
void NetworkRequest::setAuthToken(const QString &token) {
    m_authToken = token;
}

/**
 * @brief 清除认证令牌
 */
void NetworkRequest::clearAuthToken() {
    m_authToken.clear();
}

/**
 * @brief 检查是否有有效的认证信息
 * @return 如果有有效认证令牌返回true，否则返回false
 */
bool NetworkRequest::hasValidAuth() const {
    return !m_authToken.isEmpty();
}

/**
 * @brief 设置服务器地址与api版本
 * @param baseUrl 服务器基础URL
 * @param apiVersion API版本，默认为"v1"
 */
void NetworkRequest::setServerConfig(const QString &baseUrl, const QString &apiVersion) {
    // 更新基础 URL
    m_serverBaseUrl = baseUrl;
    if (!m_serverBaseUrl.endsWith('/')) {
        m_serverBaseUrl += '/';
    }

    // 仅当调用方提供了非空 apiVersion 时才覆盖；否则保持已有值，若仍为空则回退默认 v1
    if (!apiVersion.isEmpty()) {
        m_apiVersion = apiVersion;
    }
    if (m_apiVersion.isEmpty()) {
        m_apiVersion = "v1"; // 安全回退，防止出现缺少版本前缀导致 404
    }
}

/**
 * @brief 获取服务器地址
 * @return 服务器基础URL
 */
QString NetworkRequest::getServerBaseUrl() const {
    return m_serverBaseUrl;
}

/**
 * @brief 获取完整的API URL
 * @param endpoint API端点路径
 * @return 完整的API URL
 */
QString NetworkRequest::getApiUrl(const QString &endpoint) const {
    if (m_serverBaseUrl.isEmpty()) {
        return endpoint;
    }

    QString baseUrl = m_serverBaseUrl;
    if (!baseUrl.endsWith('/')) {
        baseUrl += '/';
    }

    // 添加API版本号路径
    if (!m_apiVersion.isEmpty()) {
        baseUrl += QString("api/%1/").arg(m_apiVersion);
    }

    QString cleanEndpoint = endpoint;
    if (cleanEndpoint.startsWith('/')) {
        cleanEndpoint = cleanEndpoint.mid(1);
    }

    return baseUrl + cleanEndpoint;
}

/**
 * @brief 发送网络请求
 * @param type 请求类型
 * @param config 请求配置
 * @param customHandler 自定义响应处理器（可选，仅适用于Other类型）
 */
void NetworkRequest::sendRequest(Network::RequestType type, const RequestConfig &config,
                                 const std::optional<CustomResponseHandler> &customHandler) {
    // 如果提供了自定义处理器，检查请求类型是否合法
    if (customHandler.has_value() && type < Network::RequestType::Other) {
        qWarning() << "自定义响应处理器仅适用于自定义请求类型（大于等于Network::RequestType::Other）";
        emit requestFailed(type, Network::Error::UnknownError, "自定义响应处理器仅适用于自定义请求类型");
        return;
    }

    // 检查是否为重复请求
    if (isDuplicateRequest(type)) {
        qDebug() << "忽略重复请求:" << type;
        return;
    }

    // 检查认证要求（对于自定义请求暂不处理认证）
    if (config.requiresAuth && !customHandler.has_value() && !hasValidAuth()) {
        emit requestFailed(type, Network::Error::AuthenticationError, "缺少有效的认证令牌");
        return;
    }

    // 创建待处理请求
    PendingRequest request;
    request.type = type;
    request.config = config;
    request.currentRetry = 0;
    request.requestId = m_nextRequestId++;
    // 设置自定义处理器（如果提供）
    if (customHandler.has_value())
        request.customHandler = customHandler.value();

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

            // 检查是否有自定义响应处理器（仅适用于Other类型）
            if (request.type >= Network::RequestType::Other && request.customHandler) {
                int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                responseData = request.customHandler(responseBytes, httpStatusCode);
                success = true;
#ifdef QT_DEBUG
                qInfo() << "自定义处理器处理请求成功:" << RequestTypeToString(request.type);
                qInfo() << "响应内容:" << responseData;
#else
                qDebug() << "自定义处理器处理请求成功:" << RequestTypeToString(request.type);
#endif
            } else {
                // 使用默认的JSON解析逻辑
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(responseBytes, &parseError);

                if (parseError.error != QJsonParseError::NoError) {
                    throw QString("JSON解析错误: %1").arg(parseError.errorString());
                }

                QJsonObject fullResponse = doc.object();

                bool serverSuccess = fullResponse["success"].toBool();
                if (serverSuccess) {
                    // 成功响应，提取data字段作为响应数据
                    responseData = fullResponse.contains("data") ? fullResponse["data"].toObject() : fullResponse;
                    success = true;
#ifdef QT_DEBUG
                    qInfo() << "请求成功:" << NetworkRequest::GetInstance().RequestTypeToString(request.type);
                    qInfo() << "响应内容:" << responseData;
#else
                    qDebug() << "请求成功:" << RequestTypeToString(request.type);
#endif
                } else {
                    // 服务器返回错误
                    QString serverMessage = fullResponse.contains("error") ? fullResponse["error"].toString()
                                                                           : fullResponse["message"].toString();
                    throw QString("服务器错误: %1").arg(serverMessage);
                }
            } // 结束默认JSON解析逻辑的else分支

        } else {
            // 处理网络错误（包括HTTP状态码非2xx的情况）
            int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            Network::Error networkError = mapQNetworkError(reply->error());

            // 读取响应体（最多预览前 256 字节，避免日志污染）
            QByteArray body = reply->readAll();
            QByteArray bodyPreview = body.left(256);
            QString bodyText = QString::fromUtf8(bodyPreview);

            // 预设默认错误信息（含 Qt 原生错误串）
            QString baseMsg = getErrorMessage(networkError); // 不立即拼接 reply->errorString()，留给后续覆盖
            QString qtDetail = reply->errorString();

            // 尝试解析服务端 JSON 以抽取业务错误文案
            QString serverCode;
            QString serverMessage; // 优先候选
            QJsonParseError jsonErr;
            QJsonDocument jdoc = QJsonDocument::fromJson(body, &jsonErr);
            if (jsonErr.error == QJsonParseError::NoError && jdoc.isObject()) {
                QJsonObject o = jdoc.object();
                // 结构: { success:false, message:"...", error:{ code:"...", message:"..." } }
                if (o.contains("error")) {
                    const QJsonValue v = o.value("error");
                    if (v.isObject()) {
                        QJsonObject eo = v.toObject();
                        serverCode = eo.value("code").toString();
                        if (serverMessage.isEmpty()) {
                            serverMessage = eo.value("message").toString();
                        }
                    } else if (v.isString()) {
                        // 若直接把 error 作为字符串错误码
                        serverCode = v.toString();
                    }
                }
                if (serverMessage.isEmpty() && o.value("message").isString()) {
                    serverMessage = o.value("message").toString();
                }
            }

            // 归一化 serverMessage：去除前后空白
            serverMessage = serverMessage.trimmed();

            // 组装对上层暴露的 errorMessage：
            // 规则：
            // 1. 若有 serverMessage，优先使用其作为核心业务文案
            // 2. 保留分类关键字前缀（如 "认证失败"），以维持现有上层关键字检测逻辑
            if (!serverMessage.isEmpty()) {
                errorMessage = getErrorMessage(networkError, serverMessage);
            } else {
                // 没有服务端文案则退回原逻辑：分类前缀 + Qt 细节
                errorMessage = baseMsg;
                if (!qtDetail.isEmpty()) {
                    errorMessage += ": " + qtDetail;
                }
            }

// 日志输出（含 HTTP 状态与预览体）
#ifdef QT_DEBUG
            if (!bodyText.isEmpty())
                qWarning() << "网络请求失败:" << RequestTypeToString(request.type) << "- HTTP状态码:" << httpStatus
                           << "-" << errorMessage << "-" << "错误码:" << serverCode << "响应体预览:" << bodyText;
            else
#endif
                qWarning() << "网络请求失败:" << RequestTypeToString(request.type) << "- HTTP状态码:" << httpStatus
                           << "-" << errorMessage << "-" << "错误码:" << serverCode;

            // 专门的 401 认证处理（沿用原逻辑，但复用已经解析的 JSON 信息）
            if (httpStatus == 401) {
                QString normCode = serverCode.toUpper();
                if (normCode == "UNAUTHORIZED") {
                    emit authTokenExpired();
                } else if (normCode == "LOGIN_FAILED") {
                } else {
                    emit authTokenExpired(); // 兜底，防止服务端未按规范返回 UNAUTHORIZED
                }
            } else if (httpStatus == 400) {
                // 未来可在此扩展针对 400 的细分处理
            }

            // 重试逻辑：仅在没有业务语义的“可恢复”技术性错误时重试
            if (shouldRetry(networkError) && request.currentRetry < request.config.maxRetries) {
                // 若服务器明确给出业务错误（如密码错误），不再重试
                if (serverMessage.isEmpty()) {
                    request.currentRetry++;
                    qDebug() << "重试请求:" << RequestTypeToString(request.type) << "(" << request.currentRetry << "/"
                             << request.config.maxRetries << ")";
                    QTimer::singleShot(1000 * request.currentRetry, [this, requestId]() {
                        if (m_pendingRequests.contains(requestId)) {
                            executeRequest(m_pendingRequests[requestId]);
                        }
                    });
                    reply->deleteLater();
                    return;
                }
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

    qWarning() << "请求超时:" << RequestTypeToString(request.type);

    // 取消网络请求
    if (request.reply) {
        request.reply->abort();
    }

    // 检查是否需要重试
    if (request.currentRetry < request.config.maxRetries) {
        request.currentRetry++;
        qDebug() << "超时重试:" << RequestTypeToString(request.type) << "(" << request.currentRetry << "/"
                 << request.config.maxRetries << ")";

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

    // 忽略所有SSL错误
    reply->ignoreSslErrors();
}

void NetworkRequest::executeRequest(PendingRequest &request) {
    QNetworkRequest networkRequest = createNetworkRequest(request.config);

    qDebug() << "发送网络请求:" << RequestTypeToString(request.type) << "到" << networkRequest.url().toString();

    // 准备请求数据
    QByteArray requestData;
    if (!request.config.data.isEmpty()) {
        requestData = QJsonDocument(request.config.data).toJson(QJsonDocument::Compact);
    }

    // 根据HTTP方法发送请求
    QString method = request.config.method.toUpper();
    if (method == "GET") {
        request.reply = m_networkRequest->get(networkRequest);
    } else if (method == "POST") {
        request.reply = m_networkRequest->post(networkRequest, requestData);
    } else if (method == "PUT") {
        request.reply = m_networkRequest->put(networkRequest, requestData);
    } else if (method == "PATCH") {
        // 使用sendCustomRequest发送PATCH请求
        qInfo() << "发送PATCH请求到服务器:" << networkRequest.url().toString();
        qInfo() << "PATCH请求数据:" << QString::fromUtf8(requestData);
        request.reply = m_networkRequest->sendCustomRequest(networkRequest, "PATCH", requestData);
    } else if (method == "DELETE") {
        request.reply = m_networkRequest->deleteResource(networkRequest);
    } else {
        // 默认使用POST（向后兼容）
        qWarning() << "不支持的HTTP方法:" << method << ", 使用POST代替";
        request.reply = m_networkRequest->post(networkRequest, requestData);
    }

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
        Network::Error networkError = //
            error.contains("超时")   ? Network::Error::TimeoutError
            : error.contains("连接") ? Network::Error::ConnectionError
            : error.contains("认证") ? Network::Error::AuthenticationError
            : error.contains("解析") ? Network::Error::ParseError
                                     : Network::Error::UnknownError;
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

void NetworkRequest::cancelRequest(Network::RequestType type) {
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
    // 应用名 / 版本 (平台@计算机名)
    const QByteArray userAgent = std::format("{} v{} (Qt@{})", APP_NAME, APP_VERSION_STRING, m_computerName).c_str();
    // 设置Origin头部，格式为 app://应用名(平台)
#if defined(Q_OS_WIN)
    const QByteArray origin = std::format("app://{}({})", APP_NAME, "Windows").c_str();
#elif defined(Q_OS_MACOS)
    const QByteArray origin = std::format("app://{}({})", APP_NAME, "macOS").c_str();
#else
    const QByteArray origin = std::format("app://{}({})", APP_NAME, "Linux").c_str();
#endif

    request.setRawHeader("User-Agent", userAgent);
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("Origin", origin);
}

void NetworkRequest::addAuthHeader(QNetworkRequest &request) const {
    if (!m_authToken.isEmpty()) {
        QString authHeader = QString("Bearer %1").arg(m_authToken);
        request.setRawHeader("Authorization", authHeader.toUtf8());
        // qDebug() << "添加认证头部:" << authHeader.left(20) + "..."; // 只显示前20个字符
    } else {
        qWarning() << "认证令牌为空，无法添加认证头部";
    }
}

Network::Error NetworkRequest::mapQNetworkError(QNetworkReply::NetworkError error) const {
    switch (error) {
    case QNetworkReply::TimeoutError:
        return Network::Error::TimeoutError;
    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::RemoteHostClosedError:
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::NetworkSessionFailedError:
        return Network::Error::ConnectionError;
    case QNetworkReply::AuthenticationRequiredError:
    case QNetworkReply::ProxyAuthenticationRequiredError:
        return Network::Error::AuthenticationError;
    case QNetworkReply::InternalServerError:
    case QNetworkReply::ServiceUnavailableError:
        return Network::Error::ServerError;
    default:
        return Network::Error::UnknownError;
    }
}

QString NetworkRequest::getErrorMessage(Network::Error error, const QString &details) const {
    QString baseMessage;
    switch (error) {
    case Network::Error::TimeoutError:
        baseMessage = "请求超时";
        break;
    case Network::Error::ConnectionError:
        baseMessage = "连接错误";
        break;
    case Network::Error::AuthenticationError:
        baseMessage = "认证失败";
        break;
    case Network::Error::ServerError:
        baseMessage = "服务器错误";
        break;
    case Network::Error::ParseError:
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

QString NetworkRequest::RequestTypeToString(Network::RequestType type) const {
    auto it = Network::RequestTypeNameMap.find(type);
    if (it != Network::RequestTypeNameMap.end()) {
        return it->second;
    }
    return "未知请求";
}

bool NetworkRequest::shouldRetry(Network::Error error) const {
    // 只对特定类型的错误进行重试
    return error == Network::Error::TimeoutError || error == Network::Error::ConnectionError ||
           error == Network::Error::ServerError;
}

bool NetworkRequest::isDuplicateRequest(Network::RequestType type) const {
    return m_activeRequests.contains(type);
}

void NetworkRequest::addActiveRequest(Network::RequestType type, qint64 requestId) {
    m_activeRequests[type] = requestId;
}

void NetworkRequest::removeActiveRequest(Network::RequestType type) {
    m_activeRequests.remove(type);
}
