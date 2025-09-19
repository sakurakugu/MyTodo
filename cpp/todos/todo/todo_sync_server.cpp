/**
 * @file todo_sync_server.cpp
 * @brief TodoSyncServer类的实现文件
 *
 * 该文件实现了TodoSyncServer类的所有方法，负责待办事项的服务器同步功能。
 *
 * @author Sakurakugu
 * @date 2025-08-24 23:07:18(UTC+8) 周日
 * @change 2025-09-06 01:29:53(UTC+8) 周六
 * @version 0.4.0
 */

#include "todo_sync_server.h"
#include "default_value.h"
#include "user_auth.h"

#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUuid>

TodoSyncServer::TodoSyncServer(QObject *parent)
    : BaseSyncServer(parent), //
      m_currentPushIndex(0),  //
      m_currentBatchIndex(0), //
      m_totalBatches(0)       //
{
    // 设置待办事项特有的API端点
    m_apiEndpoint = m_setting
                        .get(QStringLiteral("server/todoApiEndpoint"),
                             QString::fromStdString(std::string{DefaultValues::todoApiEndpoint}))
                        .toString();
}

TodoSyncServer::~TodoSyncServer() {
}

// 同步操作实现 - 重写基类方法
void TodoSyncServer::与服务器同步(SyncDirection direction) {
    qDebug() << "开始同步待办事项，方向:" << direction;
    qDebug() << "同步请求前状态检查: m_isSyncing =" << m_isSyncing;

    // 防止并发同步请求 - 优先检查
    if (m_isSyncing) {
        qWarning() << "同步已在进行中，忽略重复请求";
        emit syncCompleted(UnknownError, "同步已在进行中");
        return;
    }

    // 检查基本同步条件（不包括 m_isSyncing 状态）
    if (m_serverBaseUrl.isEmpty()) {
        qDebug() << "同步检查失败：服务器基础URL为空";
        emit syncCompleted(UnknownError, "服务器配置错误");
        return;
    }

    if (m_apiEndpoint.isEmpty()) {
        qDebug() << "同步检查失败：API端点为空";
        emit syncCompleted(UnknownError, "服务器配置错误");
        return;
    }

    // 检查用户登录状态
    if (!UserAuth::GetInstance().isLoggedIn()) {
        qDebug() << "同步检查失败：用户未登录或令牌已过期";
        emit syncCompleted(AuthError, "无法同步：未登录");
        return;
    }

    m_isSyncing = true;
    m_currentSyncDirection = direction;
    emit syncingChanged();
    emit syncStarted();

    执行同步(direction);
}

void TodoSyncServer::取消同步() {
    if (!m_isSyncing) {
        qDebug() << "没有正在进行的同步操作可以取消";
        return;
    }

    qDebug() << "取消待办事项同步操作";

    // 调用基类方法
    BaseSyncServer::取消同步();

    // 清理待办事项特有的状态
    m_pendingUnsyncedItems.clear();
    m_allUnsyncedItems.clear();
    m_currentPushIndex = 0;
    m_currentBatchIndex = 0;
    m_totalBatches = 0;
}

void TodoSyncServer::重置同步状态() {
    // 调用基类方法
    BaseSyncServer::重置同步状态();

    // 清理待办事项特有的状态
    m_pendingUnsyncedItems.clear();
    m_currentPushIndex = 0;
    m_currentBatchIndex = 0;
    m_totalBatches = 0;
    m_allUnsyncedItems.clear();
}

// 数据操作接口实现
void TodoSyncServer::setTodoItems(const QList<TodoItem *> &items) {
    m_todoItems = items;
    qDebug() << "已设置" << items.size() << "个待办事项用于同步";
}

