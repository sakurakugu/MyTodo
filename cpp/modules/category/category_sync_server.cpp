/**
 * @file category_sync_server.cpp
 * @brief CategorySyncServer类的实现文件
 *
 * 该文件实现了CategorySyncServer类的所有方法，负责类别的服务器同步功能。
 *
 * @author Sakurakugu
 * @date 2025-09-10 22:00:18(UTC+8) 周三
 * @change 2025-10-06 02:13:23(UTC+8) 周一
 */

#include "category_sync_server.h"
#include "category_item.h"
#include "config.h"
#include "default_value.h"
#include "utility.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <uuid.h>

CategorySyncServer::CategorySyncServer(UserAuth &userAuth, QObject *parent) : BaseSyncServer(userAuth, parent) {
    // 设置类别特有的API端点
    m_apiEndpoint =
        m_config.get("server/categoriesApiEndpoint", QString(DefaultValues::categoriesApiEndpoint)).toString();
}

CategorySyncServer::~CategorySyncServer() {}

void CategorySyncServer::取消同步() {
    BaseSyncServer::取消同步();
    m_unsyncedItems.clear();
}

void CategorySyncServer::重置同步状态() {
    BaseSyncServer::重置同步状态();
    m_unsyncedItems.clear();
}

// 类别操作接口实现
void CategorySyncServer::新增类别(const QString &name) {
    if (!是否可以执行同步()) {
        return;
    }

    m_currentOperationName = name;
    qDebug() << "新增类别到服务器:" << name;

    try {
        NetworkRequest::RequestConfig config;
        config.url = m_networkRequest.getApiUrl(m_apiEndpoint);
        config.method = "POST";
        config.requiresAuth = true;
        config.data["name"] = name;

        m_networkRequest.sendRequest(Network::RequestType::CreateCategory, config);
    } catch (const std::exception &e) {
        qCritical() << "新增类别时发生异常:" << e.what();
    }
}

void CategorySyncServer::更新类别(const QString &name, const QString &newName) {
    if (!是否可以执行同步()) {
        return;
    }

    m_currentOperationName = name;
    m_currentOperationNewName = newName;
    qDebug() << "更新类别到服务器:" << name << "->" << newName;

    try {
        NetworkRequest::RequestConfig config;
        config.url = m_networkRequest.getApiUrl(m_apiEndpoint);
        config.method = "PATCH";
        config.requiresAuth = true;
        config.data["old_name"] = name;
        config.data["new_name"] = newName;

        m_networkRequest.sendRequest(Network::RequestType::UpdateCategory, config);
    } catch (const std::exception &e) {
        qCritical() << "更新类别时发生异常:" << e.what();
    }
}

void CategorySyncServer::删除类别(const QString &name) {
    if (!是否可以执行同步()) {
        return;
    }

    m_currentOperationName = name;
    qDebug() << "删除类别到服务器:" << name;

    try {
        NetworkRequest::RequestConfig config;
        config.url = m_networkRequest.getApiUrl(m_apiEndpoint);
        config.method = "DELETE";
        config.requiresAuth = true;
        config.data["name"] = name;

        m_networkRequest.sendRequest(Network::RequestType::DeleteCategory, config);
    } catch (const std::exception &e) {
        qCritical() << "删除类别时发生异常:" << e.what();
    }
}

// 数据操作接口实现
void CategorySyncServer::设置未同步的对象(const std::vector<std::unique_ptr<CategorieItem>> &categoryItems) {
    m_unsyncedItems.clear();
    // 去除 “未同步” 类别
    int totalItems = -1;
    int syncedItems = -1;

    for (const auto &item : categoryItems) {
        totalItems++;
        if (item->synced() > 0) { // 1=插入,2=更新,3=删除
            m_unsyncedItems.push_back(item.get());
        } else {
            syncedItems++;
        }
    }

    qDebug() << QString("同步状态检查: 总计=%1, 已同步=%2, 未同步=%3")
                    .arg(totalItems)
                    .arg(syncedItems)
                    .arg(m_unsyncedItems.size());
}

