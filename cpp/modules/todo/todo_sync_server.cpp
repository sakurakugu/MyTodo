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

void TodoSyncServer::取消同步() {
    BaseSyncServer::取消同步();

    // 清理待办事项特有的状态
    m_pendingUnsyncedItems.clear();
    m_allUnsyncedItems.clear();
    m_currentPushIndex = 0;
    m_currentBatchIndex = 0;
    m_totalBatches = 0;
}

void TodoSyncServer::重置同步状态() {
    BaseSyncServer::重置同步状态();

    // 清理待办事项特有的状态
    m_pendingUnsyncedItems.clear();
    m_currentPushIndex = 0;
    m_currentBatchIndex = 0;
    m_totalBatches = 0;
    m_allUnsyncedItems.clear();
}

// 数据操作接口实现
void TodoSyncServer::setTodoItems(const std::vector<TodoItem *> &items) {
    m_todoItems = items;
    qDebug() << "已设置" << items.size() << "个待办事项用于同步";
}

std::vector<TodoItem *> TodoSyncServer::设置未同步的对象() const {
    std::vector<TodoItem *> unsyncedItems;
    int totalItems = 0;
    int syncedItems = 0;

    for (TodoItem *item : m_todoItems) {
        totalItems++;
        if (item->synced() > 0) {
            unsyncedItems.push_back(item);
        } else {
            syncedItems++;
        }
    }

    qDebug() << QString("同步状态检查: 总计=%1, 已同步=%2, 未同步=%3")
                    .arg(totalItems)
                    .arg(syncedItems)
                    .arg(unsyncedItems.size());

    return unsyncedItems;
}

