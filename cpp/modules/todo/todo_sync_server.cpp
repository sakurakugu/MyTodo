/**
 * @file todo_sync_server.cpp
 * @brief TodoSyncServer类的实现文件
 *
 * 该文件实现了TodoSyncServer类的所有方法，负责待办事项的服务器同步功能。
 *
 * @author Sakurakugu
 * @date 2025-08-24 23:07:18(UTC+8) 周日
 * @change 2025-10-06 02:13:23(UTC+8) 周一
 */

#include "todo_sync_server.h"
#include "config.h"
#include "default_value.h"
#include "todo_item.h"
#include "utility.h"

#include <QJsonDocument>
#include <QUuid>

TodoSyncServer::TodoSyncServer(UserAuth &userAuth, QObject *parent)
    : BaseSyncServer(userAuth, parent), //
      m_currentPushIndex(0),            //
      m_currentBatchIndex(0),           //
      m_totalBatches(0)                 //
{
    // 设置待办事项特有的API端点
    m_apiEndpoint = m_config.get("server/todoApiEndpoint", QString(DefaultValues::todoApiEndpoint)).toString();
}

TodoSyncServer::~TodoSyncServer() {}

// 同步操作实现 - 重写基类方法
void TodoSyncServer::与服务器同步(SyncDirection direction) {
    qDebug() << "开始同步待办事项，方向:" << direction;
    qDebug() << "同步请求前状态检查: m_isSyncing =" << m_isSyncing;

    // 入口调用：严格检查（不允许已有同步）
    检查同步前置条件(false);

    setIsSyncing(true);
    m_currentSyncDirection = direction;
    emit syncStarted();

    执行同步(direction);
}

