#include "todoModel.h"
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QNetworkProxy>
#include <QProcess>

TodoModel::TodoModel(QObject *parent, Settings *settings, Settings::StorageType storageType)
    : QAbstractListModel(parent), m_isOnline(false), m_currentCategory(""), m_currentFilter(""),
      m_networkManager(new QNetworkAccessManager(this)), m_settings(settings ? settings : new Settings(this, storageType)),
      m_networkTimeoutTimer(new QTimer(this)), m_currentRetryCount(0) {

    // 初始化默认服务器配置
    m_settings->initializeDefaultServerConfig();
    
    // 初始化服务器配置
    initializeServerConfig();

    // 配置网络超时定时器
    m_networkTimeoutTimer->setSingleShot(true);
    connect(m_networkTimeoutTimer, &QTimer::timeout, this, &TodoModel::handleNetworkTimeout);

    // 加载本地数据
    if (!loadFromLocalStorage()) {
        qWarning() << "无法从本地存储加载待办事项数据";
    }

    // 从设置初始化在线状态（与原autoSync合并）
    m_isOnline = m_settings->get("autoSync", false).toBool();
    emit isOnlineChanged();

    // 尝试使用存储的令牌自动登录
    if (m_settings->contains("accessToken")) {
        m_accessToken = m_settings->get("accessToken").toString();
        m_refreshToken = m_settings->get("refreshToken").toString();
        m_username = m_settings->get("username").toString();

        // TODO: 在这里验证令牌是否有效
        qDebug() << "使用存储的凭据自动登录用户：" << m_username;
    }
}

TodoModel::~TodoModel() {
    // 保存数据
    saveToLocalStorage();

    // 清理资源
    qDeleteAll(m_todos);
    m_todos.clear();
}

/**
 * @brief 获取模型中的行数（待办事项数量）
 * @param parent 父索引，默认为无效索引（根）
 * @return 待办事项数量
 */
int TodoModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;

    // 如果没有设置过滤条件，返回所有项目
    if (m_currentCategory.isEmpty() && m_currentFilter.isEmpty()) {
        return m_todos.count();
    }

    // 计算符合过滤条件的项目数量
    int count = 0;
    for (const TodoItem* item : m_todos) {
        bool categoryMatch = m_currentCategory.isEmpty() || item->category() == m_currentCategory;
        bool statusMatch = m_currentFilter.isEmpty() || 
                         (m_currentFilter == "done" && item->status() == "done") ||
                         (m_currentFilter == "todo" && item->status() == "todo");
        
        if (categoryMatch && statusMatch) {
            count++;
        }
    }
    
    return count;
}

/**
 * @brief 获取指定索引和角色的数据
 * @param index 待获取数据的模型索引
 * @param role 数据角色
 * @return 请求的数据值
 */
QVariant TodoModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return QVariant();

    // 如果没有设置过滤条件，直接返回对应索引的项目
    if (m_currentCategory.isEmpty() && m_currentFilter.isEmpty()) {
        if (index.row() >= m_todos.size())
            return QVariant();
            
        const TodoItem *item = m_todos.at(index.row());
        return getItemData(item, role);
    }

    // 找到符合过滤条件的第index.row()个项目
    int filteredIndex = -1;
    for (int i = 0; i < m_todos.size(); i++) {
        const TodoItem* item = m_todos.at(i);
        bool categoryMatch = m_currentCategory.isEmpty() || item->category() == m_currentCategory;
        bool statusMatch = m_currentFilter.isEmpty() || 
                         (m_currentFilter == "done" && item->status() == "done") ||
                         (m_currentFilter == "todo" && item->status() == "todo");
        
        if (categoryMatch && statusMatch) {
            filteredIndex++;
            if (filteredIndex == index.row()) {
                return getItemData(item, role);
            }
        }
    }

    return QVariant();
}

// 辅助方法，根据角色返回项目数据
QVariant TodoModel::getItemData(const TodoItem* item, int role) const {
    switch (role) {
    case IdRole:
        return item->id();
    case TitleRole:
        return item->title();
    case DescriptionRole:
        return item->description();
    case CategoryRole:
        return item->category();
    case UrgencyRole:
        return item->urgency();
    case ImportanceRole:
        return item->importance();
    case StatusRole:
        return item->status();
    case CreatedAtRole:
        return item->createdAt();
    case UpdatedAtRole:
        return item->updatedAt();
    case SyncedRole:
        return item->synced();
    default:
        return QVariant();
    }
}

