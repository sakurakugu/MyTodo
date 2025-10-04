/**
 * @file category_sync_server.cpp
 * @brief CategorySyncServer类的实现文件
 *
 * 该文件实现了CategorySyncServer类的所有方法，负责类别的服务器同步功能。
 *
 * @author Sakurakugu
 * @date 2025-09-10 22:00:18(UTC+8) 周三
 * @change 2025-09-24 03:45:31(UTC+8) 周三
 */

#include "category_sync_server.h"
#include "category_item.h"
#include "foundation/default_value.h"
#include "foundation/config.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QUuid>

CategorySyncServer::CategorySyncServer(UserAuth &userAuth, QObject *parent) : BaseSyncServer(userAuth, parent) {

    // 设置类别特有的API端点
    m_apiEndpoint =
        m_config.get("server/categoriesApiEndpoint", QString(DefaultValues::categoriesApiEndpoint)).toString();
}

CategorySyncServer::~CategorySyncServer() {}

// 属性访问器已在基类中实现

// 同步操作实现
void CategorySyncServer::与服务器同步(SyncDirection direction) {
    if (m_isSyncing) {
        qDebug() << "类别同步操作正在进行中，忽略新的同步请求";
        return;
    }

    qDebug() << "与服务器同步开始，当前同步状态:" << m_isSyncing;

    if (!是否可以执行同步()) {
        // 发出同步完成信号，通知UI重置状态
        emit syncCompleted(AuthError, "无法同步：未登录");
        return;
    }

    m_currentSyncDirection = direction;
    执行同步(direction);
}

void CategorySyncServer::重置同步状态() {
    BaseSyncServer::重置同步状态();
    m_unsyncedItems.clear();
}

void CategorySyncServer::取消同步() {
    BaseSyncServer::取消同步();
    m_unsyncedItems.clear();
}

// 类别操作接口实现
void CategorySyncServer::新增类别(const QString &name) {
    if (!是否可以执行同步()) {
        emit categoryCreated(name, false, "无法新增类别：未登录");
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
        emit categoryCreated(name, false, QString("新增类别失败: %1").arg(e.what()));
    }
}

void CategorySyncServer::更新类别(const QString &name, const QString &newName) {
    if (!是否可以执行同步()) {
        emit categoryUpdated(name, newName, false, "无法更新类别：未登录");
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
        emit categoryUpdated(name, newName, false, QString("更新类别失败: %1").arg(e.what()));
    }
}

