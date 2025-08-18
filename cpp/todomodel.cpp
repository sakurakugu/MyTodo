#include "todoModel.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkProxy>
#include <QProcess>
#include <QUuid>
#include <map>
#include <algorithm>

#include "networkmanager.h"

TodoModel::TodoModel(QObject *parent, Config *config, Config::StorageType storageType)
    : QAbstractListModel(parent),
      m_filterCacheDirty(true),
      m_isOnline(false),
      m_currentCategory(""),
      m_currentFilter(""),
      m_networkManager(new NetworkManager(this)),
      m_config(config ? config : new Config(this, storageType)) {
    // 初始化默认服务器配置
    m_config->initializeDefaultServerConfig();

    // 初始化服务器配置
    initializeServerConfig();

    // 连接网络管理器信号
    connect(m_networkManager, &NetworkManager::requestCompleted, this, &TodoModel::onNetworkRequestCompleted);
    connect(m_networkManager, &NetworkManager::requestFailed, this, &TodoModel::onNetworkRequestFailed);
    connect(m_networkManager, &NetworkManager::networkStatusChanged, this, &TodoModel::onNetworkStatusChanged);
    connect(m_networkManager, &NetworkManager::authTokenExpired, this, &TodoModel::onAuthTokenExpired);

    // 加载本地数据
    if (!loadFromLocalStorage()) {
        qWarning() << "无法从本地存储加载待办事项数据";
    }

    // 初始化在线状态
    m_isOnline = m_config->get("autoSync", false).toBool();
    emit isOnlineChanged();

    // 尝试使用存储的令牌自动登录
    if (m_config->contains("user/accessToken")) {
        m_accessToken = m_config->get("user/accessToken").toString();
        m_refreshToken = m_config->get("user/refreshToken").toString();
        m_username = m_config->get("user/username").toString();

        // TODO: 在这里验证令牌是否有效
        qDebug() << "使用存储的凭据自动登录用户：" << m_username;
    }
}

/**
 * @brief 析构函数
 *
 * 清理资源，保存未同步的数据到本地存储。
 */
TodoModel::~TodoModel() {
    // 保存数据
    saveToLocalStorage();

    m_todos.clear();
    m_filteredTodos.clear();
}

/**
 * @brief 获取模型中的行数（待办事项数量）
 * @param parent 父索引，默认为无效索引（根）
 * @return 待办事项数量
 */
int TodoModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;

    // 如果没有设置过滤条件，返回所有项目
    if (m_currentCategory.isEmpty() && m_currentFilter.isEmpty()) {
        return m_todos.size();
    }

    // 使用缓存的过滤结果
    const_cast<TodoModel *>(this)->updateFilterCache();
    return m_filteredTodos.count();
}

/**
 * @brief 获取指定索引和角色的数据
 * @param index 待获取数据的模型索引
 * @param role 数据角色
 * @return 请求的数据值
 */
QVariant TodoModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return QVariant();

    // 如果没有设置过滤条件，直接返回对应索引的项目
    if (m_currentCategory.isEmpty() && m_currentFilter.isEmpty()) {
        if (static_cast<size_t>(index.row()) >= m_todos.size()) return QVariant();
        return getItemData(m_todos[index.row()].get(), role);
    }

    // 使用缓存的过滤结果
    const_cast<TodoModel *>(this)->updateFilterCache();
    if (index.row() >= m_filteredTodos.size()) return QVariant();

    return getItemData(m_filteredTodos[index.row()], role);
}

// 辅助方法，根据角色返回项目数据
QVariant TodoModel::getItemData(const TodoItem *item, int role) const {
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
    if (!index.isValid() || static_cast<size_t>(index.row()) >= m_todos.size()) return false;

    TodoItem *item = m_todos.at(index.row()).get();
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
        invalidateFilterCache();
        emit dataChanged(index, index, QVector<int>() << role);
        saveToLocalStorage();
        return true;
    }

    return false;
}

// 性能优化相关方法实现
void TodoModel::updateFilterCache() {
    if (!m_filterCacheDirty) {
        return;  // 缓存仍然有效
    }

    m_filteredTodos.clear();

    // 如果没有过滤条件，缓存所有项目
    if (m_currentCategory.isEmpty() && m_currentFilter.isEmpty()) {
        for (auto it = m_todos.begin(); it != m_todos.end(); ++it) {
            m_filteredTodos.append(it->get());
        }
    } else {
        // 应用过滤条件
        for (auto it = m_todos.begin(); it != m_todos.end(); ++it) {
            if (itemMatchesFilter(it->get())) {
                m_filteredTodos.append(it->get());
            }
        }
    }

    m_filterCacheDirty = false;
}

bool TodoModel::itemMatchesFilter(const TodoItem *item) const {
    if (!item) return false;

    bool categoryMatch = m_currentCategory.isEmpty() || item->category() == m_currentCategory;
    bool statusMatch = m_currentFilter.isEmpty() || (m_currentFilter == "done" && item->status() == "done") ||
                       (m_currentFilter == "todo" && item->status() == "todo");

    return categoryMatch && statusMatch;
}