/**
 * @brief 获取角色名称映射，用于QML访问
 * @return 角色ID到角色名称的映射
 */
QHash<int, QByteArray> TodoModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[TitleRole] = "title";
    roles[DescriptionRole] = "description";
    roles[CategoryRole] = "category";
    roles[UrgencyRole] = "urgency";
    roles[ImportanceRole] = "importance";
    roles[StatusRole] = "status";
    roles[CreatedAtRole] = "createdAt";
    roles[UpdatedAtRole] = "updatedAt";
    roles[SyncedRole] = "synced";
    return roles;
}

/**
 * @brief 设置指定索引和角色的数据
 * @param index 待设置数据的模型索引
 * @param value 新的数据值
 * @param role 数据角色
 * @return 设置是否成功
 */
bool TodoModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (!index.isValid() || index.row() >= m_todos.size())
        return false;

    TodoItem *item = m_todos.at(index.row());
    bool changed = false;

    switch (role) {
    case TitleRole:
        item->setTitle(value.toString());
        changed = true;
        break;
    case DescriptionRole:
        item->setDescription(value.toString());
        changed = true;
        break;
    case CategoryRole:
        item->setCategory(value.toString());
        changed = true;
        break;
    case UrgencyRole:
        item->setUrgency(value.toString());
        changed = true;
        break;
    case ImportanceRole:
        item->setImportance(value.toString());
        changed = true;
        break;
    case StatusRole:
        item->setStatus(value.toString());
        changed = true;
        break;
    }

    if (changed) {
        item->setUpdatedAt(QDateTime::currentDateTime());
        item->setSynced(false);
        emit dataChanged(index, index, QVector<int>() << role);
        saveToLocalStorage();
        return true;
    }

    return false;
}

/**
 * @brief 获取当前在线状态
 * @return 是否在线
 */
bool TodoModel::isOnline() const {
    return m_isOnline;
}

/**
 * @brief 设置在线状态
 * @param online 新的在线状态
 */
void TodoModel::setIsOnline(bool online) {
    // 如果已经是目标状态，则不做任何操作
    if (m_isOnline == online) {
        return;
    }
    
    if (online) {
        // 尝试连接服务器，验证是否可以切换到在线模式
        QNetworkRequest request(QUrl(getApiUrl(m_todoApiEndpoint)));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        
        if (isLoggedIn()) {
            request.setRawHeader("Authorization", QString("Bearer %1").arg(m_accessToken).toUtf8());
        }
        
        QNetworkReply *reply = m_networkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, online]() {
            reply->deleteLater();
            
            if (reply->error() == QNetworkReply::NoError) {
                // 连接成功，更新状态
                m_isOnline = online;
                emit isOnlineChanged();
                // 保存到设置，保持与autoSync一致
                m_settings->save("autoSync", m_isOnline);
                
                if (m_isOnline && isLoggedIn()) {
                    syncWithServer();
                }
            } else {
                // 连接失败，记录错误并通知用户
                QString errorMessage = reply->errorString();
                logError("切换在线模式", errorMessage);
                emit syncCompleted(false, QString("切换在线模式失败: %1").arg(errorMessage));
            }
        });
    } else {
        // 切换到离线模式不需要验证，直接更新状态
        m_isOnline = online;
        emit isOnlineChanged();
        // 保存到设置，保持与autoSync一致
        m_settings->save("autoSync", m_isOnline);
    }
}

/**
 * @brief 获取当前激活的分类筛选器
 * @return 当前分类名称
 */
QString TodoModel::currentCategory() const {
    return m_currentCategory;
}

/**
 * @brief 设置分类筛选器
 * @param category 分类名称，空字符串表示显示所有分类
 */
void TodoModel::setCurrentCategory(const QString &category) {
    if (m_currentCategory != category) {
        m_currentCategory = category;
        emit currentCategoryChanged();

        // 应用过滤逻辑
        beginResetModel();
        // 过滤逻辑在data方法中实现
        endResetModel();
    }
}

/**
 * @brief 获取当前激活的筛选条件
 * @return 当前筛选条件
 */
QString TodoModel::currentFilter() const {
    return m_currentFilter;
}

/**
 * @brief 设置筛选条件
 * @param filter 筛选条件（如"完成"、"未完成"等）
 */
void TodoModel::setCurrentFilter(const QString &filter) {
    if (m_currentFilter != filter) {
        m_currentFilter = filter;
        emit currentFilterChanged();

        // 应用过滤逻辑
        beginResetModel();
        // 过滤逻辑在data方法中实现
        endResetModel();
    }
}