void TodoSyncServer::取消同步() {
    if (!m_isSyncing)
        return;

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
        if (item->synced() > 0) {
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

// 网络请求回调处理 - 重写基类方法
void TodoSyncServer::onNetworkRequestCompleted(Network::RequestType type, const QJsonObject &response) {
    switch (type) {
    case Network::RequestType::FetchTodos:
        handleFetchTodosSuccess(response);
        break;
    case Network::RequestType::PushTodos:
        处理推送更改成功(response);
        break;
    default:
        // 调用基类的默认处理
        BaseSyncServer::onNetworkRequestCompleted(type, response);
        break;
    }
}

void TodoSyncServer::onNetworkRequestFailed(Network::RequestType type, NetworkRequest::NetworkError error,
                                            const QString &message) {
    if (type == Network::RequestType::PushTodos) {
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
        // 若存在本地未同步更改，先推送再拉取，避免拉取阶段把旧名称再插入成重复
        if (!getUnsyncedItems().isEmpty()) {
            m_pushFirstInBidirectional = true;
            推送待办();
        } else {
            // 没有本地更改仍按旧逻辑直接拉取
            m_pushFirstInBidirectional = false;
            拉取待办();
        }
        break;
    case UploadOnly:
        // 仅上传：只推送本地更改
        推送待办();
        break;
    case DownloadOnly:
        // 仅下载：只获取服务器数据
        拉取待办();
        break;
    }
}

void TodoSyncServer::拉取待办() {
    qDebug() << "从服务器获取待办事项...";
    emit syncProgress(25, "正在从服务器获取数据...");

    try {
        NetworkRequest::RequestConfig config;
        config.url = m_networkRequest.getApiUrl(m_apiEndpoint);
        config.method = "GET"; // 明确指定使用GET方法
        config.requiresAuth = true;

        m_networkRequest.sendRequest(Network::RequestType::FetchTodos, config);
    } catch (const std::exception &e) {
        qCritical() << "获取服务器数据时发生异常:" << e.what();

        setIsSyncing(false);
        emit syncCompleted(UnknownError, QString("获取服务器数据失败: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "获取服务器数据时发生未知异常";

        setIsSyncing(false);
        emit syncCompleted(UnknownError, "获取服务器数据失败：未知错误");
    }
}

void TodoSyncServer::推送待办() {
    qInfo() << "开始推送本地更改到服务器...";

    // 第二阶段（双向同步 fetch 之后）允许继续沿用 m_isSyncing，占用状态不视为冲突
    检查同步前置条件(true);

    // 找出所有未同步的项目
    QList<TodoItem *> unsyncedItems = getUnsyncedItems();
    // qInfo() << "检测到" << unsyncedItems.size() << "个未同步的项目";

    if (unsyncedItems.isEmpty()) {
        qInfo() << "没有需要同步的项目，上传流程完成";

        // 如果是双向同步且没有本地更改，直接完成
        if (m_currentSyncDirection == Bidirectional || m_currentSyncDirection == UploadOnly) {
            setIsSyncing(false);
            更新最后同步时间();
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
            obj["uuid"] = item->uuid().toString(QUuid::WithoutBraces);
            obj["user_uuid"] = item->userUuid().toString(QUuid::WithoutBraces);
            obj["title"] = item->title();
            obj["description"] = item->description();
            obj["category"] = item->category();
            obj["important"] = item->important();
            // 统一使用 RFC3339 (UTC, 带毫秒, 末尾 Z) 字符串传递时间，避免 Go 端 time.Time 反序列化失败
            obj["deadline"] = Utility::toRfc3339Json(item->deadline());
            obj["recurrenceInterval"] = item->recurrenceInterval();
            obj["recurrenceCount"] = item->recurrenceCount();
            // recurrenceStartDate 仍保留为仅日期 ISO 字符串（业务语义：重复计划起始日期）
            obj["recurrenceStartDate"] = item->recurrenceStartDate().toString(Qt::ISODate);
            obj["is_completed"] = item->isCompleted();
            obj["completed_at"] = Utility::toRfc3339Json(item->completedAt());
            obj["is_trashed"] = item->isTrashed();
            obj["trashed_at"] = Utility::toRfc3339Json(item->trashedAt());
            obj["created_at"] = Utility::toRfc3339Json(item->createdAt());
            obj["updated_at"] = Utility::toRfc3339Json(item->updatedAt());
            obj["synced"] = item->synced();

            jsonArray.append(obj);
        }

        NetworkRequest::RequestConfig config;
        config.url = m_networkRequest.getApiUrl(m_apiEndpoint);
        config.method = "POST"; // 批量推送使用POST方法
        config.requiresAuth = true;
        config.data["todos"] = jsonArray;

        // 存储当前批次的未同步项目引用，用于成功后标记为已同步
        m_pendingUnsyncedItems = batch;

        m_networkRequest.sendRequest(Network::RequestType::PushTodos, config);
    } catch (const std::exception &e) {
        qCritical() << "推送更改时发生异常:" << e.what();
        setIsSyncing(false);
        emit syncCompleted(UnknownError, QString("推送更改失败: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "推送更改时发生未知异常";
        setIsSyncing(false);
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
        setIsSyncing(false);
        更新最后同步时间();
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

    setIsSyncing(false);
    更新最后同步时间();
    emit syncCompleted(Success, "同步完成");
}

void TodoSyncServer::handleFetchTodosSuccess(const QJsonObject &response) {
    qDebug() << "获取待办事项成功";
    emit syncProgress(50, "数据获取完成，正在处理...");

    if (response.contains("todos")) {
        QJsonArray todosArray = response["todos"].toArray();
        emit todosUpdatedFromServer(todosArray);
    }

    // 如果是双向同步，成功获取数据后推送本地更改；但当 push-first 策略启用时，此处不再推送
    if (m_currentSyncDirection == Bidirectional && !m_pushFirstInBidirectional) {
        // 检查是否有未同步的项目
        QList<TodoItem *> unsyncedItems = getUnsyncedItems();
        if (unsyncedItems.isEmpty()) {
            // 没有本地更改需要推送，直接完成同步
            qInfo() << "双向同步：没有本地更改需要推送，同步完成";
            setIsSyncing(false);
            更新最后同步时间();
            emit syncCompleted(Success, "双向同步完成");
        } else {
            // 有本地更改需要推送
            qInfo() << "双向同步：检测到" << unsyncedItems.size() << "个本地更改，开始推送";
            推送待办();
        }
    } else {
        // 仅下载模式，直接完成同步
        setIsSyncing(false);
        更新最后同步时间();
        emit syncCompleted(Success, "数据获取完成");
    }
}

void TodoSyncServer::处理推送更改成功(const QJsonObject &response) {
    qDebug() << "推送更改成功";

    // 验证服务器响应
    bool shouldMarkAsSynced = true;

    QJsonObject summary = response["summary"].toObject();
    int created = summary.value("created").toInt();
    int updated = summary.value("updated").toInt();
    // 新结构: conflicts, error_count, conflict_details
    int conflicts = summary.contains("conflicts") ? summary.value("conflicts").toInt() : 0;
    int errorCount = 0;
    if (summary.contains("error_count")) {
        errorCount = summary.value("error_count").toInt();
    } else {
        // 兼容旧结构: 通过 errors 数组大小统计
        errorCount = summary.value("errors").toArray().size();
    }

    qInfo() << QString("服务器处理结果: 创建=%1, 更新=%2, 冲突=%3, 错误=%4")
                   .arg(created)
                   .arg(updated)
                   .arg(conflicts)
                   .arg(errorCount);

    // 记录冲突详情（如果有）
    if (conflicts > 0) {
        QJsonArray conflictArray = summary.value("conflict_details").toArray();
        int idx = 0;
        for (const auto &cVal : conflictArray) {
            QJsonObject cObj = cVal.toObject();
            qWarning() << QString("冲突 %1: index=%2, reason=%3, server_version=%4")
                              .arg(++idx)
                              .arg(cObj.value("index").toInt())
                              .arg(cObj.value("reason").toString())
                              .arg(QString::fromUtf8(
                                  QJsonDocument(cObj.value("server_item").toObject()).toJson(QJsonDocument::Compact)));
        }
    }

    // 如果有错误，记录详细信息
    if (errorCount > 0) {
        QJsonArray errorArray = summary.value("errors").toArray();
        int idx = 0;
        for (const auto &errorValue : errorArray) {
            QJsonObject error = errorValue.toObject();
            qWarning() << QString("错误 %1: 项目序号=%2, 描述=%3")
                              .arg(++idx)
                              .arg(error.value("index").toInt())
                              .arg(error.value("error").toString());
        }
    }

    if (conflicts > 0 || errorCount > 0) {
        shouldMarkAsSynced = false;
        qWarning() << "由于存在" << (conflicts > 0 ? "冲突" : "") << ((conflicts > 0 && errorCount > 0) ? "和" : "")
                   << (errorCount > 0 ? "错误" : "") << "，不标记项目为已同步";
    }

    // 只有在验证通过时才标记当前批次的项目为已同步
    if (shouldMarkAsSynced) {
        for (TodoItem *item : m_pendingUnsyncedItems) {
            if (item->synced() != 3)
                item->setSynced(0);
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

        if (m_currentSyncDirection == Bidirectional && m_pushFirstInBidirectional) {
            qDebug() << "推送阶段完成（push-first），继续执行拉取阶段";
            // 清空标志，避免递归
            m_pushFirstInBidirectional = false;
            // 继续拉取（此时保持 m_isSyncing=true 防止外部再触发）
            拉取待办();
            return;
        } else {
            setIsSyncing(false);
            更新最后同步时间();
            emit syncCompleted(Success, "待办事项更改推送完成");
        }
    }
}

void TodoSyncServer::推送单个项目(TodoItem *item) {
    if (!item) {
        // 如果项目无效，跳到下一个
        qInfo() << "跳过无效项目，继续推送下一个";
        推送下个项目();
        return;
    }

    qInfo() << "开始推送项目到服务器:" << item->title() << "(ID:" << item->id() << ")";

    NetworkRequest::RequestConfig config;
    config.url = m_networkRequest.getApiUrl(m_apiEndpoint);
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
    itemData["deadline"] = item->deadline().isValid()
                               ? QJsonValue(static_cast<qint64>(item->deadline().toUTC().toMSecsSinceEpoch()))
                               : QJsonValue();

    if (item->recurrenceInterval() > 0) {
        itemData["recurrenceInterval"] = item->recurrenceInterval();
        itemData["recurrenceCount"] = item->recurrenceCount();
        if (item->recurrenceStartDate().isValid()) {
            itemData["recurrenceStartDate"] = item->recurrenceStartDate().toString(Qt::ISODate); // 仍使用日期
        }
    }

    // 根据项目状态决定使用的HTTP方法
    if (item->id() > 0) {
        // 已存在的项目，使用PATCH更新
        config.method = "PATCH";
        itemData["uuid"] = item->uuid().toString(QUuid::WithoutBraces);
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

    m_networkRequest.sendRequest(Network::RequestType::PushTodos, config);
    qInfo() << "项目推送请求已发送，等待服务器响应...";
}

void TodoSyncServer::handleSingleItemPushSuccess() {
    qInfo() << "单个项目推送成功！";

    // 标记当前项目为已同步
    if (m_currentPushIndex < m_pendingUnsyncedItems.size()) {
        TodoItem *item = m_pendingUnsyncedItems[m_currentPushIndex];
        if (item) {
            item->setSynced(0);
        }
    }

    // 推送下一个项目
    qInfo() << "继续推送队列中的下一个项目...";
    推送下个项目();
}

void TodoSyncServer::推送下个项目() {
    m_currentPushIndex++;

    if (m_currentPushIndex < m_pendingUnsyncedItems.size()) {
        // 还有更多项目需要推送
        TodoItem *nextItem = m_pendingUnsyncedItems[m_currentPushIndex];
        推送单个项目(nextItem);

        // 更新进度
        int progress = 75 + (25 * m_currentPushIndex / m_pendingUnsyncedItems.size());
        emit syncProgress(
            progress, QString("正在推送项目 %1/%2...").arg(m_currentPushIndex + 1).arg(m_pendingUnsyncedItems.size()));
    } else {
        // 所有项目都已推送完成
        qDebug() << "所有项目推送完成";

        setIsSyncing(false);
        更新最后同步时间();
        emit syncCompleted(Success, "同步完成");

        // 清理临时数据
        m_pendingUnsyncedItems.clear();
        m_currentPushIndex = 0;
    }
}