TodoItem *TodoModel::getFilteredItem(int index) const {
    const_cast<TodoModel *>(this)->updateFilterCache();

    if (index < 0 || index >= m_filteredTodos.size()) {
        return nullptr;
    }

    return m_filteredTodos[index];
}

void TodoModel::invalidateFilterCache() { m_filterCacheDirty = true; }

/**
 * @brief 获取当前在线状态
 * @return 是否在线
 */
bool TodoModel::isOnline() const { return m_isOnline; }

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
        // TODO: 这部分是不是可以复用

        // 尝试连接服务器，验证是否可以切换到在线模式
        NetworkManager::RequestConfig config;
        config.url = getApiUrl(m_todoApiEndpoint);
        config.requiresAuth = isLoggedIn();
        config.timeout = 5000;  // 5秒超时

        // 发送测试请求
        m_networkManager->sendRequest(NetworkManager::FetchTodos, config);

        // TODO: 暂时设置为在线状态，实际状态将在请求回调中确定
        m_isOnline = online;
        emit isOnlineChanged();
        // 保存到设置，保持与autoSync一致
        m_config->save("setting/autoSync", m_isOnline);

        if (m_isOnline && isLoggedIn()) {
            syncWithServer();
        }
    } else {
        // 切换到离线模式不需要验证，直接更新状态
        m_isOnline = online;
        emit isOnlineChanged();
        // 保存到设置，保持与autoSync一致
        m_config->save("setting/autoSync", m_isOnline);
    }
}

/**
 * @brief 获取当前激活的分类筛选器
 * @return 当前分类名称
 */
QString TodoModel::currentCategory() const { return m_currentCategory; }

/**
 * @brief 设置分类筛选器
 * @param category 分类名称，空字符串表示显示所有分类
 */
void TodoModel::setCurrentCategory(const QString &category) {
    if (m_currentCategory != category) {
        beginResetModel();
        m_currentCategory = category;
        invalidateFilterCache();
        endResetModel();
        emit currentCategoryChanged();
    }
}

/**
 * @brief 获取当前激活的筛选条件
 * @return 当前筛选条件
 */
QString TodoModel::currentFilter() const { return m_currentFilter; }

/**
 * @brief 设置筛选条件
 * @param filter 筛选条件（如"完成"、"未完成"等）
 */