/**
 * @brief 添加新的待办事项
 * @param title 任务标题（必填）
 * @param description 任务描述（可选）
 * @param category 任务分类（默认为"default"）
 * @param urgency 紧急程度（默认为"medium"）
 * @param importance 重要程度（默认为"medium"）
 */
void TodoModel::addTodo(const QString &title, const QString &description, const QString &category,
                        const QString &urgency, const QString &importance) {
    beginInsertRows(QModelIndex(), rowCount(), rowCount());

    TodoItem *newItem =
        new TodoItem(QUuid::createUuid().toString(QUuid::WithoutBraces), title, description, category, urgency,
                     importance, "todo", QDateTime::currentDateTime(), QDateTime::currentDateTime(), false, this);

    m_todos.append(newItem);

    endInsertRows();

    saveToLocalStorage();

    if (m_isOnline && isLoggedIn()) {
        // 如果在线且已登录，立即尝试同步到服务器
        syncWithServer();
    }
}

/**
 * @brief 更新现有待办事项
 * @param index 待更新事项的索引
 * @param todoData 包含要更新字段的映射
 * @return 更新是否成功
 */
bool TodoModel::updateTodo(int index, const QVariantMap &todoData) {
    // 检查索引是否有效
    if (index < 0 || index >= m_todos.size()) {
        qWarning() << "尝试更新无效的索引:" << index;
        return false;
    }

    QModelIndex modelIndex = createIndex(index, 0);
    bool anyUpdated = false;

    try {
        // 更新各个字段
        if (todoData.contains("title")) {
            anyUpdated |= setData(modelIndex, todoData["title"], TitleRole);
        }

        if (todoData.contains("description")) {
            anyUpdated |= setData(modelIndex, todoData["description"], DescriptionRole);
        }

        if (todoData.contains("category")) {
            anyUpdated |= setData(modelIndex, todoData["category"], CategoryRole);
        }

        if (todoData.contains("urgency")) {
            anyUpdated |= setData(modelIndex, todoData["urgency"], UrgencyRole);
        }

        if (todoData.contains("importance")) {
            anyUpdated |= setData(modelIndex, todoData["importance"], ImportanceRole);
        }

        if (todoData.contains("status")) {
            anyUpdated |= setData(modelIndex, todoData["status"], StatusRole);
        }

        // 如果有任何更新，则保存并同步
        if (anyUpdated) {
            if (!saveToLocalStorage()) {
                qWarning() << "更新待办事项后无法保存到本地存储";
            }

            if (m_isOnline && isLoggedIn()) {
                syncWithServer();
            }

            qDebug() << "成功更新索引" << index << "处的待办事项";
            return true;
        } else {
            qDebug() << "没有字段被更新，索引:" << index;
            return false;
        }
    } catch (const std::exception &e) {
        qCritical() << "更新待办事项时发生异常:" << e.what();
        logError("更新待办事项", QString("异常: %1").arg(e.what()));
        return false;
    } catch (...) {
        qCritical() << "更新待办事项时发生未知异常";
        logError("更新待办事项", "未知异常");
        return false;
    }
}

/**
 * @brief 删除待办事项
 * @param index 待删除事项的索引
 * @return 删除是否成功
 */
bool TodoModel::removeTodo(int index) {
    // 检查索引是否有效
    if (index < 0 || index >= m_todos.size()) {
        qWarning() << "尝试删除无效的索引:" << index;
        return false;
    }

    try {
        beginRemoveRows(QModelIndex(), index, index);

        TodoItem *item = m_todos.takeAt(index);
        delete item;

        endRemoveRows();

        if (!saveToLocalStorage()) {
            qWarning() << "删除待办事项后无法保存到本地存储";
        }

        if (m_isOnline && isLoggedIn()) {
            syncWithServer();
        }

        qDebug() << "成功删除索引" << index << "处的待办事项";
        return true;
    } catch (const std::exception &e) {
        qCritical() << "删除待办事项时发生异常:" << e.what();
        logError("删除待办事项", QString("异常: %1").arg(e.what()));
        return false;
    } catch (...) {
        qCritical() << "删除待办事项时发生未知异常";
        logError("删除待办事项", "未知异常");
        return false;
    }
}

/**
 * @brief 将待办事项标记为已完成
 * @param index 待办事项的索引
 * @return 操作是否成功
 */
