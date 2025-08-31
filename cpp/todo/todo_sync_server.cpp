/**
 * @file todo_sync_server.cpp
 * @brief TodoSyncServer类的实现文件
 *
 * 该文件实现了TodoSyncServer类的所有方法，负责待办事项的服务器同步功能。
 *
 * @author Sakurakugu
 * @date 2025-01-25
 * @version 1.0.0
 */

#include "todo_sync_server.h"
#include "user_auth.h"
#include "default_value.h"

#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUuid>
#include <algorithm>

TodoSyncServer::TodoSyncServer(QObject *parent)
    : QObject(parent),                                 //
      m_networkRequest(NetworkRequest::GetInstance()), //
      m_setting(Setting::GetInstance()),               //
      m_autoSyncTimer(new QTimer(this)),               //
      m_isAutoSyncEnabled(false),                      //
      m_isSyncing(false),                              //
      m_autoSyncInterval(30),                          //
      m_currentSyncDirection(Bidirectional),          //
      m_currentPushIndex(0),                           //
      m_currentBatchIndex(0),                          //
      m_totalBatches(0) {

    // 初始化服务器配置
    initializeServerConfig();

    // 连接网络请求信号
    connect(&m_networkRequest, &NetworkRequest::requestCompleted, this, &TodoSyncServer::onNetworkRequestCompleted);
    connect(&m_networkRequest, &NetworkRequest::requestFailed, this, &TodoSyncServer::onNetworkRequestFailed);

    // 连接设置变化信号
    connect(&m_setting, &Setting::baseUrlChanged, this, &TodoSyncServer::onBaseUrlChanged);

    // 配置自动同步定时器
    m_autoSyncTimer->setSingleShot(false);
    connect(m_autoSyncTimer, &QTimer::timeout, this, &TodoSyncServer::onAutoSyncTimer);

    // 从设置中恢复状态
    m_isAutoSyncEnabled = m_setting.get(QStringLiteral("sync/autoSyncEnabled"), false).toBool();
    m_autoSyncInterval = m_setting.get(QStringLiteral("sync/autoSyncInterval"), 30).toInt();
    m_lastSyncTime = m_setting.get(QStringLiteral("sync/lastSyncTime"), QString()).toString();

    // 如果启用了自动同步，启动定时器
    if (m_isAutoSyncEnabled) {
        startAutoSyncTimer();
    }

    qDebug() << "TodoSyncServer 初始化完成";
}

TodoSyncServer::~TodoSyncServer() {
    // 保存当前状态到设置
    m_setting.save(QStringLiteral("sync/autoSyncEnabled"), m_isAutoSyncEnabled);
    m_setting.save(QStringLiteral("sync/autoSyncInterval"), m_autoSyncInterval);
    m_setting.save(QStringLiteral("sync/lastSyncTime"), m_lastSyncTime);

    qDebug() << "TodoSyncServer 已销毁";
}

// 属性访问器实现
bool TodoSyncServer::isAutoSyncEnabled() const {
    return m_isAutoSyncEnabled;
}

void TodoSyncServer::setAutoSyncEnabled(bool enabled) {
    if (m_isAutoSyncEnabled != enabled) {
        m_isAutoSyncEnabled = enabled;
        m_setting.save(QStringLiteral("sync/autoSyncEnabled"), enabled);

        if (enabled) {
            startAutoSyncTimer();
        } else {
            stopAutoSyncTimer();
        }

        emit autoSyncEnabledChanged();
        qDebug() << "自动同步" << (enabled ? "已启用" : "已禁用");
    }
}

bool TodoSyncServer::isSyncing() const {
    return m_isSyncing;
}

QString TodoSyncServer::lastSyncTime() const {
    return m_lastSyncTime;
}

int TodoSyncServer::autoSyncInterval() const {
    return m_autoSyncInterval;
}