void CategorySyncServer::删除类别(const QString &name) {
    if (!是否可以执行同步()) {
        emit categoryDeleted(name, false, "无法删除类别：未登录");
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
        emit categoryDeleted(name, false, QString("删除类别失败: %1").arg(e.what()));
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

    qDebug() << QString("类别同步状态检查: 总计=%1, 已同步=%2, 未同步=%3")
                    .arg(totalItems)
                    .arg(syncedItems)
                    .arg(m_unsyncedItems.size());
}

// 网络请求回调处理
void CategorySyncServer::onNetworkRequestCompleted(Network::RequestType type, const QJsonObject &response) {
    switch (type) {
    case Network::RequestType::FetchCategories:
        处理获取类别成功(response);
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

void CategorySyncServer::onNetworkRequestFailed(Network::RequestType type, NetworkRequest::NetworkError error,
                                                const QString &message) {
    QString typeStr;
    SyncResult result = NetworkError;

    switch (type) {
    case Network::RequestType::FetchCategories:
        typeStr = "拉取类别";
        break;
    case Network::RequestType::CreateCategory:
        typeStr = "新建类别";
        qInfo() << "类别创建失败！错误类型:" << static_cast<int>(error);
        qInfo() << "失败详情:" << message;
        emit categoryCreated(m_currentOperationName, false, message);
        return;
    case Network::RequestType::PushCategories:
        typeStr = "批量同步类别";
        qInfo() << "批量类别同步失败！错误类型:" << static_cast<int>(error);
        qInfo() << "失败详情:" << message;
        // 批量失败视为同步失败
        setIsSyncing(false);
        emit syncCompleted(NetworkError, message);
        return;
    case Network::RequestType::UpdateCategory:
        typeStr = "更新类别";
        qInfo() << "类别更新失败！错误类型:" << static_cast<int>(error);
        qInfo() << "失败详情:" << message;
        emit categoryUpdated(m_currentOperationName, m_currentOperationNewName, false, message);
        return;
    case Network::RequestType::DeleteCategory:
        typeStr = "删除类别";
        qInfo() << "类别删除失败！错误类型:" << static_cast<int>(error);
        qInfo() << "失败详情:" << message;
        emit categoryDeleted(m_currentOperationName, false, message);
        return;
    default:
        return;
    }

    // 根据错误类型确定同步结果
    switch (error) {
    case NetworkRequest::NetworkError::AuthenticationError:
        result = AuthError;
        qInfo() << "认证错误 - 用户可能需要重新登录";
        break;
    case NetworkRequest::NetworkError::UnknownError:
        result = NetworkError;
        qInfo() << "网络错误 - 请检查网络连接和服务器状态";
        break;
    default:
        result = UnknownError;
        qInfo() << "未知错误类型:" << static_cast<int>(error);
        break;
    }

    qInfo() << "类别同步状态更新: isSyncing = false";
    setIsSyncing(false);
    emit syncCompleted(result, message);

    qWarning() << typeStr << "失败:" << message;
    qDebug() << "错误处理完成，同步结果:" << static_cast<int>(result);
}

// 同步操作实现
void CategorySyncServer::执行同步(SyncDirection direction) {
    qDebug() << "开始同步类别，方向:" << direction;

    emit syncingChanged();
    emit syncStarted();

    // 根据同步方向执行不同的操作
    switch (direction) {
    case Bidirectional:
        // 若存在本地未同步更改，先推送再拉取，避免拉取阶段把旧名称再插入成重复
        if (!m_unsyncedItems.empty()) {
            m_pushFirstInBidirectional = true;
            推送类别();
        } else {
            // 没有本地更改仍按旧逻辑直接拉取
            m_pushFirstInBidirectional = false;
            拉取类别();
        }
        break;
    case UploadOnly:
        // 仅上传：只推送本地更改
        推送类别();
        break;
    case DownloadOnly:
        // 仅下载：只获取服务器数据
        拉取类别();
        break;
    }
}

void CategorySyncServer::拉取类别() {
    // 第一阶段（拉取）严格检查
    检查同步前置条件(false);
    setIsSyncing(false);

    qDebug() << "从服务器获取类别...";
    emit syncProgress(25, "正在从服务器获取类别数据...");

    try {
        NetworkRequest::RequestConfig config;
        config.url = m_networkRequest.getApiUrl(m_apiEndpoint);
        config.method = "GET";
        config.requiresAuth = true;

        m_networkRequest.sendRequest(Network::RequestType::FetchCategories, config);
    } catch (const std::exception &e) {
        qCritical() << "获取服务器类别数据时发生异常:" << e.what();

        setIsSyncing(false);
        emit syncCompleted(UnknownError, QString("获取服务器类别数据失败: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "获取服务器类别数据时发生未知异常";

        setIsSyncing(false);
        emit syncCompleted(UnknownError, "获取服务器类别数据失败：未知错误");
    }
}

void CategorySyncServer::推送类别() {
    // 第二阶段（推送）允许前一阶段已占用同步状态，避免误判
    检查同步前置条件(true);
    setIsSyncing(true);

    if (m_unsyncedItems.empty()) {
        qInfo() << "没有需要同步的类别，上传流程完成";

        // 如果是双向同步且没有本地更改，直接完成
        if (m_currentSyncDirection == Bidirectional || m_currentSyncDirection == UploadOnly) {
            setIsSyncing(false);
            更新最后同步时间();
            emit syncCompleted(Success, "类别同步完成");
        }
        return;
    }

    qInfo() << "开始推送" << m_unsyncedItems.size() << "个类别到服务器";
    emit syncProgress(75, QString("正在推送 %1 个类别更改到服务器...").arg(m_unsyncedItems.size()));

    try {
        // 创建一个包含当前批次类别的JSON数组
        QJsonArray jsonArray;
        for (CategorieItem *item : m_unsyncedItems) {
            QJsonObject obj;
            obj["uuid"] = item->uuid().toString(QUuid::WithoutBraces);
            obj["name"] = item->name();
            // 服务端 SyncCategoryItem.CreatedAt/UpdatedAt 期望 *time.Time (RFC3339/ISO8601 字符串)
            // 原实现发送 int64 毫秒时间戳，Go 端 Unmarshal 失败，fallback 导致 name 为空验证错误
            const auto createdUtc = item->createdAt().toUTC();
            const auto updatedUtc = item->updatedAt().toUTC();
            // Qt::ISODateWithMs 生成 2025-09-28T06:12:34.123Z 形式（需补 Z 表示 UTC）
            auto createdStr = createdUtc.toString(Qt::ISODateWithMs);
            auto updatedStr = updatedUtc.toString(Qt::ISODateWithMs);
            if (!createdStr.endsWith('Z'))
                createdStr += 'Z';
            if (!updatedStr.endsWith('Z'))
                updatedStr += 'Z';
            obj["created_at"] = createdStr;
            obj["updated_at"] = updatedStr;
            obj["synced"] = item->synced();
            jsonArray.append(obj);
        }

        NetworkRequest::RequestConfig config;
        config.url = m_networkRequest.getApiUrl(m_apiEndpoint);
        config.method = "POST"; // 批量推送使用POST方法
        config.requiresAuth = true;
        config.data["categories"] = jsonArray;

#ifdef QT_DEBUG
        // 调试日志：打印payload（截断防止过长）
        QJsonDocument payloadDoc(config.data);
        QByteArray payloadBytes = payloadDoc.toJson(QJsonDocument::Compact);
        qDebug() << "批量类别同步Payload:" << QString::fromUtf8(payloadBytes.left(512));
#endif

        // 批量推送本地未同步类别应使用 PushCategories 类型，以便回调进入 处理推送更改成功()
        m_networkRequest.sendRequest(Network::RequestType::PushCategories, config);
    } catch (const std::exception &e) {
        qCritical() << "推送类别更改时发生异常:" << e.what();

        setIsSyncing(false);
        emit syncCompleted(UnknownError, QString("推送类别更改失败: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "推送类别更改时发生未知异常";

        setIsSyncing(false);
        emit syncCompleted(UnknownError, "推送类别更改失败：未知错误");
    }
}

void CategorySyncServer::处理获取类别成功(const QJsonObject &response) {
    qDebug() << "获取类别成功";
    emit syncProgress(50, "类别数据获取完成，正在处理...");

    if (response.contains("categories")) {
        QJsonArray categoriesArray = response["categories"].toArray();
        emit categoriesUpdatedFromServer(categoriesArray);
    }

    // 双向同步旧逻辑：拉取后再推送；但当 push-first 策略启用时，此处不再推送
    if (m_currentSyncDirection == Bidirectional && !m_pushFirstInBidirectional) {
        推送类别();
    } else {
        // 仅下载模式，直接完成同步
        setIsSyncing(false);
        更新最后同步时间();
        emit syncCompleted(Success, "类别数据获取完成");
    }
}

void CategorySyncServer::处理推送更改成功(const QJsonObject &response) {
    qDebug() << "推送类别更改成功";

    // 验证服务器响应（部分失败支持）
    QJsonObject summary = response["summary"].toObject();
    int created = summary["created"].toInt();
    int updated = summary["updated"].toInt();
    QJsonArray errorArray = summary["errors"].toArray();
    int errors = errorArray.size();

    qInfo() << QString("服务器处理结果: 创建=%1, 更新=%2, 错误=%3").arg(created).arg(updated).arg(errors);

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
        // 如果是删除操作 (synced == 3)，不要在这里把状态重置为0。
        // 逻辑链：
        // 1. 本地删除 -> 软删除时设置 synced=3 或直接物理删除(若尚未同步插入)。
        // 2. 批量推送发送该条目，服务器删除成功。
        // 3. 这里如果改成 0，会使 CategoryModel::更新同步成功状态 无法识别其为删除，从而不会调用物理删除，导致 UI
        // 仍显示。
        // 4. 保持为3，随后 emit localChangesUploaded() 后 CategoryModel 会调用 m_dataStorage.删除类别 进行最终移除。
        if (item->synced() != 3) {
            qInfo() << "类别条目" << item->name() << "同步成功，更新状态为 synced=0";
            item->setSynced(0);
        } else {
            qInfo() << "类别条目" << item->name() << "删除同步成功，保持 synced=3 以便模型层移除";
        }
        item->setUpdatedAt(nowUtc);
        actuallySynced.push_back(item);
        ++changed;
    }
    qDebug() << "成功同步并标记" << changed << "个类别为 synced=0, 失败" << failedIndexes.size();
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

    // 如果是双向同步且采用了 push-first 策略，则在推送成功后继续拉取服务器最新（此时本地改名等已写入服务器）
    if (m_currentSyncDirection == Bidirectional && m_pushFirstInBidirectional) {
        qDebug() << "推送阶段完成（push-first），继续执行拉取阶段";
        // 清空标志，避免递归
        m_pushFirstInBidirectional = false;
        // 继续拉取（此时保持 m_isSyncing=true 防止外部再触发）
        拉取类别();
        return;
    } else {
        setIsSyncing(false);
        更新最后同步时间();
        emit syncCompleted(Success, "类别更改推送完成");
    }
}

void CategorySyncServer::处理创建类别成功(const QJsonObject &response) {
    qDebug() << "创建类别成功:" << m_currentOperationName;

    QString message = "类别创建成功";
    if (response.contains("message")) {
        message = response["message"].toString();
    }

    emit categoryCreated(m_currentOperationName, true, message);
}

void CategorySyncServer::处理更新类别成功(const QJsonObject &response) {
    qDebug() << "更新类别成功:" << m_currentOperationName << "->" << m_currentOperationNewName;

    QString message = "类别更新成功";
    if (response.contains("message")) {
        message = response["message"].toString();
    }

    emit categoryUpdated(m_currentOperationName, m_currentOperationNewName, true, message);
}

void CategorySyncServer::处理删除类别成功(const QJsonObject &response) {
    qDebug() << "删除类别成功:" << m_currentOperationName;

    QString message = "类别删除成功";
    if (response.contains("message")) {
        message = response["message"].toString();
    }

    emit categoryDeleted(m_currentOperationName, true, message);
}