// 网络请求回调处理
void CategorySyncServer::onNetworkRequestCompleted(Network::RequestType type, const QJsonObject &response) {
    switch (type) {
    case Network::RequestType::FetchCategories:
        处理获取数据成功(response);
        break;
    case Network::RequestType::CreateCategory:
        处理创建类别成功(response);
        break;
    case Network::RequestType::PushCategories:
        处理推送更改成功(response);
        break;
    case Network::RequestType::UpdateCategory:
        处理更新类别成功(response);
        break;
    case Network::RequestType::DeleteCategory:
        处理删除类别成功(response);
        break;
    default:
        // 调用基类处理其他类型的请求
        BaseSyncServer::onNetworkRequestCompleted(type, response);
        break;
    }
}

void CategorySyncServer::onNetworkRequestFailed( //
    Network::RequestType type, Network::Error error, const QString &message) {
    QString typeStr;
    switch (type) {
    case Network::RequestType::FetchCategories:
    case Network::RequestType::CreateCategory:
    case Network::RequestType::PushCategories:
    case Network::RequestType::UpdateCategory:
    case Network::RequestType::DeleteCategory:
        typeStr = NetworkRequest::GetInstance().RequestTypeToString(type);
        qInfo() << "与服务器同步失败！错误类型:" << static_cast<int>(error);
        qInfo() << typeStr << "失败:" << message;
        break;
    default:
        break;
    }

    BaseSyncServer::onNetworkRequestFailed(type, error, message);
}