bool TodoModel::markAsDone(int index) {
    // 检查索引是否有效
    if (index < 0 || index >= m_todos.size()) {
        qWarning() << "尝试标记无效索引的待办事项为已完成:" << index;
        return false;
    }

    try {
        QModelIndex modelIndex = createIndex(index, 0);
        bool success = setData(modelIndex, "done", StatusRole);

        if (success) {
            if (m_isOnline && isLoggedIn()) {
                syncWithServer();
            }
            qDebug() << "成功将索引" << index << "处的待办事项标记为已完成";
        } else {
            qWarning() << "无法将索引" << index << "处的待办事项标记为已完成";
        }

        return success;
    } catch (const std::exception &e) {
        qCritical() << "标记待办事项为已完成时发生异常:" << e.what();
        logError("标记为已完成", QString("异常: %1").arg(e.what()));
        return false;
    } catch (...) {
        qCritical() << "标记待办事项为已完成时发生未知异常";
        logError("标记为已完成", "未知异常");
        return false;
    }
}

/**
 * @brief 与服务器同步待办事项数据
 *
 * 该方法会先获取服务器上的最新数据，然后将本地更改推送到服务器。
 * 操作结果通过syncCompleted信号通知。
 */
void TodoModel::syncWithServer() {
    // 检查是否可以进行同步
    if (!m_isOnline) {
        qDebug() << "无法同步：离线模式";
        return;
    }

    if (!isLoggedIn()) {
        qDebug() << "无法同步：未登录";
        return;
    }

    qDebug() << "开始同步待办事项...";
    emit syncStarted();

    try {
        // 重置重试计数
        m_currentRetryCount = 0;

        // 先获取服务器上的最新数据
        fetchTodosFromServer();

        // 然后推送本地更改到服务器
        // 注意：这部分会在fetchTodosFromServer的回调中完成
        // 因为我们需要先获取最新数据，解决冲突后再推送
    } catch (const std::exception &e) {
        qCritical() << "同步过程中发生异常:" << e.what();
        logError("同步", QString("异常: %1").arg(e.what()));
        emit syncCompleted(false, QString("同步失败: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "同步过程中发生未知异常";
        logError("同步", "未知异常");
        emit syncCompleted(false, "同步失败：未知错误");
    }
}

/**
 * @brief 使用用户凭据登录服务器
 * @param username 用户名
 * @param password 密码
 *
 * 登录结果会通过loginSuccessful或loginFailed信号通知。
 */
void TodoModel::login(const QString &username, const QString &password) {
    if (username.isEmpty() || password.isEmpty()) {
        qWarning() << "尝试使用空的用户名或密码登录";
        emit loginFailed("用户名和密码不能为空");
        return;
    }

    qDebug() << "尝试登录用户:" << username;

    try {
        // 配置网络代理（如果需要）
        // 注意：如果您的环境需要代理，请取消注释并配置下面的代码
        /*
        QNetworkProxy proxy;
        proxy.setType(QNetworkProxy::HttpProxy);
        proxy.setHostName("your.proxy.server"); // 替换为您的代理服务器地址
        proxy.setPort(8080);                    // 替换为您的代理服务器端口

        // 如果代理需要身份验证
        proxy.setUser("proxyuser");             // 替换为您的代理用户名
        proxy.setPassword("proxypass");         // 替换为您的代理密码

        m_networkManager->setProxy(proxy);
        */

        // 或者禁用代理（尝试直接连接）
        m_networkManager->setProxy(QNetworkProxy::NoProxy);

        // 确保 URL 正确
        QUrl url(getApiUrl(m_authApiEndpoint) + "?action=login");

        QNetworkRequest request(url);

        // 设置请求头
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        // 添加 Origin 头部来匹配服务器的 CORS 配置
        request.setRawHeader("Origin", "https://example.com");
        // 添加 User-Agent 头部以帮助服务器识别客户端
        request.setRawHeader("User-Agent", "MyTodoApp/1.0 (Qt)");

        // 打印完整的请求信息用于调试
        qDebug() << "请求URL:" << url.toString();
        qDebug() << "请求方法: POST";
        qDebug() << "内容类型:" << request.header(QNetworkRequest::ContentTypeHeader).toString();

        // 创建 JSON 格式的登录数据
        QJsonObject json;
        json["username"] = username;
        json["password"] = password;
        QByteArray requestData = QJsonDocument(json).toJson();

        // 打印请求体（不包含密码）
        QJsonObject logJson = json;
        logJson["password"] = "********";
        qDebug() << "请求体:" << QJsonDocument(logJson).toJson();

        // 重置重试计数
        m_currentRetryCount = 0;

        // 启动超时计时器
        m_networkTimeoutTimer->start(NETWORK_TIMEOUT_MS);

        // 发送请求
        QNetworkReply *reply = m_networkManager->post(request, requestData);

        // 连接额外的信号以获取更多错误信息
        connect(reply, &QNetworkReply::sslErrors, this, [](const QList<QSslError> &errors) {
            qWarning() << "SSL 错误:";
            for (const QSslError &error : errors) {
                qWarning() << " - " << error.errorString();
            }
        });

        connect(reply, &QNetworkReply::errorOccurred, [reply](QNetworkReply::NetworkError code) {
            qWarning() << "网络错误: " << code << " - " << reply->errorString();
        });

        // 连接完成信号
        connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleLoginResponse(reply); });

        emit syncStarted(); // 表示正在进行网络操作

    } catch (const std::exception &e) {
        m_networkTimeoutTimer->stop();
        qCritical() << "发起登录请求时发生异常:" << e.what();
        logError("登录请求", QString("异常: %1").arg(e.what()));
        emit loginFailed(QString("登录请求失败: %1").arg(e.what()));
    } catch (...) {
        m_networkTimeoutTimer->stop();
        qCritical() << "发起登录请求时发生未知异常";
        logError("登录请求", "未知异常");
        emit loginFailed("登录请求失败：未知错误");
    }
}