QList<TodoItem *> TodoSyncServer::getUnsyncedItems() const {
    QList<TodoItem *> unsyncedItems;
    int totalItems = 0;
    int syncedItems = 0;

    for (TodoItem *item : m_todoItems) {
        totalItems++;
        if (!item->synced()) {
            unsyncedItems.append(item);
        } else {
            syncedItems++;
        }
    }

    qDebug() << QString("同步状态检查: 总计=%1, 已同步=%2, 未同步=%3")
                    .arg(totalItems)
                    .arg(syncedItems)
                    .arg(unsyncedItems.size());

    // 打印前5个未同步项目的详细信息
    for (int i = 0; i < qMin(5, unsyncedItems.size()); i++) {
        TodoItem *item = unsyncedItems[i];
        qDebug() << QString("未同步项目 %1: ID=%2, 标题='%3', synced=%4")
                        .arg(i + 1)
                        .arg(item->id())
                        .arg(item->title())
                        .arg(item->synced());
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

// 网络请求回调处理 - 重写基类方法
void TodoSyncServer::onNetworkRequestCompleted(NetworkRequest::RequestType type, const QJsonObject &response) {
    switch (type) {
    case NetworkRequest::RequestType::FetchTodos:
        handleFetchTodosSuccess(response);
        break;
    case NetworkRequest::RequestType::PushTodos:
        handlePushChangesSuccess(response);
        break;
    default:
        // 调用基类的默认处理
        BaseSyncServer::onNetworkRequestCompleted(type, response);
        break;
    }
}

void TodoSyncServer::onNetworkRequestFailed(NetworkRequest::RequestType type, NetworkRequest::NetworkError error,
                                            const QString &message) {
    if (type == NetworkRequest::RequestType::PushTodos) {
        qInfo() << "项目推送失败！错误类型:" << static_cast<int>(error);
        qInfo() << "失败详情:" << message;
        qInfo() << "当前推送索引:" << m_currentPushIndex;
    }

    // 调用基类的默认处理
    BaseSyncServer::onNetworkRequestFailed(type, error, message);
}

// 同步操作实现 - 重写基类虚函数
void TodoSyncServer::执行同步(SyncDirection direction) {
    qDebug() << "开始同步待办事项，方向:" << direction;

    // 根据同步方向执行不同的操作
    switch (direction) {
    case Bidirectional:
        // 双向同步：先获取服务器数据，然后在handleFetchTodosSuccess中推送本地更改
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
    qDebug() << "从服务器获取待办事项...";
    emit syncProgress(25, "正在从服务器获取数据...");

    try {
        NetworkRequest::RequestConfig config;
        config.url = getApiUrl(m_apiEndpoint);
        config.method = "GET"; // 明确指定使用GET方法
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
    qInfo() << "开始推送本地更改到服务器...";

    // 找出所有未同步的项目
    QList<TodoItem *> unsyncedItems = getUnsyncedItems();
    qInfo() << "检测到" << unsyncedItems.size() << "个未同步的项目";

    if (unsyncedItems.isEmpty()) {
        qInfo() << "没有需要同步的项目，上传流程完成";

        // 如果是双向同步且没有本地更改，直接完成
        if (m_currentSyncDirection == Bidirectional || m_currentSyncDirection == UploadOnly) {
            m_isSyncing = false;
            emit syncingChanged();
            updateLastSyncTime();
            emit syncCompleted(Success, "同步完成");
        }
        return;
    }

    qInfo() << "开始推送" << unsyncedItems.size() << "个项目到服务器";

    // 服务器限制：一次最多同步100个项目
    const int maxBatchSize = 100;
    qInfo() << "服务器批量限制: 最多" << maxBatchSize << "个项目/批次";

    if (unsyncedItems.size() <= maxBatchSize) {
        // 项目数量在限制范围内，直接推送
        qInfo() << "项目数量在限制范围内，使用单批次推送";
        m_pendingUnsyncedItems = unsyncedItems;
        pushBatchToServer(unsyncedItems);
    } else {
        // 项目数量超过限制，需要分批推送
        qInfo() << "项目数量超过限制，需要分批推送";
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
            obj["uuid"] = item->uuid().toString(QUuid::WithoutBraces);
            obj["user_uuid"] = item->userUuid().toString(QUuid::WithoutBraces);
            obj["title"] = item->title();
            obj["description"] = item->description();
            obj["category"] = item->category();
            obj["important"] = item->important();
            obj["deadline"] = item->deadline().toString(Qt::ISODate);
            obj["recurrenceInterval"] = item->recurrenceInterval();
            obj["recurrenceCount"] = item->recurrenceCount();
            obj["recurrenceStartDate"] = item->recurrenceStartDate().toString(Qt::ISODate);
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
        config.url = getApiUrl(m_apiEndpoint);
        config.method = "POST"; // 批量推送使用POST方法
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

    qDebug() << "推送第" << (m_currentBatchIndex + 1) << "批，共" << m_totalBatches << "批，当前批次"
             << currentBatch.size() << "个项目";

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
        // 检查是否有未同步的项目
        QList<TodoItem *> unsyncedItems = getUnsyncedItems();
        if (unsyncedItems.isEmpty()) {
            // 没有本地更改需要推送，直接完成同步
            qInfo() << "双向同步：没有本地更改需要推送，同步完成";
            m_isSyncing = false;
            emit syncingChanged();
            updateLastSyncTime();
            emit syncCompleted(Success, "双向同步完成");
        } else {
            // 有本地更改需要推送
            qInfo() << "双向同步：检测到" << unsyncedItems.size() << "个本地更改，开始推送";
            pushLocalChangesToServer();
        }
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

    // 验证服务器响应
    bool shouldMarkAsSynced = true;
    if (response.contains("summary")) {
        QJsonObject summary = response["summary"].toObject();
        int created = summary["created"].toInt();
        int updated = summary["updated"].toInt();
        int errors = summary["errors"].toArray().size();

        qInfo() << QString("服务器处理结果: 创建=%1, 更新=%2, 错误=%3").arg(created).arg(updated).arg(errors);

        // 如果有错误，记录详细信息
        if (errors > 0) {
            QJsonArray errorArray = summary["errors"].toArray();
            for (const auto &errorValue : errorArray) {
                QJsonObject error = errorValue.toObject();
                qWarning()
                    << QString("项目 %1 处理失败: %2").arg(error["index"].toInt()).arg(error["error"].toString());
            }
            shouldMarkAsSynced = false;
            qWarning() << "由于服务器处理错误，不标记项目为已同步";
        }
    } else {
        // 兼容旧的响应格式
        qWarning() << "服务器响应格式不标准，假设操作成功";
        if (response.contains("updated_count")) {
            int updatedCount = response["updated_count"].toInt();
            qDebug() << "已更新" << updatedCount << "个待办事项";
        }
    }

    // 只有在验证通过时才标记当前批次的项目为已同步
    if (shouldMarkAsSynced) {
        for (TodoItem *item : m_pendingUnsyncedItems) {
            item->setSynced(true);
        }
        // 发出本地更改已上传信号
        emit localChangesUploaded(m_pendingUnsyncedItems);
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

// 辅助方法已在基类BaseSyncServer中实现

void TodoSyncServer::pushSingleItem(TodoItem *item) {
    if (!item) {
        // 如果项目无效，跳到下一个
        qInfo() << "跳过无效项目，继续推送下一个";
        pushNextItem();
        return;
    }

    qInfo() << "开始推送项目到服务器:" << item->title() << "(ID:" << item->id() << ")";

    NetworkRequest::RequestConfig config;
    config.url = getApiUrl(m_apiEndpoint);
    config.requiresAuth = true;

    // 准备项目数据
    QJsonObject itemData;
    itemData["uuid"] = item->uuid().toString(QUuid::WithoutBraces);
    itemData["user_uuid"] = item->userUuid().toString(QUuid::WithoutBraces);
    itemData["title"] = item->title();
    itemData["description"] = item->description();
    itemData["category"] = item->category();
    itemData["important"] = item->important();
    itemData["is_completed"] = item->isCompleted();

    // 处理可选的日期时间字段
    if (item->deadline().isValid()) {
        itemData["deadline"] = item->deadline().date().toString(Qt::ISODate);
    }
    if (item->recurrenceInterval() > 0) {
        itemData["recurrenceInterval"] = item->recurrenceInterval();
        itemData["recurrenceCount"] = item->recurrenceCount();
        if (item->recurrenceStartDate().isValid()) {
            itemData["recurrenceStartDate"] = item->recurrenceStartDate().toString(Qt::ISODate);
        }
    }

    // 根据项目状态决定使用的HTTP方法
    if (item->id() > 0) {
        // 已存在的项目，使用PATCH更新
        config.method = "PATCH";
        itemData["id"] = item->id();
        qInfo() << "使用PATCH方法更新已存在项目，ID:" << item->id();
    } else {
        // 新项目，使用POST创建
        config.method = "POST";
        qInfo() << "使用POST方法创建新项目:" << item->title();
    }

    config.data = itemData;
    qInfo() << "发送请求到API端点:" << config.url;
    qInfo() << "请求方法:" << config.method;
    qInfo() << "项目数据:" << QJsonDocument(itemData).toJson(QJsonDocument::Compact);

    m_networkRequest.sendRequest(NetworkRequest::RequestType::PushTodos, config);
    qInfo() << "项目推送请求已发送，等待服务器响应...";
}

void TodoSyncServer::handleSingleItemPushSuccess() {
    qInfo() << "单个项目推送成功！";

    // 标记当前项目为已同步
    if (m_currentPushIndex < m_pendingUnsyncedItems.size()) {
        TodoItem *item = m_pendingUnsyncedItems[m_currentPushIndex];
        if (item) {
            item->setSynced(true);
        }
    }

    // 推送下一个项目
    qInfo() << "继续推送队列中的下一个项目...";
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
        emit syncProgress(
            progress, QString("正在推送项目 %1/%2...").arg(m_currentPushIndex + 1).arg(m_pendingUnsyncedItems.size()));
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