void TodoSyncServer::setAutoSyncInterval(int minutes) {
    if (m_autoSyncInterval != minutes && minutes > 0) {
        m_autoSyncInterval = minutes;
        m_setting.save(QStringLiteral("sync/autoSyncInterval"), minutes);

        // 如果自动同步已启用，重新启动定时器
        if (m_isAutoSyncEnabled) {
            startAutoSyncTimer();
        }

        emit autoSyncIntervalChanged();
        qDebug() << "自动同步间隔已设置为" << minutes << "分钟";
    }
}

// 同步操作实现
void TodoSyncServer::syncWithServer(SyncDirection direction) {
    if (m_isSyncing) {
        qDebug() << "同步操作正在进行中，忽略新的同步请求";
        return;
    }

    if (!canPerformSync()) {
        return;
    }

    m_currentSyncDirection = direction;
    performSync(direction);
}

void TodoSyncServer::cancelSync() {
    if (m_isSyncing) {
        // TODO：这里可以添加取消网络请求的逻辑，需要吗？
        m_isSyncing = false;
        emit syncingChanged();
        emit syncCompleted(NetworkError, "同步已取消");
        qDebug() << "同步操作已取消";
    }
}

void TodoSyncServer::resetSyncState() {
    m_isSyncing = false;
    m_pendingUnsyncedItems.clear();
    m_lastSyncTime.clear();
    m_setting.save(QStringLiteral("sync/lastSyncTime"), QString());

    emit syncingChanged();
    emit lastSyncTimeChanged();
    qDebug() << "同步状态已重置";
}

// 数据操作接口实现
void TodoSyncServer::setTodoItems(const QList<TodoItem *> &items) {
    m_todoItems = items;
    qDebug() << "已设置" << items.size() << "个待办事项用于同步";
}

QList<TodoItem *> TodoSyncServer::getUnsyncedItems() const {
    QList<TodoItem *> unsyncedItems;
    for (TodoItem *item : m_todoItems) {
        if (!item->synced()) {
            unsyncedItems.append(item);
        }
    }
    return unsyncedItems;
}

void TodoSyncServer::markItemAsSynced(TodoItem *item) {
    if (item) {
        item->setSynced(true);
    }
}

void TodoSyncServer::markItemAsUnsynced(TodoItem *item) {
    if (item) {
        item->setSynced(false);
    }
}

// 配置管理实现
void TodoSyncServer::updateServerConfig(const QString &baseUrl, const QString &apiEndpoint) {
    bool changed = false;

    if (m_serverBaseUrl != baseUrl) {
        m_serverBaseUrl = baseUrl;
        m_setting.save(QStringLiteral("server/baseUrl"), baseUrl);
        changed = true;
    }

    if (m_todoApiEndpoint != apiEndpoint) {
        m_todoApiEndpoint = apiEndpoint;
        m_setting.save(QStringLiteral("server/todoApiEndpoint"), apiEndpoint);
        changed = true;
    }

    if (changed) {
        emit serverConfigChanged();
        qDebug() << "服务器配置已更新:";
        qDebug() << "  基础URL:" << m_serverBaseUrl;
        qDebug() << "  待办事项API端点:" << m_todoApiEndpoint;
    }
}

QString TodoSyncServer::getServerBaseUrl() const {
    return m_serverBaseUrl;
}

QString TodoSyncServer::getApiEndpoint() const {
    return m_todoApiEndpoint;
}

// 网络请求回调处理
void TodoSyncServer::onNetworkRequestCompleted(NetworkRequest::RequestType type, const QJsonObject &response) {
    switch (type) {
    case NetworkRequest::RequestType::Sync:
        handleSyncSuccess(response);
        break;
    case NetworkRequest::RequestType::FetchTodos:
        handleFetchTodosSuccess(response);
        break;
    case NetworkRequest::RequestType::PushTodos:
        handlePushChangesSuccess(response);
        break;
    default:
        // 其他类型的请求不在同步管理器的处理范围内
        break;
    }
}