/**
 * @brief 注销当前用户
 *
 * 清除存储的凭据并将所有项标记为未同步。
 */
void TodoModel::logout() {
    m_accessToken.clear();
    m_refreshToken.clear();
    m_username.clear();

    m_settings->remove("accessToken");
    m_settings->remove("refreshToken");
    m_settings->remove("username");

    // 标记所有项为未同步
    for (int i = 0; i < m_todos.size(); ++i) {
        m_todos[i]->setSynced(false);
    }
    
    // 发出退出登录成功信号
    emit logoutSuccessful();
}

/**
 * @brief 检查用户是否已登录
 * @return 是否已登录
 */
bool TodoModel::isLoggedIn() const {
    return !m_accessToken.isEmpty();
}

/**
 * @brief 获取用户名
 * @return 用户名
 */
QString TodoModel::getUsername() const {
    return m_username;
}

/**
 * @brief 获取邮箱
 * @return 邮箱
 */
QString TodoModel::getEmail() const {
    return m_email;
}


void TodoModel::onNetworkReplyFinished(QNetworkReply *reply) {
    // 停止超时计时器
    m_networkTimeoutTimer->stop();

    // 检查网络错误
    if (reply->error() != QNetworkReply::NoError) {
        logError("网络请求", reply->errorString());
    }

    reply->deleteLater();
}

void TodoModel::handleNetworkTimeout() {
    qWarning() << "网络请求超时";

    // 根据当前重试计数决定是否重试
    if (m_currentRetryCount < MAX_RETRIES) {
        m_currentRetryCount++;
        qDebug() << "尝试重新连接... 尝试" << m_currentRetryCount << "/" << MAX_RETRIES;

        // 这里可以重新发起相应的请求
        // 注意：需要记录当前正在进行的请求类型
    } else {
        m_currentRetryCount = 0;
        emit syncCompleted(false, "网络请求超时，请检查网络连接");
    }
}

