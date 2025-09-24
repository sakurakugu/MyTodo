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
#include "default_value.h"

#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
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

        m_networkRequest.sendRequest(NetworkRequest::RequestType::CreateCategory, config);
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

        m_networkRequest.sendRequest(NetworkRequest::RequestType::UpdateCategory, config);
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

        m_networkRequest.sendRequest(NetworkRequest::RequestType::DeleteCategory, config);
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
void CategorySyncServer::onNetworkRequestCompleted(NetworkRequest::RequestType type, const QJsonObject &response) {
    switch (type) {
    case NetworkRequest::RequestType::FetchCategories:
        处理获取类别成功(response);
        break;
    case NetworkRequest::RequestType::CreateCategory:
        处理创建类别成功(response);
        break;
    case NetworkRequest::RequestType::UpdateCategory:
        处理更新类别成功(response);
        break;
    case NetworkRequest::RequestType::DeleteCategory:
        处理删除类别成功(response);
        break;
    default:
        // 调用基类处理其他类型的请求
        BaseSyncServer::onNetworkRequestCompleted(type, response);
        break;
    }
}

void CategorySyncServer::onNetworkRequestFailed(NetworkRequest::RequestType type, NetworkRequest::NetworkError error,
                                                const QString &message) {
    QString typeStr;
    SyncResult result = NetworkError;

    switch (type) {
    case NetworkRequest::RequestType::FetchCategories:
        typeStr = "拉取类别";
        break;
    case NetworkRequest::RequestType::CreateCategory:
        typeStr = "新建类别";
        qInfo() << "类别创建失败！错误类型:" << static_cast<int>(error);
        qInfo() << "失败详情:" << message;
        emit categoryCreated(m_currentOperationName, false, message);
        return;
    case NetworkRequest::RequestType::UpdateCategory:
        typeStr = "更新类别";
        qInfo() << "类别更新失败！错误类型:" << static_cast<int>(error);
        qInfo() << "失败详情:" << message;
        emit categoryUpdated(m_currentOperationName, m_currentOperationNewName, false, message);
        return;
    case NetworkRequest::RequestType::DeleteCategory:
        typeStr = "删除类别";
        qInfo() << "类别删除失败！错误类型:" << static_cast<int>(error);
        qInfo() << "失败详情:" << message;
        emit categoryDeleted(m_currentOperationName, false, message);
        return;
    default:
        // 其他类型的请求不在类别同步管理器的处理范围内
        qInfo() << "未知请求类型失败:" << static_cast<int>(type) << "错误:" << message;
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
    m_isSyncing = false;
    emit syncingChanged();
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
        // 双向同步：先获取服务器数据，然后在 处理获取类别成功() 中推送本地更改
        拉取类别();
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
    检查同步前置条件();
    m_isSyncing = true;

    qDebug() << "从服务器获取类别...";
    emit syncProgress(25, "正在从服务器获取类别数据...");

    try {
        NetworkRequest::RequestConfig config;
        config.url = m_networkRequest.getApiUrl(m_apiEndpoint);
        config.method = "GET";
        config.requiresAuth = true;

        m_networkRequest.sendRequest(NetworkRequest::RequestType::FetchCategories, config);
    } catch (const std::exception &e) {
        qCritical() << "获取服务器类别数据时发生异常:" << e.what();

        m_isSyncing = false;
        emit syncingChanged();
        emit syncCompleted(UnknownError, QString("获取服务器类别数据失败: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "获取服务器类别数据时发生未知异常";

        m_isSyncing = false;
        emit syncingChanged();
        emit syncCompleted(UnknownError, "获取服务器类别数据失败：未知错误");
    }
}

void CategorySyncServer::推送类别() {
    检查同步前置条件();
    m_isSyncing = true;

    if (m_unsyncedItems.empty()) {
        qInfo() << "没有需要同步的类别，上传流程完成";

        // 如果是双向同步且没有本地更改，直接完成
        if (m_currentSyncDirection == Bidirectional || m_currentSyncDirection == UploadOnly) {
            m_isSyncing = false;
            emit syncingChanged();
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
            obj["created_at"] = item->createdAt().toString(Qt::ISODate);
            obj["updated_at"] = item->updatedAt().toString(Qt::ISODate);
            obj["synced"] = item->synced();

            jsonArray.append(obj);
        }

        NetworkRequest::RequestConfig config;
        config.url = m_networkRequest.getApiUrl(m_apiEndpoint);
        config.method = "POST"; // 批量推送使用POST方法
        config.requiresAuth = true;
        config.data["categories"] = jsonArray;

        m_networkRequest.sendRequest(NetworkRequest::RequestType::CreateCategory, config);
    } catch (const std::exception &e) {
        qCritical() << "推送类别更改时发生异常:" << e.what();

        m_isSyncing = false;
        emit syncingChanged();
        emit syncCompleted(UnknownError, QString("推送类别更改失败: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "推送类别更改时发生未知异常";

        m_isSyncing = false;
        emit syncingChanged();
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

    // 如果是双向同步，成功获取数据后推送本地更改
    if (m_currentSyncDirection == Bidirectional) {
        推送类别();
    } else {
        // 仅下载模式，直接完成同步
        m_isSyncing = false;
        emit syncingChanged();
        更新最后同步时间();
        emit syncCompleted(Success, "类别数据获取完成");
    }
}

void CategorySyncServer::处理推送更改成功(const QJsonObject &response) {
    qDebug() << "推送类别更改成功";

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
                    << QString("类别 %1 处理失败: %2").arg(error["index"].toInt()).arg(error["error"].toString());
            }
            shouldMarkAsSynced = false;
            qWarning() << "由于服务器处理错误，不标记类别为已同步";
        }
    } else {
        // 兼容旧的响应格式
        qWarning() << "服务器响应格式不标准，假设操作成功";
        if (response.contains("updated_count")) {
            int updatedCount = response["updated_count"].toInt();
            qDebug() << "已更新" << updatedCount << "个类别";
        }
    }

    // 只有在验证通过时才标记当前批次的类别为已同步
    if (shouldMarkAsSynced) {
        // 发出本地更改已上传信号
        emit localChangesUploaded(m_unsyncedItems);
    }

    emit syncProgress(100, "类别更改推送完成");

    // 清空待同步类别列表
    m_unsyncedItems.clear();

    m_isSyncing = false;
    emit syncingChanged();
    更新最后同步时间();
    emit syncCompleted(Success, "类别更改推送完成");
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