void TodoSyncServer::onNetworkRequestFailed(NetworkRequest::RequestType type, NetworkRequest::NetworkError error,
                                            const QString &message) {
    QString typeStr;
    SyncResult result = NetworkError;

    switch (type) {
    case NetworkRequest::RequestType::Sync:
        typeStr = "同步";
        break;
    case NetworkRequest::RequestType::FetchTodos:
        typeStr = "获取待办事项";
        break;
    case NetworkRequest::RequestType::PushTodos:
        typeStr = "推送更改";
        break;
    default:
        // 其他类型的请求不在同步管理器的处理范围内
        return;
    }

    // 根据错误类型确定同步结果
    switch (error) {
    case NetworkRequest::NetworkError::AuthenticationError:
        result = AuthError;
        break;
    case NetworkRequest::NetworkError::UnknownError:
        result = NetworkError;
        break;
    default:
        result = UnknownError;
        break;
    }

    m_isSyncing = false;
    emit syncingChanged();
    emit syncCompleted(result, message);

    qWarning() << typeStr << "失败:" << message;
}

void TodoSyncServer::onAutoSyncTimer() {
    if (canPerformSync() && m_isAutoSyncEnabled && !m_isSyncing) {
        qDebug() << "自动同步定时器触发，开始同步";
        syncWithServer(Bidirectional);
    }
}

void TodoSyncServer::onBaseUrlChanged(const QString &newBaseUrl) {
    qDebug() << "服务器基础URL已更新:" << m_serverBaseUrl << "->" << newBaseUrl;
    m_serverBaseUrl = newBaseUrl;
    emit serverConfigChanged();

    // 如果当前启用自动同步且已登录，可以选择重新同步数据
    if (m_isAutoSyncEnabled && UserAuth::GetInstance().isLoggedIn()) {
        // 立即同步
        syncWithServer(Bidirectional);
    }
}

// 同步操作实现
void TodoSyncServer::performSync(SyncDirection direction) {
    qDebug() << "开始同步待办事项，方向:" << direction;

    m_isSyncing = true;
    emit syncingChanged();
    emit syncStarted();

    // 根据同步方向执行不同的操作
    switch (direction) {
    case Bidirectional:
        // 双向同步：先获取服务器数据，然后推送本地更改
        fetchTodosFromServer();
        break;
    case UploadOnly:
        // 仅上传：只推送本地更改
        pushLocalChangesToServer();
        break;
    case DownloadOnly:
        // 仅下载：只获取服务器数据
        fetchTodosFromServer();
        break;
    }
}