void TodoModel::handleLoginResponse(QNetworkReply *reply) {
    // 停止超时计时器
    m_networkTimeoutTimer->stop();
    m_currentRetryCount = 0;

    // 防止内存泄漏
    reply->deleteLater();

    try {
        // 打印响应信息用于调试
        qDebug() << "收到响应:";
        qDebug() << " - URL:" << reply->url().toString();
        qDebug() << " - 状态码:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << " - 内容类型:" << reply->header(QNetworkRequest::ContentTypeHeader).toString();

        // 先读取所有响应数据
        QByteArray responseData = reply->readAll();

        // 检查错误
        if (reply->error() == QNetworkReply::NoError) {
            // 解析响应
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);

            if (parseError.error != QJsonParseError::NoError) {
                qWarning() << "JSON 解析错误:" << parseError.errorString();
                qWarning() << "原始响应:" << responseData;
                throw QString("JSON解析错误: %1").arg(parseError.errorString());
            }

            QJsonObject obj = doc.object();

            if (obj["success"].toBool()) {
                // 验证响应中包含必要的字段
                if (!obj.contains("access_token") || !obj.contains("refresh_token") || !obj.contains("user")) {
                    throw QString("服务器响应缺少必要字段");
                }

                m_accessToken = obj["access_token"].toString();
                m_refreshToken = obj["refresh_token"].toString();
                m_username = obj["user"].toObject()["username"].toString();

                // 保存令牌
                m_settings->save("accessToken", m_accessToken);
                m_settings->save("refreshToken", m_refreshToken);
                m_settings->save("username", m_username);

                qDebug() << "用户" << m_username << "登录成功";
                emit loginSuccessful(m_username);

                // 登录成功后自动同步
                if (m_isOnline) {
                    syncWithServer();
                }
            } else {
                QString errorMessage = obj.contains("message") ? obj["message"].toString() : "未知登录错误";
                qWarning() << "登录失败:" << errorMessage;
                emit loginFailed(errorMessage);
                logError("登录", errorMessage);
            }
        } else {
            QString errorMessage = reply->errorString();
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

            // 尝试解析响应内容，可能包含更多错误信息
            QString additionalInfo;
            if (!responseData.isEmpty()) {
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
                if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                    QJsonObject obj = doc.object();
                    if (obj.contains("error")) {
                        additionalInfo = obj["error"].toString();
                    }
                } else {
                    // 如果不是有效的JSON，则直接显示原始响应
                    additionalInfo = QString::fromUtf8(responseData);
                }
            }

            // 构建详细的错误信息
            QString detailedError = QString("HTTP 状态码: %1, 错误: %2").arg(statusCode).arg(errorMessage);
            if (!additionalInfo.isEmpty()) {
                detailedError += QString(", 详情: %1").arg(additionalInfo);
            }

            qWarning() << "网络请求失败:" << detailedError;

            // 针对特定错误提供更友好的信息
            if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
                emit loginFailed("服务器需要身份验证，请检查网络设置或联系系统管理员");
                logError("登录网络错误", "服务器需要身份验证 - " + detailedError);
            } else if (reply->error() == QNetworkReply::ProxyAuthenticationRequiredError) {
                emit loginFailed("代理服务器需要身份验证，请检查代理设置");
                logError("登录网络错误", "代理身份验证错误 - " + detailedError);
            } else if (reply->error() == QNetworkReply::ConnectionRefusedError) {
                emit loginFailed("连接被拒绝，请检查服务器地址和端口是否正确");
                logError("登录网络错误", "连接被拒绝 - " + detailedError);
            } else if (reply->error() == QNetworkReply::SslHandshakeFailedError) {
                emit loginFailed("SSL握手失败，请检查您的网络安全设置");
                logError("登录网络错误", "SSL错误 - " + detailedError);
            } else {
                emit loginFailed(errorMessage);
                logError("登录网络错误", detailedError);
            }
        }
    } catch (const QString &error) {
        emit loginFailed(error);
        logError("登录异常", error);
    } catch (...) {
        emit loginFailed("处理登录响应时发生未知错误");
        logError("登录异常", "未知异常");
    }
}

void TodoModel::handleSyncResponse(QNetworkReply *reply) {
    // 停止超时计时器
    m_networkTimeoutTimer->stop();
    m_currentRetryCount = 0;

    // 防止内存泄漏
    reply->deleteLater();

    bool success = false;
    QString errorMessage;

    try {
        if (reply->error() == QNetworkReply::NoError) {
            // 解析响应
            QByteArray responseData = reply->readAll();
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);

            if (parseError.error != QJsonParseError::NoError) {
                throw QString("JSON解析错误: %1").arg(parseError.errorString());
            }

            // 处理同步响应，更新本地数据
            // TODO: 实现实际的同步逻辑
            qDebug() << "成功获取服务器数据，处理同步逻辑";

            success = true;
        } else {
            errorMessage = reply->errorString();
            logError("同步网络错误", errorMessage);
        }
    } catch (const QString &error) {
        errorMessage = error;
        logError("同步异常", error);
    } catch (...) {
        errorMessage = "处理同步响应时发生未知错误";
        logError("同步异常", "未知异常");
    }

    emit syncCompleted(success, errorMessage);
}

/**
 * @brief 从本地存储加载待办事项
 * @return 加载是否成功
 */