void CategorySyncServer::拉取数据() {
    qDebug() << "从服务器获取数据...";
    emit syncProgress(25, "正在从服务器获取数据...");

    try {
        NetworkRequest::RequestConfig config;
        config.url = m_networkRequest.getApiUrl(m_apiEndpoint);
        config.method = "GET";
        config.requiresAuth = true;

        m_networkRequest.sendRequest(Network::RequestType::FetchCategories, config);
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

void CategorySyncServer::推送数据() {
    // 第二阶段（推送）允许前一阶段已占用同步状态，避免误判
    if (!是否可以执行同步()) {
        emit syncCompleted(UnknownError, "无法同步");
    }

    if (m_unsyncedItems.empty()) {
        qInfo() << "没有需要同步的类别，上传流程完成";

        // 如果是双向同步且没有本地更改，直接完成
        if (m_currentSyncDirection == Bidirectional || m_currentSyncDirection == UploadOnly) {
            setIsSyncing(false);
            更新最后同步时间();
            emit syncCompleted(Success, "同步完成");
        }
        return;
    }

    qInfo() << "开始推送" << m_unsyncedItems.size() << "个项目到服务器";
    emit syncProgress(75, QString("正在推送 %1 个更改到服务器...").arg(m_unsyncedItems.size()));

    try {
        // 创建一个包含当前批次类别的JSON数组
        QJsonArray jsonArray;
        for (CategorieItem *item : std::as_const(m_unsyncedItems)) {
            QJsonObject obj;
            obj["uuid"] = QString::fromStdString(uuids::to_string(item->uuid()));
            obj["name"] = QString::fromStdString(item->name());
            obj["created_at"] = Utility::toRfc3339Json(item->createdAt().toQDateTime());
            obj["updated_at"] = Utility::toRfc3339Json(item->updatedAt().toQDateTime());
            obj["synced"] = item->synced();
            jsonArray.append(obj);
        }

        NetworkRequest::RequestConfig config;
        config.url = m_networkRequest.getApiUrl(m_apiEndpoint);
        config.method = "POST"; // 批量推送使用POST方法
        config.requiresAuth = true;
        config.data["categories"] = jsonArray;

        // #ifdef QT_DEBUG
        //         // 调试日志：打印payload（截断防止过长）
        //         QJsonDocument payloadDoc(config.data);
        //         QByteArray payloadBytes = payloadDoc.toJson(QJsonDocument::Compact);
        //         qDebug() << "批量类别同步Payload:" << QString::fromUtf8(payloadBytes.left(512));
        // #endif

        m_networkRequest.sendRequest(Network::RequestType::PushCategories, config);
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

void CategorySyncServer::处理获取数据成功(const QJsonObject &response) {
    qDebug() << "获取数据成功";
    emit syncProgress(50, "数据获取完成，正在处理...");

    if (response.contains("categories")) {
        QJsonArray categoriesArray = response["categories"].toArray();
        emit categoriesUpdatedFromServer(categoriesArray);
    }

    // 双向同步逻辑：先拉取再推送
    if (m_currentSyncDirection == Bidirectional) {
        推送数据();
    } else {
        // 仅下载模式，直接完成同步
        setIsSyncing(false);
        更新最后同步时间();
        emit syncCompleted(Success, "数据获取完成");
    }
}

void CategorySyncServer::处理推送更改成功(const QJsonObject &response) {
    qDebug() << "推送更改成功";

    // 验证服务器响应
    QJsonObject summary = response["summary"].toObject();
    int created = summary["created"].toInt();
    int updated = summary["updated"].toInt();
    QJsonArray errorArray = summary["errors"].toArray();
    int errors = errorArray.size();

    qInfo() << std::format("服务器处理结果: 创建={}, 更新={}, 错误={}", //
                           created, updated, errors);

    // 如果有错误，记录详细信息
    QSet<int> failedIndexes;
    if (errors > 0) {
        for (const auto &errorValue : errorArray) {
            QJsonObject errObj = errorValue.toObject();
            int idx = errObj["index"].toInt(-1);
            QString errMsg = errObj["error"].toString();
            qWarning() << QString("类别条目 index=%1 处理失败: %2").arg(idx).arg(errMsg);
            if (idx >= 0)
                failedIndexes.insert(idx);
        }
    }

    const auto nowUtc = QDateTime::currentDateTimeUtc();
    int changed = 0;
    std::vector<CategorieItem *> actuallySynced;
    actuallySynced.reserve(m_unsyncedItems.size());
    for (int i = 0; i < static_cast<int>(m_unsyncedItems.size()); ++i) {
        auto *item = m_unsyncedItems[i];
        if (!item)
            continue;
        if (failedIndexes.contains(i)) {
            // 保留其 synced 状态，留给下次重试
            continue;
        }
        item->setSynced(0);

        item->setUpdatedAt(nowUtc);
        actuallySynced.push_back(item);
        ++changed;
    }
    if (!actuallySynced.empty()) {
        emit localChangesUploaded(actuallySynced);
    }

    emit syncProgress(100, "类别更改推送完成");

    // 移除已成功的条目, 保留失败的以便下次重试
    if (!m_unsyncedItems.empty()) {
        std::vector<CategorieItem *> remaining;
        remaining.reserve(failedIndexes.size());
        for (int i = 0; i < static_cast<int>(m_unsyncedItems.size()); ++i) {
            if (failedIndexes.contains(i)) {
                remaining.push_back(m_unsyncedItems[i]);
            }
        }
        m_unsyncedItems.swap(remaining);
    }

    // 如果是双向同步，则在推送成功后继续拉取服务器最新（此时本地改名等已写入服务器）
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

void CategorySyncServer::处理创建类别成功(const QJsonObject &response) {
    qDebug() << "创建类别成功:" << m_currentOperationName;

    QString message = "类别创建成功";
    if (response.contains("message")) {
        message = response["message"].toString();
    }
    qWarning() << message;
}

void CategorySyncServer::处理更新类别成功(const QJsonObject &response) {
    qDebug() << "更新类别成功:" << m_currentOperationName << "->" << m_currentOperationNewName;

    QString message = "类别更新成功";
    if (response.contains("message")) {
        message = response["message"].toString();
    }
    qWarning() << message;
}

void CategorySyncServer::处理删除类别成功(const QJsonObject &response) {
    qDebug() << "删除类别成功:" << m_currentOperationName;

    QString message = "类别删除成功";
    if (response.contains("message")) {
        message = response["message"].toString();
    }
    qWarning() << message;
}