// 网络请求回调处理 - 重写基类方法
void TodoSyncServer::onNetworkRequestCompleted(Network::RequestType type, const QJsonObject &response) {
    switch (type) {
    case Network::RequestType::FetchTodos:
        处理获取数据成功(response);
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

void TodoSyncServer::onNetworkRequestFailed( //
    Network::RequestType type, Network::Error error, const QString &message) {
    QString typeStr;
    switch (type) {
    case Network::RequestType::FetchTodos:
        typeStr = NetworkRequest::GetInstance().RequestTypeToString(type);
        qInfo() << "与服务器同步失败！错误类型:" << static_cast<int>(error);
        qInfo() << typeStr << "失败:" << message;
        break;
    default:
        break;
    }

    BaseSyncServer::onNetworkRequestFailed(type, error, message);
}

void TodoSyncServer::拉取数据() {
    qDebug() << "从服务器获取数据...";
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

void TodoSyncServer::推送数据() {
    // 第二阶段（双向同步 fetch 之后）允许继续沿用 m_isSyncing，占用状态不视为冲突
    if (!是否可以执行同步()) {
        emit syncCompleted(UnknownError, "无法同步");
    }

    // 找出所有未同步的项目
    std::vector<TodoItem *> unsyncedItems = 设置未同步的对象();

    if (unsyncedItems.empty()) {
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
        推送批次到服务器(unsyncedItems);
    } else {
        // 项目数量超过限制，需要分批推送
        qInfo() << "项目数量超过限制，需要分批推送";
        qDebug() << "项目数量超过" << maxBatchSize << "个，将分批推送";
        m_allUnsyncedItems = unsyncedItems;
        m_currentBatchIndex = 0;
        m_totalBatches = (unsyncedItems.size() + maxBatchSize - 1) / maxBatchSize;

        // 开始推送第一批
        推送下一个批次();
    }
}

void TodoSyncServer::推送批次到服务器(const std::vector<TodoItem *> &batch) {
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
            obj["deadline"] = Utility::toRfc3339Json(item->deadline());
            obj["recurrenceInterval"] = item->recurrenceInterval();
            obj["recurrenceCount"] = item->recurrenceCount();
            // recurrenceStartDate 仅在有效时写入
            if (item->recurrenceStartDate().isValid()) {
                obj["recurrenceStartDate"] = item->recurrenceStartDate().toString(Qt::ISODate);
            }
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

        qInfo() << "项目数据:" << QString::fromUtf8(QJsonDocument(jsonArray).toJson(QJsonDocument::Compact));

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

void TodoSyncServer::推送下一个批次() {
    const int maxBatchSize = 100;
    int startIndex = m_currentBatchIndex * maxBatchSize;
    int endIndex = std::min(startIndex + maxBatchSize, static_cast<int>(m_allUnsyncedItems.size()));

    if (startIndex >= static_cast<int>(m_allUnsyncedItems.size())) {
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
    std::vector<TodoItem *> currentBatch;
    for (int i = startIndex; i < endIndex; ++i) {
        currentBatch.push_back(m_allUnsyncedItems[i]);
    }

    qDebug() << "推送第" << (m_currentBatchIndex + 1) << "批，共" << m_totalBatches << "批，当前批次"
             << currentBatch.size() << "个项目";

    // 推送当前批次
    推送批次到服务器(currentBatch);
}

void TodoSyncServer::处理获取数据成功(const QJsonObject &response) {
    qDebug() << "获取数据成功";
    emit syncProgress(50, "数据获取完成，正在处理...");

    if (response.contains("todos")) {
        QJsonArray todosArray = response["todos"].toArray();
        emit todosUpdatedFromServer(todosArray);
    }

    // 如果是双向同步，成功获取数据后推送本地更改
    if (m_currentSyncDirection == Bidirectional) {
        推送数据();
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
    QJsonObject summary = response["summary"].toObject();
    int created = summary.value("created").toInt();
    int updated = summary.value("updated").toInt();
    QJsonArray conflictArray = summary["conflicts"].toArray();
    QJsonArray errorArray = summary["errors"].toArray();
    int conflicts = conflictArray.size();
    int errors = errorArray.size();

    qInfo() << std::format("服务器处理结果: 创建={}, 更新={}, 冲突={}, 错误={}", //
                           created, updated, conflicts, errors);

    // 记录冲突详情（如果有）
    if (conflicts > 0) {
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
    if (errors > 0) {
        int idx = 0;
        for (const auto &errorValue : errorArray) {
            QJsonObject error = errorValue.toObject();
            qWarning() << QString("错误 %1: 项目序号=%2, 描述=%3")
                              .arg(++idx)
                              .arg(error.value("index").toInt())
                              .arg(error.value("error").toString());
        }
    }

    bool shouldMarkAsSynced = true;
    if (conflicts > 0 || errors > 0) {
        shouldMarkAsSynced = false;
        qWarning() << "由于存在" << (conflicts > 0 ? "冲突" : "") << ((conflicts > 0 && errors > 0) ? "和" : "")
                   << (errors > 0 ? "错误" : "") << "，不标记项目为已同步";
    }

    // 只有在验证通过时才标记当前批次的项目为已同步
    if (shouldMarkAsSynced) {
        for (TodoItem *item : m_pendingUnsyncedItems) {
            item->setSynced(0);
        }
        // 发出本地更改已上传信号
        emit localChangesUploaded(m_pendingUnsyncedItems);
    }

    // 检查是否还有更多批次需要推送
    if (!m_allUnsyncedItems.empty() && m_currentBatchIndex < m_totalBatches - 1) {
        // 还有更多批次需要推送
        m_currentBatchIndex++;

        // 更新进度
        int progress = 75 + (20 * m_currentBatchIndex / m_totalBatches);
        emit syncProgress(progress, QString("正在推送第 %1/%2 批...").arg(m_currentBatchIndex + 1).arg(m_totalBatches));

        // 清空当前批次的待同步项目列表
        m_pendingUnsyncedItems.clear();

        推送下一个批次();
    } else {
        // 所有批次都已完成或这是单批次推送
        emit syncProgress(100, "更改推送完成");

        // 清空待同步项目列表
        m_pendingUnsyncedItems.clear();

        if (!m_allUnsyncedItems.empty()) {
            // 分批推送完成
            qDebug() << "所有批次推送完成，共" << m_allUnsyncedItems.size() << "个项目";
            m_allUnsyncedItems.clear();
            m_currentBatchIndex = 0;
            m_totalBatches = 0;
        }

        if (m_currentSyncDirection == Bidirectional) {
            qDebug() << "推送阶段完成，继续执行拉取阶段";
            // 继续拉取（此时保持 m_isSyncing=true 防止外部再触发）
            拉取数据();
            return;
        } else {
            setIsSyncing(false);
            更新最后同步时间();
            emit syncCompleted(Success, "数据更改推送完成");
        }
    }
}

//         m_pendingUnsyncedItems.clear();
//         m_currentPushIndex = 0;
//     }
// }