bool TodoModel::loadFromLocalStorage() {
    beginResetModel();
    bool success = true;

    try {
        // 清除当前数据
        qDeleteAll(m_todos);
        m_todos.clear();

        // 检查设置是否可访问
        if (!m_settings) {
            qWarning() << "设置对象不可用";
            return false;
        }

        // 从设置中加载数据
        int count = m_settings->get("todos/size", 0).toInt();
        qDebug() << "从本地存储加载" << count << "个待办事项";

        for (int i = 0; i < count; ++i) {
            QString prefix = QString("todos/%1/").arg(i);

            // 验证必要字段
            if (!m_settings->contains(prefix + "id") || !m_settings->contains(prefix + "title")) {
                qWarning() << "跳过无效的待办事项记录（索引" << i << "）：缺少必要字段";
                continue;
            }

            TodoItem *item = new TodoItem(
                m_settings->get(prefix + "id").toString(),
                m_settings->get(prefix + "title").toString(),
                m_settings->get(prefix + "description").toString(),
                m_settings->get(prefix + "category").toString(),
                m_settings->get(prefix + "urgency").toString(),
                m_settings->get(prefix + "importance").toString(),
                m_settings->get(prefix + "status").toString(),
                m_settings->get(prefix + "createdAt").toDateTime(),
                m_settings->get(prefix + "updatedAt").toDateTime(),
                m_settings->get(prefix + "synced").toBool(),
                this);

            m_todos.append(item);
        }
    } catch (const std::exception &e) {
        qCritical() << "加载本地存储时发生异常:" << e.what();
        success = false;
        logError("加载本地存储", QString("异常: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "加载本地存储时发生未知异常";
        success = false;
        logError("加载本地存储", "未知异常");
    }

    endResetModel();
    return success;
}

/**
 * @brief 将待办事项保存到本地存储
 * @return 保存是否成功
 */
bool TodoModel::saveToLocalStorage() {
    bool success = true;

    try {
        // 检查设置对象是否可用
        if (!m_settings) {
            qWarning() << "设置对象不可用";
            return false;
        }

        // 保存待办事项数量
        m_settings->save("todos/size", m_todos.size());

        // 保存每个待办事项
        for (int i = 0; i < m_todos.size(); ++i) {
            const TodoItem *item = m_todos.at(i);
            QString prefix = QString("todos/%1/").arg(i);

            m_settings->save(prefix + "id", item->id());
            m_settings->save(prefix + "title", item->title());
            m_settings->save(prefix + "description", item->description());
            m_settings->save(prefix + "category", item->category());
            m_settings->save(prefix + "urgency", item->urgency());
            m_settings->save(prefix + "importance", item->importance());
            m_settings->save(prefix + "status", item->status());
            m_settings->save(prefix + "createdAt", item->createdAt());
            m_settings->save(prefix + "updatedAt", item->updatedAt());
            m_settings->save(prefix + "synced", item->synced());
        }

        qDebug() << "已成功保存" << m_todos.size() << "个待办事项到本地存储";
        success = true;
    } catch (const std::exception &e) {
        qCritical() << "保存到本地存储时发生异常:" << e.what();
        success = false;
        logError("保存本地存储", QString("异常: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "保存到本地存储时发生未知异常";
        success = false;
        logError("保存本地存储", "未知异常");
    }

    return success;
}

/**
 * @brief 从服务器获取最新的待办事项
 */
void TodoModel::fetchTodosFromServer() {
    if (!m_isOnline || !isLoggedIn()) {
        qWarning() << "无法获取服务器数据：离线或未登录";
        return;
    }

    qDebug() << "从服务器获取待办事项...";

    try {
        // 创建网络请求
        QNetworkRequest request(QUrl(getApiUrl(m_todoApiEndpoint)));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_accessToken).toUtf8());

        // 设置请求超时
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);

        // 启动超时计时器
        m_networkTimeoutTimer->start(NETWORK_TIMEOUT_MS);

        QNetworkReply *reply = m_networkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            handleSyncResponse(reply);

            // 成功获取数据后，推送本地更改
            if (reply->error() == QNetworkReply::NoError) {
                pushLocalChangesToServer();
            }
        });
    } catch (const std::exception &e) {
        m_networkTimeoutTimer->stop();
        qCritical() << "获取服务器数据时发生异常:" << e.what();
        logError("获取服务器数据", QString("异常: %1").arg(e.what()));
        emit syncCompleted(false, QString("获取服务器数据失败: %1").arg(e.what()));
    } catch (...) {
        m_networkTimeoutTimer->stop();
        qCritical() << "获取服务器数据时发生未知异常";
        logError("获取服务器数据", "未知异常");
        emit syncCompleted(false, "获取服务器数据失败：未知错误");
    }
}

/**
 * @brief 记录错误信息
 * @param context 错误发生的上下文
 * @param error 错误信息
 */