void TodoSyncServer::fetchTodosFromServer() {
    if (!canPerformSync()) {
        m_isSyncing = false;
        emit syncingChanged();
        return;
    }

    qDebug() << "从服务器获取待办事项...";
    emit syncProgress(25, "正在从服务器获取数据...");

    try {
        NetworkRequest::RequestConfig config;
        config.url = getApiUrl(m_todoApiEndpoint);
        config.method = "GET";  // 明确指定使用GET方法
        config.requiresAuth = true;

        m_networkRequest.sendRequest(NetworkRequest::RequestType::FetchTodos, config);
    } catch (const std::exception &e) {
        qCritical() << "获取服务器数据时发生异常:" << e.what();

        m_isSyncing = false;
        emit syncingChanged();
        emit syncCompleted(UnknownError, QString("获取服务器数据失败: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "获取服务器数据时发生未知异常";

        m_isSyncing = false;
        emit syncingChanged();
        emit syncCompleted(UnknownError, "获取服务器数据失败：未知错误");
    }
}

void TodoSyncServer::pushLocalChangesToServer() {
    if (!canPerformSync()) {
        m_isSyncing = false;
        emit syncingChanged();
        return;
    }

    // 找出所有未同步的项目
    QList<TodoItem *> unsyncedItems = getUnsyncedItems();

    if (unsyncedItems.isEmpty()) {
        qDebug() << "没有需要同步的项目";

        // 如果是双向同步且没有本地更改，直接完成
        if (m_currentSyncDirection == Bidirectional || m_currentSyncDirection == UploadOnly) {
            m_isSyncing = false;
            emit syncingChanged();
            updateLastSyncTime();
            emit syncCompleted(Success, "同步完成");
        }
        return;
    }

    qDebug() << "推送" << unsyncedItems.size() << "个项目到服务器";
    
    // 服务器限制：一次最多同步100个项目
    const int maxBatchSize = 100;
    
    if (unsyncedItems.size() <= maxBatchSize) {
        // 项目数量在限制范围内，直接推送
        m_pendingUnsyncedItems = unsyncedItems;
        pushBatchToServer(unsyncedItems);
    } else {
        // 项目数量超过限制，需要分批推送
        qDebug() << "项目数量超过" << maxBatchSize << "个，将分批推送";
        m_allUnsyncedItems = unsyncedItems;
        m_currentBatchIndex = 0;
        m_totalBatches = (unsyncedItems.size() + maxBatchSize - 1) / maxBatchSize;
        
        // 开始推送第一批
        pushNextBatch();
    }
}

void TodoSyncServer::pushBatchToServer(const QList<TodoItem *> &batch) {
    emit syncProgress(75, QString("正在推送 %1 个更改到服务器...").arg(batch.size()));

    try {
        // 创建一个包含当前批次项目的JSON数组
        QJsonArray jsonArray;
        for (TodoItem *item : std::as_const(batch)) {
            QJsonObject obj;
            obj["id"] = item->id();
            obj["uuid"] = item->uuid().toString();
            obj["user_uuid"] = item->userUuid().toString();
            obj["title"] = item->title();
            obj["description"] = item->description();
            obj["category"] = item->category();
            obj["important"] = item->important();
            obj["deadline"] = item->deadline().toString(Qt::ISODate);
            obj["recurrence_interval"] = item->recurrenceInterval();
            obj["recurrence_count"] = item->recurrenceCount();
            obj["recurrence_start_date"] = item->recurrenceStartDate().toString(Qt::ISODate);
            obj["is_completed"] = item->isCompleted();
            obj["completed_at"] = item->completedAt().toString(Qt::ISODate);
            obj["is_deleted"] = item->isDeleted();
            obj["deleted_at"] = item->deletedAt().toString(Qt::ISODate);
            obj["created_at"] = item->createdAt().toString(Qt::ISODate);
            obj["updated_at"] = item->updatedAt().toString(Qt::ISODate);
            obj["last_modified_at"] = item->lastModifiedAt().toString(Qt::ISODate);

            jsonArray.append(obj);
        }

        NetworkRequest::RequestConfig config;
        config.url = getApiUrl(m_todoApiEndpoint);
        config.method = "POST";  // 批量推送使用POST方法
        config.requiresAuth = true;
        config.data["todos"] = jsonArray;

        // 存储当前批次的未同步项目引用，用于成功后标记为已同步
        m_pendingUnsyncedItems = batch;

        m_networkRequest.sendRequest(NetworkRequest::RequestType::PushTodos, config);
    } catch (const std::exception &e) {
        qCritical() << "推送更改时发生异常:" << e.what();

        m_isSyncing = false;
        emit syncingChanged();
        emit syncCompleted(UnknownError, QString("推送更改失败: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "推送更改时发生未知异常";

        m_isSyncing = false;
        emit syncingChanged();
        emit syncCompleted(UnknownError, "推送更改失败：未知错误");
    }
}

void TodoSyncServer::pushNextBatch() {
    const int maxBatchSize = 100;
    int startIndex = m_currentBatchIndex * maxBatchSize;
    int endIndex = qMin(startIndex + maxBatchSize, m_allUnsyncedItems.size());
    
    if (startIndex >= m_allUnsyncedItems.size()) {
        // 所有批次都已推送完成
        qDebug() << "所有批次推送完成";
        m_isSyncing = false;
        emit syncingChanged();
        updateLastSyncTime();
        emit syncCompleted(Success, QString("分批同步完成，共推送 %1 个项目").arg(m_allUnsyncedItems.size()));
        
        // 清理临时数据
        m_allUnsyncedItems.clear();
        m_currentBatchIndex = 0;
        m_totalBatches = 0;
        return;
    }
    
    // 获取当前批次的项目
    QList<TodoItem *> currentBatch;
    for (int i = startIndex; i < endIndex; ++i) {
        currentBatch.append(m_allUnsyncedItems[i]);
    }
    
    qDebug() << "推送第" << (m_currentBatchIndex + 1) << "批，共" << m_totalBatches << "批，当前批次" << currentBatch.size() << "个项目";
    
    // 推送当前批次
    pushBatchToServer(currentBatch);
}

void TodoSyncServer::handleSyncSuccess(const QJsonObject &response) {
    qDebug() << "同步成功";
    emit syncProgress(100, "同步完成");

    // 处理同步响应数据
    if (response.contains("todos")) {
        QJsonArray todosArray = response["todos"].toArray();
        emit todosUpdatedFromServer(todosArray);
    }

    m_isSyncing = false;
    emit syncingChanged();
    updateLastSyncTime();
    emit syncCompleted(Success, "同步完成");
}

void TodoSyncServer::handleFetchTodosSuccess(const QJsonObject &response) {
    qDebug() << "获取待办事项成功";
    emit syncProgress(50, "数据获取完成，正在处理...");

    if (response.contains("todos")) {
        QJsonArray todosArray = response["todos"].toArray();
        emit todosUpdatedFromServer(todosArray);
    }

    // 如果是双向同步，成功获取数据后推送本地更改
    if (m_currentSyncDirection == Bidirectional) {
        pushLocalChangesToServer();
    } else {
        // 仅下载模式，直接完成同步
        m_isSyncing = false;
        emit syncingChanged();
        updateLastSyncTime();
        emit syncCompleted(Success, "数据获取完成");
    }
}

void TodoSyncServer::handlePushChangesSuccess(const QJsonObject &response) {
    qDebug() << "推送更改成功";
    
    // 标记当前批次的项目为已同步
    for (TodoItem *item : m_pendingUnsyncedItems) {
        item->setSynced(true);
    }

    // 发出本地更改已上传信号
    emit localChangesUploaded(m_pendingUnsyncedItems);

    // 处理推送响应
    if (response.contains("updated_count")) {
        int updatedCount = response["updated_count"].toInt();
        qDebug() << "已更新" << updatedCount << "个待办事项";
    }
    
    // 检查是否还有更多批次需要推送
    if (!m_allUnsyncedItems.isEmpty() && m_currentBatchIndex < m_totalBatches - 1) {
        // 还有更多批次需要推送
        m_currentBatchIndex++;
        
        // 更新进度
        int progress = 75 + (20 * m_currentBatchIndex / m_totalBatches);
        emit syncProgress(progress, QString("正在推送第 %1/%2 批...").arg(m_currentBatchIndex + 1).arg(m_totalBatches));
        
        // 清空当前批次的待同步项目列表
        m_pendingUnsyncedItems.clear();
        
        // 推送下一批
        pushNextBatch();
    } else {
        // 所有批次都已完成或这是单批次推送
        emit syncProgress(100, "更改推送完成");
        
        // 清空待同步项目列表
        m_pendingUnsyncedItems.clear();
        
        if (!m_allUnsyncedItems.isEmpty()) {
            // 分批推送完成
            qDebug() << "所有批次推送完成，共" << m_allUnsyncedItems.size() << "个项目";
            m_allUnsyncedItems.clear();
            m_currentBatchIndex = 0;
            m_totalBatches = 0;
        }
        
        m_isSyncing = false;
        emit syncingChanged();
        updateLastSyncTime();
        emit syncCompleted(Success, "更改推送完成");
    }
}

// 辅助方法实现
void TodoSyncServer::initializeServerConfig() {
    // 从设置中读取服务器配置，如果不存在则使用默认值
    m_serverBaseUrl = m_setting.get(QStringLiteral("server/baseUrl"), QString::fromStdString(std::string{DefaultValues::baseUrl})).toString();
    m_todoApiEndpoint = m_setting.get(QStringLiteral("server/todoApiEndpoint"), QString::fromStdString(std::string{DefaultValues::todoApiEndpoint})).toString();

    qDebug() << "服务器配置 - 基础URL:" << m_serverBaseUrl << ", 待办事项API:" << m_todoApiEndpoint;
}

QString TodoSyncServer::getApiUrl(const QString &endpoint) const {
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

void TodoSyncServer::updateLastSyncTime() {
    m_lastSyncTime = QDateTime::currentDateTime().toString(Qt::ISODate);
    m_setting.save(QStringLiteral("sync/lastSyncTime"), m_lastSyncTime);
    emit lastSyncTimeChanged();
}

bool TodoSyncServer::canPerformSync() const {
    if (!UserAuth::GetInstance().isLoggedIn()) {
        qDebug() << "无法同步：未登录";
        return false;
    }

    return true;
}

void TodoSyncServer::startAutoSyncTimer() {
    if (m_autoSyncTimer->isActive()) {
        m_autoSyncTimer->stop();
    }

    int intervalMs = m_autoSyncInterval * 60 * 1000; // 转换为毫秒
    m_autoSyncTimer->start(intervalMs);

    qDebug() << "自动同步定时器已启动，间隔:" << m_autoSyncInterval << "分钟";
}

void TodoSyncServer::stopAutoSyncTimer() {
    if (m_autoSyncTimer->isActive()) {
        m_autoSyncTimer->stop();
        qDebug() << "自动同步定时器已停止";
    }
}

void TodoSyncServer::pushSingleItem(TodoItem *item) {
    if (!item) {
        // 如果项目无效，跳到下一个
        pushNextItem();
        return;
    }

    qDebug() << "推送单个项目到服务器:" << item->title();

    NetworkRequest::RequestConfig config;
    config.url = getApiUrl(m_todoApiEndpoint);
    config.requiresAuth = true;

    // 准备项目数据
    QJsonObject itemData;
    itemData["title"] = item->title();
    itemData["description"] = item->description();
    itemData["category"] = item->category();
    itemData["important"] = item->important();
    itemData["is_completed"] = item->isCompleted();
    
    // 处理可选的日期时间字段
    if (item->deadline().isValid()) {
        itemData["deadline"] = item->deadline().toString(Qt::ISODate);
    }
    if (item->recurrenceInterval() > 0) {
        itemData["recurrence_interval"] = item->recurrenceInterval();
        itemData["recurrence_count"] = item->recurrenceCount();
        if (item->recurrenceStartDate().isValid()) {
            itemData["recurrence_start_date"] = item->recurrenceStartDate().toString(Qt::ISODate);
        }
    }

    // 根据项目状态决定使用的HTTP方法
    if (item->id() > 0) {
        // 已存在的项目，使用PATCH更新
        config.method = "PATCH";
        itemData["id"] = item->id();
    } else {
        // 新项目，使用POST创建
        config.method = "POST";
    }

    config.data = itemData;
    m_networkRequest.sendRequest(NetworkRequest::RequestType::PushTodos, config);
}

void TodoSyncServer::handleSingleItemPushSuccess(const QJsonObject &response) {
    qDebug() << "单个项目推送成功";
    
    // 标记当前项目为已同步
    if (m_currentPushIndex < m_pendingUnsyncedItems.size()) {
        TodoItem *item = m_pendingUnsyncedItems[m_currentPushIndex];
        if (item) {
            // TODO: 标记项目为已同步的逻辑
            // item->setIsSynced(true);
        }
    }

    // 推送下一个项目
    pushNextItem();
}

void TodoSyncServer::pushNextItem() {
    m_currentPushIndex++;
    
    if (m_currentPushIndex < m_pendingUnsyncedItems.size()) {
        // 还有更多项目需要推送
        TodoItem *nextItem = m_pendingUnsyncedItems[m_currentPushIndex];
        pushSingleItem(nextItem);
        
        // 更新进度
        int progress = 75 + (25 * m_currentPushIndex / m_pendingUnsyncedItems.size());
        emit syncProgress(progress, QString("正在推送项目 %1/%2...").arg(m_currentPushIndex + 1).arg(m_pendingUnsyncedItems.size()));
    } else {
        // 所有项目都已推送完成
        qDebug() << "所有项目推送完成";
        
        m_isSyncing = false;
        emit syncingChanged();
        updateLastSyncTime();
        emit syncCompleted(Success, "同步完成");
        
        // 清理临时数据
        m_pendingUnsyncedItems.clear();
        m_currentPushIndex = 0;
    }
}