void TodoModel::setCurrentFilter(const QString &filter) {
    if (m_currentFilter != filter) {
        beginResetModel();
        m_currentFilter = filter;
        invalidateFilterCache();
        endResetModel();
        emit currentFilterChanged();
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

    auto newItem = std::make_unique<TodoItem>(QUuid::createUuid().toString(QUuid::WithoutBraces), title, description,
                                              category, urgency, importance, "todo", QDateTime::currentDateTime(),
                                              QDateTime::currentDateTime(), false, this);

    m_todos.push_back(std::move(newItem));
    invalidateFilterCache();

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
    if (index < 0 || static_cast<size_t>(index) >= m_todos.size()) {
        qWarning() << "尝试更新无效的索引:" << index;
        return false;
    }

    QModelIndex modelIndex = createIndex(index, 0);
    bool anyUpdated = false;
    TodoItem *item = m_todos[index].get();
    QVector<int> changedRoles;

    try {
        // 直接更新TodoItem对象，避免多次触发dataChanged信号
        if (todoData.contains("title")) {
            QString newTitle = todoData["title"].toString();
            if (item->title() != newTitle) {
                item->setTitle(newTitle);
                changedRoles << TitleRole;
                anyUpdated = true;
            }
        }

        if (todoData.contains("description")) {
            QString newDescription = todoData["description"].toString();
            if (item->description() != newDescription) {
                item->setDescription(newDescription);
                changedRoles << DescriptionRole;
                anyUpdated = true;
            }
        }

        if (todoData.contains("category")) {
            QString newCategory = todoData["category"].toString();
            if (item->category() != newCategory) {
                item->setCategory(newCategory);
                changedRoles << CategoryRole;
                anyUpdated = true;
            }
        }

        if (todoData.contains("urgency")) {
            QString newUrgency = todoData["urgency"].toString();
            if (item->urgency() != newUrgency) {
                item->setUrgency(newUrgency);
                changedRoles << UrgencyRole;
                anyUpdated = true;
            }
        }

        if (todoData.contains("importance")) {
            QString newImportance = todoData["importance"].toString();
            if (item->importance() != newImportance) {
                item->setImportance(newImportance);
                changedRoles << ImportanceRole;
                anyUpdated = true;
            }
        }

        if (todoData.contains("status")) {
            QString newStatus = todoData["status"].toString();
            if (item->status() != newStatus) {
                item->setStatus(newStatus);
                changedRoles << StatusRole;
                anyUpdated = true;
            }
        }

        // 如果有任何更新，则触发一次dataChanged信号并保存
        if (anyUpdated) {
            item->setUpdatedAt(QDateTime::currentDateTime());
            item->setSynced(false);
            invalidateFilterCache();

            // 只触发一次dataChanged信号
            emit dataChanged(modelIndex, modelIndex, changedRoles);

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
    if (index < 0 || static_cast<size_t>(index) >= m_todos.size()) {
        qWarning() << "尝试删除无效的索引:" << index;
        return false;
    }

    try {
        beginRemoveRows(QModelIndex(), index, index);

        m_todos.erase(m_todos.begin() + index);
        invalidateFilterCache();

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
    if (index < 0 || static_cast<size_t>(index) >= m_todos.size()) {
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

    // 准备同步请求配置
    NetworkManager::RequestConfig config;
    config.url = getApiUrl(m_todoApiEndpoint);
    config.requiresAuth = true;

    // 发送同步请求
    m_networkManager->sendRequest(NetworkManager::Sync, config);
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

    // 准备请求配置
    NetworkManager::RequestConfig config;
    config.url = getApiUrl(m_authApiEndpoint) + "?action=login";
    config.requiresAuth = false;  // 登录请求不需要认证

    // 创建登录数据
    config.data["username"] = username;
    config.data["password"] = password;

    // 发送登录请求
    emit syncStarted();
    m_networkManager->sendRequest(NetworkManager::Login, config);
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

    m_config->remove("user/accessToken");
    m_config->remove("user/refreshToken");
    m_config->remove("user/username");

    // 标记所有项为未同步
    for (size_t i = 0; i < m_todos.size(); ++i) {
        m_todos[i]->setSynced(false);
    }

    // 发出用户名变化信号
    emit usernameChanged();
    // 发出登录状态变化信号
    emit isLoggedInChanged();

    // 发出退出登录成功信号
    emit logoutSuccessful();
}

/**
 * @brief 检查用户是否已登录
 * @return 是否已登录
 */
bool TodoModel::isLoggedIn() const { return !m_accessToken.isEmpty(); }

/**
 * @brief 获取用户名
 * @return 用户名
 */
QString TodoModel::getUsername() const { return m_username; }

/**
 * @brief 获取邮箱
 * @return 邮箱
 */
QString TodoModel::getEmail() const { return m_email; }

void TodoModel::onNetworkRequestCompleted(NetworkManager::RequestType type, const QJsonObject &response) {
    switch (type) {
        case NetworkManager::Login:
            handleLoginSuccess(response);
            break;
        case NetworkManager::Sync:
            handleSyncSuccess(response);
            break;
        case NetworkManager::FetchTodos:
            handleFetchTodosSuccess(response);
            break;
        case NetworkManager::PushTodos:
            handlePushChangesSuccess(response);
            break;
        case NetworkManager::Logout:
            // 注销成功处理
            emit logoutSuccessful();
            break;
    }
}

void TodoModel::onNetworkRequestFailed(NetworkManager::RequestType type, NetworkManager::NetworkError error,
                                       const QString &errorMessage) {
    Q_UNUSED(error)  // 标记未使用的参数

    QString typeStr;
    switch (type) {
        case NetworkManager::Login:
            typeStr = "登录";
            emit loginFailed(errorMessage);
            break;
        case NetworkManager::Sync:
            typeStr = "同步";
            emit syncCompleted(false, errorMessage);
            break;
        case NetworkManager::FetchTodos:
            typeStr = "获取待办事项";
            emit syncCompleted(false, errorMessage);
            break;
        case NetworkManager::PushTodos:
            typeStr = "推送更改";
            emit syncCompleted(false, errorMessage);
            break;
        case NetworkManager::Logout:
            typeStr = "注销";
            emit logoutSuccessful();
            break;
    }

    qWarning() << typeStr << "失败:" << errorMessage;
    logError(typeStr, errorMessage);
}

void TodoModel::onNetworkStatusChanged(bool isOnline) {
    if (m_isOnline != isOnline) {
        m_isOnline = isOnline;
        emit isOnlineChanged();
        qDebug() << "网络状态变更:" << (isOnline ? "在线" : "离线");
    }
}

void TodoModel::onAuthTokenExpired() {
    qWarning() << "认证令牌已过期，需要重新登录";
    logout();
    emit loginRequired();
}

void TodoModel::handleLoginSuccess(const QJsonObject &response) {
    qDebug() << "登录成功";

    // 验证响应中包含必要的字段
    if (!response.contains("access_token") || !response.contains("refresh_token") || !response.contains("user")) {
        emit loginFailed("服务器响应缺少必要字段");
        return;
    }

    m_accessToken = response["access_token"].toString();
    m_refreshToken = response["refresh_token"].toString();
    m_username = response["user"].toObject()["username"].toString();

    // 设置网络管理器的认证令牌
    m_networkManager->setAuthToken(m_accessToken);

    // 保存令牌
    m_config->save("user/accessToken", m_accessToken);
    m_config->save("user/refreshToken", m_refreshToken);
    m_config->save("user/username", m_username);

    qDebug() << "用户" << m_username << "登录成功";

    // 发出用户名变化信号
    emit usernameChanged();
    // 发出登录状态变化信号
    emit isLoggedInChanged();

    emit loginSuccessful(m_username);

    // 登录成功后自动同步
    if (m_isOnline) {
        syncWithServer();
    }
}

void TodoModel::handleSyncSuccess(const QJsonObject &response) {
    qDebug() << "同步成功";

    // 处理同步响应数据
    if (response.contains("todos")) {
        QJsonArray todosArray = response["todos"].toArray();
        // 更新本地数据
        updateTodosFromServer(todosArray);
    }

    emit syncCompleted(true, "同步完成");
}

void TodoModel::handleFetchTodosSuccess(const QJsonObject &response) {
    qDebug() << "获取待办事项成功";

    if (response.contains("todos")) {
        QJsonArray todosArray = response["todos"].toArray();
        updateTodosFromServer(todosArray);
    }

    // 成功获取数据后，推送本地更改
    pushLocalChangesToServer();

    emit syncCompleted(true, "数据获取完成");
}

void TodoModel::handlePushChangesSuccess(const QJsonObject &response) {
    qDebug() << "推送更改成功";

    // 标记待同步项目为已同步
    for (TodoItem *item : m_pendingUnsyncedItems) {
        item->setSynced(true);
    }

    // 清空待同步项目列表
    m_pendingUnsyncedItems.clear();

    // 保存到本地存储
    if (!saveToLocalStorage()) {
        qWarning() << "无法在同步后保存本地存储";
    }

    // 处理推送响应
    if (response.contains("updated_count")) {
        int updatedCount = response["updated_count"].toInt();
        qDebug() << "已更新" << updatedCount << "个待办事项";
    }

    emit syncCompleted(true, "更改推送完成");
}

void TodoModel::updateTodosFromServer(const QJsonArray &todosArray) {
    // TODO: 实现从服务器数据更新本地待办事项的逻辑
    qDebug() << "从服务器更新" << todosArray.size() << "个待办事项";

    // 这里应该实现:
    // 1. 解析服务器返回的待办事项数据
    // 2. 与本地数据进行比较和合并
    // 3. 处理冲突解决
    // 4. 更新模型数据
    // 5. 保存到本地存储
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
        m_todos.clear();
        invalidateFilterCache();

        // 检查设置是否可访问
        if (!m_config) {
            qWarning() << "设置对象不可用";
            return false;
        }

        // 从设置中加载数据
        int count = m_config->get("todos/size", 0).toInt();
        qDebug() << "从本地存储加载" << count << "个待办事项";

        for (int i = 0; i < count; ++i) {
            QString prefix = QString("todos/%1/").arg(i);

            // 验证必要字段
            if (!m_config->contains(prefix + "id") || !m_config->contains(prefix + "title")) {
                qWarning() << "跳过无效的待办事项记录（索引" << i << "）：缺少必要字段";
                continue;
            }

            auto item = std::make_unique<TodoItem>(
                m_config->get(prefix + "id").toString(), m_config->get(prefix + "title").toString(),
                m_config->get(prefix + "description").toString(), m_config->get(prefix + "category").toString(),
                m_config->get(prefix + "urgency").toString(), m_config->get(prefix + "importance").toString(),
                m_config->get(prefix + "status").toString(), m_config->get(prefix + "createdAt").toDateTime(),
                m_config->get(prefix + "updatedAt").toDateTime(), m_config->get(prefix + "synced").toBool(), this);

            m_todos.push_back(std::move(item));
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
        if (!m_config) {
            qWarning() << "设置对象不可用";
            return false;
        }

        // 保存待办事项数量
        m_config->save("todos/size", m_todos.size());

        // 保存每个待办事项
        for (size_t i = 0; i < m_todos.size(); ++i) {
            const TodoItem *item = m_todos.at(i).get();
            QString prefix = QString("todos/%1/").arg(i);

            m_config->save(prefix + "id", item->id());
            m_config->save(prefix + "title", item->title());
            m_config->save(prefix + "description", item->description());
            m_config->save(prefix + "category", item->category());
            m_config->save(prefix + "urgency", item->urgency());
            m_config->save(prefix + "importance", item->importance());
            m_config->save(prefix + "status", item->status());
            m_config->save(prefix + "createdAt", item->createdAt());
            m_config->save(prefix + "updatedAt", item->updatedAt());
            m_config->save(prefix + "synced", item->synced());
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
        NetworkManager::RequestConfig config;
        config.url = getApiUrl(m_todoApiEndpoint);
        config.requiresAuth = true;

        m_networkManager->sendRequest(NetworkManager::FetchTodos, config);
    } catch (const std::exception &e) {
        qCritical() << "获取服务器数据时发生异常:" << e.what();
        logError("获取服务器数据", QString("异常: %1").arg(e.what()));
        emit syncCompleted(false, QString("获取服务器数据失败: %1").arg(e.what()));
    } catch (...) {
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
    for (const auto &item : std::as_const(m_todos)) {
        if (!item->synced()) {
            unsyncedItems.append(item.get());
        }
    }

    if (unsyncedItems.isEmpty()) {
        qDebug() << "没有需要同步的项目";
        return;  // 没有未同步的项目
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

        NetworkManager::RequestConfig config;
        config.url = getApiUrl(m_todoApiEndpoint);
        config.requiresAuth = true;
        config.data["todos"] = jsonArray;

        // 存储未同步项目的引用，用于成功后标记为已同步
        m_pendingUnsyncedItems = unsyncedItems;

        m_networkManager->sendRequest(NetworkManager::PushTodos, config);
    } catch (const std::exception &e) {
        qCritical() << "推送更改时发生异常:" << e.what();
        logError("推送更改", QString("异常: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "推送更改时发生未知异常";
        logError("推送更改", "未知异常");
    }
}

/**
 * @brief 初始化服务器配置
 */
void TodoModel::initializeServerConfig() {
    // 从设置中读取服务器配置，如果不存在则使用默认值
    m_serverBaseUrl = m_config->get("server/baseUrl", "https://api.example.com").toString();
    m_todoApiEndpoint = m_config->get("server/todoApiEndpoint", "/todo_api.php").toString();
    m_authApiEndpoint = m_config->get("server/authApiEndpoint", "/auth_api.php").toString();

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
QString TodoModel::getApiUrl(const QString &endpoint) const { return m_serverBaseUrl + endpoint; }

/**
 * @brief 检查URL是否使用HTTPS协议
 * @param url 要检查的URL
 * @return 如果使用HTTPS则返回true，否则返回false
 */
bool TodoModel::isHttpsUrl(const QString &url) const { return url.startsWith("https://", Qt::CaseInsensitive); }

/**
 * @brief 更新服务器配置
 * @param baseUrl 新的服务器基础URL
 */
void TodoModel::updateServerConfig(const QString &baseUrl) {
    if (baseUrl.isEmpty()) {
        qWarning() << "尝试设置空的服务器URL";
        return;
    }

    // 更新内存中的配置
    m_serverBaseUrl = baseUrl;

    // 保存到设置中
    m_config->save("server/baseUrl", baseUrl);

    qDebug() << "服务器配置已更新:" << baseUrl;
    qDebug() << "HTTPS状态:" << (isHttpsUrl(baseUrl) ? "安全" : "不安全");
}

bool TodoModel::exportTodos(const QString &filePath) {
    QJsonArray todosArray;

    // 将所有待办事项转换为JSON格式
    for (const auto &todo : m_todos) {
        QJsonObject todoObj;
        todoObj["id"] = todo->id();
        todoObj["title"] = todo->title();
        todoObj["description"] = todo->description();
        todoObj["category"] = todo->category();
        todoObj["urgency"] = todo->urgency();
        todoObj["importance"] = todo->importance();
        todoObj["status"] = todo->status();
        todoObj["createdAt"] = todo->createdAt().toString(Qt::ISODate);
        todoObj["updatedAt"] = todo->updatedAt().toString(Qt::ISODate);
        todoObj["synced"] = todo->synced();

        todosArray.append(todoObj);
    }

    // 创建根JSON对象
    QJsonObject rootObj;
    rootObj["version"] = "1.0";
    rootObj["exportDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    rootObj["todos"] = todosArray;

    // 写入文件
    QJsonDocument doc(rootObj);
    QFile file(filePath);

    // 确保目录存在
    QFileInfo fileInfo(filePath);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "无法打开文件进行写入:" << filePath;
        return false;
    }

    file.write(doc.toJson());
    file.close();

    qDebug() << "成功导出" << m_todos.size() << "个待办事项到" << filePath;
    return true;
}

QVariantList TodoModel::importTodosWithAutoResolution(const QString &filePath) {
    QVariantList conflicts;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件进行读取:" << filePath;
        return conflicts;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON解析错误:" << error.errorString();
        return conflicts;
    }

    QJsonObject rootObj = doc.object();

    // 检查版本兼容性
    QString version = rootObj["version"].toString();
    if (version != "1.0") {
        qWarning() << "不支持的文件版本:" << version;
        return conflicts;
    }

    QJsonArray todosArray = rootObj["todos"].toArray();

    // 分离冲突和非冲突项目
    QJsonArray nonConflictTodos;

    qDebug() << "开始检查导入冲突，现有项目数量:" << m_todos.size() << "，导入项目数量:" << todosArray.size();

    // 打印现有项目的ID
    for (size_t i = 0; i < m_todos.size(); ++i) {
        qDebug() << "现有项目" << i << "ID:" << m_todos[i]->id() << "标题:" << m_todos[i]->title();
    }

    for (const QJsonValue &value : todosArray) {
        QJsonObject todoObj = value.toObject();
        QString id = todoObj["id"].toString();

        bool hasConflict = false;
        bool shouldSkip = false;
        TodoItem *existingTodo = nullptr;

        // 查找是否存在相同ID的待办事项
        for (const auto &todo : m_todos) {
            if (todo->id() == id) {
                // 检查内容是否真的不同
                QString importTitle = todoObj["title"].toString();
                QString importDescription = todoObj["description"].toString();
                QString importCategory = todoObj["category"].toString();
                QString importStatus = todoObj["status"].toString();

                if (todo->title() != importTitle || todo->description() != importDescription ||
                    todo->category() != importCategory || todo->status() != importStatus) {
                    hasConflict = true;
                    existingTodo = todo.get();
                    qDebug() << "发现真正冲突项目 ID:" << id << "现有标题:" << todo->title()
                             << "导入标题:" << importTitle;
                } else {
                    qDebug() << "ID相同且内容一致，直接跳过 ID:" << id << "标题:" << importTitle;
                    // 内容完全一致的项目直接跳过，既不导入也不显示冲突
                    shouldSkip = true;
                }
                break;
            }
        }

        if (shouldSkip) {
            // 跳过内容完全一致的项目
            continue;
        } else if (hasConflict) {
            // 发现冲突，添加到冲突列表
            QVariantMap conflictInfo;
            conflictInfo["id"] = id;
            conflictInfo["existingTitle"] = existingTodo->title();
            conflictInfo["existingDescription"] = existingTodo->description();
            conflictInfo["existingCategory"] = existingTodo->category();
            conflictInfo["existingStatus"] = existingTodo->status();
            conflictInfo["existingUpdatedAt"] = existingTodo->updatedAt();

            conflictInfo["importTitle"] = todoObj["title"].toString();
            conflictInfo["importDescription"] = todoObj["description"].toString();
            conflictInfo["importCategory"] = todoObj["category"].toString();
            conflictInfo["importStatus"] = todoObj["status"].toString();
            conflictInfo["importUpdatedAt"] = QDateTime::fromString(todoObj["updatedAt"].toString(), Qt::ISODate);

            conflicts.append(conflictInfo);
        } else {
            // 无冲突，添加到非冲突列表
            qDebug() << "无冲突项目 ID:" << id << "标题:" << todoObj["title"].toString();
            nonConflictTodos.append(value);
        }
    }

    qDebug() << "冲突检查完成，冲突项目数量:" << conflicts.size() << "，无冲突项目数量:" << nonConflictTodos.size();

    // 导入无冲突的项目
    if (nonConflictTodos.size() > 0) {
        beginInsertRows(QModelIndex(), m_todos.size(), m_todos.size() + nonConflictTodos.size() - 1);

        for (const QJsonValue &value : nonConflictTodos) {
            QJsonObject todoObj = value.toObject();

            auto newTodo = std::make_unique<TodoItem>(
                todoObj["id"].toString(), todoObj["title"].toString(), todoObj["description"].toString(),
                todoObj["category"].toString(), todoObj["urgency"].toString(), todoObj["importance"].toString(),
                todoObj["status"].toString(), QDateTime::fromString(todoObj["createdAt"].toString(), Qt::ISODate),
                QDateTime::fromString(todoObj["updatedAt"].toString(), Qt::ISODate),
                false,  // 导入的项目标记为未同步
                this);

            m_todos.push_back(std::move(newTodo));
        }

        endInsertRows();
    }

    // 保存到本地存储
    if (nonConflictTodos.size() > 0) {
        saveToLocalStorage();
    }

    return conflicts;
}

bool TodoModel::importTodos(const QString &filePath) {
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件进行读取:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON解析错误:" << error.errorString();
        return false;
    }

    QJsonObject rootObj = doc.object();

    // 检查版本兼容性
    QString version = rootObj["version"].toString();
    if (version != "1.0") {
        qWarning() << "不支持的文件版本:" << version;
        return false;
    }

    QJsonArray todosArray = rootObj["todos"].toArray();
    int importedCount = 0;
    int skippedCount = 0;

    beginResetModel();

    for (const QJsonValue &value : todosArray) {
        QJsonObject todoObj = value.toObject();

        QString id = todoObj["id"].toString();

        // 检查是否已存在相同ID的待办事项
        bool exists = false;
        for (const auto &existingTodo : m_todos) {
            if (existingTodo->id() == id) {
                exists = true;
                skippedCount++;
                break;
            }
        }

        if (!exists) {
            // 创建新的待办事项
            auto newTodo = std::make_unique<TodoItem>(
                id, todoObj["title"].toString(), todoObj["description"].toString(), todoObj["category"].toString(),
                todoObj["urgency"].toString(), todoObj["importance"].toString(), todoObj["status"].toString(),
                QDateTime::fromString(todoObj["createdAt"].toString(), Qt::ISODate),
                QDateTime::fromString(todoObj["updatedAt"].toString(), Qt::ISODate), todoObj["synced"].toBool(), this);

            m_todos.push_back(std::move(newTodo));
            importedCount++;
        }
    }

    endResetModel();

    // 保存到本地存储
    saveToLocalStorage();

    qDebug() << "导入完成 - 新增:" << importedCount << "个，跳过:" << skippedCount << "个";
    return true;
}

QVariantList TodoModel::checkImportConflicts(const QString &filePath) {
    QVariantList conflicts;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件进行读取:" << filePath;
        return conflicts;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON解析错误:" << error.errorString();
        return conflicts;
    }

    QJsonObject rootObj = doc.object();

    // 检查版本兼容性
    QString version = rootObj["version"].toString();
    if (version != "1.0") {
        qWarning() << "不支持的文件版本:" << version;
        return conflicts;
    }

    QJsonArray todosArray = rootObj["todos"].toArray();

    for (const QJsonValue &value : todosArray) {
        QJsonObject todoObj = value.toObject();
        QString id = todoObj["id"].toString();

        // 查找是否存在相同ID的待办事项
        for (const auto &existingTodo : m_todos) {
            if (existingTodo->id() == id) {
                // 发现冲突，创建冲突信息
                QVariantMap conflictInfo;
                conflictInfo["id"] = id;
                conflictInfo["existingTitle"] = existingTodo->title();
                conflictInfo["existingDescription"] = existingTodo->description();
                conflictInfo["existingCategory"] = existingTodo->category();
                conflictInfo["existingStatus"] = existingTodo->status();
                conflictInfo["existingUpdatedAt"] = existingTodo->updatedAt();

                conflictInfo["importTitle"] = todoObj["title"].toString();
                conflictInfo["importDescription"] = todoObj["description"].toString();
                conflictInfo["importCategory"] = todoObj["category"].toString();
                conflictInfo["importStatus"] = todoObj["status"].toString();
                conflictInfo["importUpdatedAt"] = QDateTime::fromString(todoObj["updatedAt"].toString(), Qt::ISODate);

                conflicts.append(conflictInfo);
                break;
            }
        }
    }

    return conflicts;
}

bool TodoModel::importTodosWithConflictResolution(const QString &filePath, const QString &conflictResolution) {
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件进行读取:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON解析错误:" << error.errorString();
        return false;
    }

    QJsonObject rootObj = doc.object();

    // 检查版本兼容性
    QString version = rootObj["version"].toString();
    if (version != "1.0") {
        qWarning() << "不支持的文件版本:" << version;
        return false;
    }

    QJsonArray todosArray = rootObj["todos"].toArray();
    int importedCount = 0;
    int skippedCount = 0;
    int overwrittenCount = 0;

    beginResetModel();

    for (const QJsonValue &value : todosArray) {
        QJsonObject todoObj = value.toObject();
        QString id = todoObj["id"].toString();

        // 查找是否已存在相同ID的待办事项
        bool exists = false;
        int existingIndex = -1;
        for (size_t i = 0; i < m_todos.size(); ++i) {
            if (m_todos[i]->id() == id) {
                exists = true;
                existingIndex = static_cast<int>(i);
                break;
            }
        }

        if (exists) {
            if (conflictResolution == "overwrite") {
                // 覆盖现有项目
                TodoItem *existingItem = m_todos[existingIndex].get();
                existingItem->setTitle(todoObj["title"].toString());
                existingItem->setDescription(todoObj["description"].toString());
                existingItem->setCategory(todoObj["category"].toString());
                existingItem->setUrgency(todoObj["urgency"].toString());
                existingItem->setImportance(todoObj["importance"].toString());
                existingItem->setStatus(todoObj["status"].toString());
                existingItem->setUpdatedAt(QDateTime::fromString(todoObj["updatedAt"].toString(), Qt::ISODate));
                existingItem->setSynced(todoObj["synced"].toBool());
                overwrittenCount++;
            } else if (conflictResolution == "merge") {
                // 合并：保留较新的更新时间的版本
                TodoItem *existingItem = m_todos[existingIndex].get();
                QDateTime importUpdatedAt = QDateTime::fromString(todoObj["updatedAt"].toString(), Qt::ISODate);

                if (importUpdatedAt > existingItem->updatedAt()) {
                    // 导入的版本更新，使用导入的数据
                    existingItem->setTitle(todoObj["title"].toString());
                    existingItem->setDescription(todoObj["description"].toString());
                    existingItem->setCategory(todoObj["category"].toString());
                    existingItem->setUrgency(todoObj["urgency"].toString());
                    existingItem->setImportance(todoObj["importance"].toString());
                    existingItem->setStatus(todoObj["status"].toString());
                    existingItem->setUpdatedAt(importUpdatedAt);
                    existingItem->setSynced(todoObj["synced"].toBool());
                    overwrittenCount++;
                }
                // 如果现有版本更新或相同，则保持不变
            } else if (conflictResolution == "skip") {
                // 跳过冲突项目
                skippedCount++;
            }
        } else {
            // 创建新的待办事项
            auto newTodo = std::make_unique<TodoItem>(
                id, todoObj["title"].toString(), todoObj["description"].toString(), todoObj["category"].toString(),
                todoObj["urgency"].toString(), todoObj["importance"].toString(), todoObj["status"].toString(),
                QDateTime::fromString(todoObj["createdAt"].toString(), Qt::ISODate),
                QDateTime::fromString(todoObj["updatedAt"].toString(), Qt::ISODate), todoObj["synced"].toBool(), this);

            m_todos.push_back(std::move(newTodo));
            importedCount++;
        }
    }

    endResetModel();

    // 保存到本地存储
    saveToLocalStorage();

    qDebug() << "导入完成 - 新增:" << importedCount << "个，覆盖:" << overwrittenCount << "个，跳过:" << skippedCount
             << "个";
    return true;
}

bool TodoModel::importTodosWithIndividualResolution(const QString &filePath, const QVariantMap &resolutions) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件进行读取:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON解析错误:" << error.errorString();
        return false;
    }

    if (!doc.isArray()) {
        qWarning() << "JSON文档不是数组格式";
        return false;
    }

    QJsonArray jsonArray = doc.array();
    int importedCount = 0;
    int updatedCount = 0;
    int skippedCount = 0;

    for (const QJsonValue &value : jsonArray) {
        if (!value.isObject()) continue;

        QJsonObject obj = value.toObject();
        QString id = obj["id"].toString();

        // 查找是否存在相同ID的项目
        TodoItem *existingItem = nullptr;
        for (const auto &item : m_todos) {
            if (item->id() == id) {
                existingItem = item.get();
                break;
            }
        }

        if (existingItem) {
            // 获取该项目的处理方式
            QString resolution = resolutions.value(id, "skip").toString();

            if (resolution == "overwrite") {
                // 覆盖现有数据
                existingItem->setTitle(obj["title"].toString());
                existingItem->setDescription(obj["description"].toString());
                existingItem->setCategory(obj["category"].toString());
                existingItem->setStatus(obj["status"].toString());
                existingItem->setCreatedAt(QDateTime::fromString(obj["createdAt"].toString(), Qt::ISODate));
                existingItem->setUpdatedAt(QDateTime::fromString(obj["updatedAt"].toString(), Qt::ISODate));
                existingItem->setSynced(false);  // 标记为未同步
                updatedCount++;
            } else if (resolution == "merge") {
                // 智能合并：保留更新时间较新的版本
                QDateTime existingUpdated = existingItem->updatedAt();
                QDateTime importUpdated = QDateTime::fromString(obj["updatedAt"].toString(), Qt::ISODate);

                if (importUpdated > existingUpdated) {
                    existingItem->setTitle(obj["title"].toString());
                    existingItem->setDescription(obj["description"].toString());
                    existingItem->setCategory(obj["category"].toString());
                    existingItem->setStatus(obj["status"].toString());
                    existingItem->setCreatedAt(QDateTime::fromString(obj["createdAt"].toString(), Qt::ISODate));
                    existingItem->setUpdatedAt(importUpdated);
                    existingItem->setSynced(false);  // 标记为未同步
                    updatedCount++;
                } else {
                    skippedCount++;  // 现有数据更新，跳过导入
                }
            } else {
                // skip - 跳过冲突项目
                skippedCount++;
            }
        } else {
            // 创建新项目（没有冲突的项目直接导入）
            auto newItem = std::make_unique<TodoItem>(this);
            newItem->setId(id);
            newItem->setTitle(obj["title"].toString());
            newItem->setDescription(obj["description"].toString());
            newItem->setCategory(obj["category"].toString());
            newItem->setStatus(obj["status"].toString());
            newItem->setCreatedAt(QDateTime::fromString(obj["createdAt"].toString(), Qt::ISODate));
            newItem->setUpdatedAt(QDateTime::fromString(obj["updatedAt"].toString(), Qt::ISODate));
            newItem->setSynced(false);  // 标记为未同步

            beginInsertRows(QModelIndex(), m_todos.size(), m_todos.size());
            m_todos.push_back(std::move(newItem));
            endInsertRows();
            importedCount++;
        }
    }

    // 保存到本地存储
    saveToLocalStorage();

    qDebug() << "个别冲突处理导入完成 - 新增:" << importedCount << "个，更新:" << updatedCount
             << "个，跳过:" << skippedCount << "个";
    return true;
}