void TodoModel::logError(const QString &context, const QString &error) {
    qCritical() << "[错误] -" << context << ":" << error;

    // 可以将错误记录到日志文件中
    // TODO: 实现日志文件记录

    // 也可以在这里添加错误报告机制
}

/**
 * @brief 将本地更改推送到服务器
 */
void TodoModel::pushLocalChangesToServer() {
    // 检查网络和登录状态
    if (!m_isOnline || !isLoggedIn()) {
        qDebug() << "无法推送更改：离线或未登录";
        return;
    }

    // 找出所有未同步的项目
    QList<TodoItem *> unsyncedItems;
    for (TodoItem *item : std::as_const(m_todos)) {
        if (!item->synced()) {
            unsyncedItems.append(item);
        }
    }

    if (unsyncedItems.isEmpty()) {
        qDebug() << "没有需要同步的项目";
        return; // 没有未同步的项目
    }

    qDebug() << "推送" << unsyncedItems.size() << "个项目到服务器";

    try {
        // 创建一个包含所有未同步项目的JSON数组
        QJsonArray jsonArray;
        for (TodoItem *item : std::as_const(unsyncedItems)) {
            QJsonObject obj;
            obj["id"] = item->id();
            obj["title"] = item->title();
            obj["description"] = item->description();
            obj["category"] = item->category();
            obj["urgency"] = item->urgency();
            obj["importance"] = item->importance();
            obj["status"] = item->status();
            obj["created_at"] = item->createdAt().toString(Qt::ISODate);
            obj["updated_at"] = item->updatedAt().toString(Qt::ISODate);

            jsonArray.append(obj);
        }

        // 创建网络请求
        QNetworkRequest request(QUrl(getApiUrl(m_todoApiEndpoint)));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_accessToken).toUtf8());

        QNetworkReply *reply = m_networkManager->post(request, QJsonDocument(jsonArray).toJson());

        // 启动超时计时器
        m_networkTimeoutTimer->start(NETWORK_TIMEOUT_MS);

        connect(reply, &QNetworkReply::finished, this, [this, reply, unsyncedItems]() {
            // 停止超时计时器
            m_networkTimeoutTimer->stop();

            if (reply->error() == QNetworkReply::NoError) {
                qDebug() << "成功推送更改到服务器";

                // 标记这些项目为已同步
                for (TodoItem *item : unsyncedItems) {
                    item->setSynced(true);
                }

                // 保存到本地存储
                if (!saveToLocalStorage()) {
                    qWarning() << "无法在同步后保存本地存储";
                }
            } else {
                QString errorMessage = reply->errorString();
                qWarning() << "推送更改失败:" << errorMessage;
                logError("推送更改", errorMessage);
            }

            reply->deleteLater();
        });
    } catch (const std::exception &e) {
        qCritical() << "推送更改时发生异常:" << e.what();
        logError("推送更改", QString("异常: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "推送更改时发生未知异常";
        logError("推送更改", "未知异常");
    }
}

/**
 * @brief 保存设置到配置文件
 * @param key 设置键名
 * @param value 设置值
 * @return 保存是否成功
 */
bool TodoModel::save(const QString &key, const QVariant &value) {
    return m_settings->save(key, value);
}

/**
 * @brief 从配置文件读取设置
 * @param key 设置键名
 * @param defaultValue 默认值（如果设置不存在）
 * @return 设置值
 */
QVariant TodoModel::get(const QString &key, const QVariant &defaultValue) {
    return m_settings->get(key, defaultValue);
}

/**
 * @brief 初始化服务器配置
 */
void TodoModel::initializeServerConfig() {
    // 从设置中读取服务器配置，如果不存在则使用默认值
    m_serverBaseUrl = m_settings->get("server/baseUrl", "https://api.example.com").toString();
    m_todoApiEndpoint = m_settings->get("server/todoApiEndpoint", "/todo_api.php").toString();
    m_authApiEndpoint = m_settings->get("server/authApiEndpoint", "/auth_api.php").toString();
    
    qDebug() << "服务器配置已初始化:";
    qDebug() << "  基础URL:" << m_serverBaseUrl;
    qDebug() << "  待办事项API:" << m_todoApiEndpoint;
    qDebug() << "  认证API:" << m_authApiEndpoint;
}

/**
 * @brief 获取完整的API URL
 * @param endpoint API端点路径
 * @return 完整的API URL
 */
QString TodoModel::getApiUrl(const QString &endpoint) const {
    return m_serverBaseUrl + endpoint;
}
