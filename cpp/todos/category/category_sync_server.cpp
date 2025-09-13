/**
 * @file category_sync_server.cpp
 * @brief CategorySyncServer类的实现文件
 *
 * 该文件实现了CategorySyncServer类的所有方法，负责类别的服务器同步功能。
 *
 * @author Sakurakugu
 * @date 2025-09-10 22:00:00(UTC+8) 周三
 * @version 0.4.0
 */

#include "category_sync_server.h"
#include "default_value.h"
#include "user_auth.h"

#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUuid>

CategorySyncServer::CategorySyncServer(QObject *parent)
    : BaseSyncServer(parent), //
      m_currentPushIndex(0),  //
      m_currentBatchIndex(0), //
      m_totalBatches(0)       //
{

    // 设置类别特有的API端点
    m_apiEndpoint = m_setting
                        .get("server/categoriesApiEndpoint",
                             QString::fromStdString(std::string{DefaultValues::categoriesApiEndpoint}))
                        .toString();

    qDebug() << "CategorySyncServer 已初始化，API端点:" << m_apiEndpoint;
}

CategorySyncServer::~CategorySyncServer() {
    qDebug() << "CategorySyncServer 已销毁";
}

// 属性访问器已在基类中实现

// 同步操作实现
void CategorySyncServer::syncWithServer(SyncDirection direction) {
    qDebug() << "CategorySyncServer::syncWithServer 开始，当前同步状态:" << m_isSyncing;

    if (m_isSyncing) {
        qDebug() << "类别同步操作正在进行中，忽略新的同步请求";
        return;
    }

    qDebug() << "调用 canPerformSync() 检查...";
    if (!canPerformSync()) {
        qDebug() << "canPerformSync() 检查失败";
        // 发出同步完成信号，通知UI重置状态
        emit syncCompleted(AuthError, "无法同步：未登录");
        return;
    }

    qDebug() << "canPerformSync() 检查通过，开始执行同步";
    m_currentSyncDirection = direction;
    performSync(direction);
}

void CategorySyncServer::fetchCategories() {
    if (m_isSyncing) {
        qDebug() << "类别同步操作正在进行中，忽略获取请求";
        return;
    }

    syncWithServer(DownloadOnly);
}

void CategorySyncServer::pushCategories() {
    if (m_isSyncing) {
        qDebug() << "类别同步操作正在进行中，忽略推送请求";
        return;
    }

    syncWithServer(UploadOnly);
}

void CategorySyncServer::cancelSync() {
    if (!m_isSyncing) {
        qDebug() << "没有正在进行的类别同步操作可以取消";
        return;
    }

    qDebug() << "取消类别同步操作";

    // 调用基类方法
    BaseSyncServer::cancelSync();

    // 清理类别特有的状态
    m_pendingUnsyncedItems.clear();
}

void CategorySyncServer::resetSyncState() {
    BaseSyncServer::resetSyncState();
    m_pendingUnsyncedItems.clear();
    qDebug() << "类别同步状态已重置";
}

// 类别操作接口实现
void CategorySyncServer::createCategoryWithServer(const QString &name) {
    if (!canPerformSync()) {
        emit categoryCreated(name, false, "无法创建类别：未登录");
        return;
    }

    if (!isValidCategoryName(name)) {
        emit categoryCreated(name, false, "类别名称无效");
        return;
    }

    m_currentOperationName = name;
    qDebug() << "创建类别到服务器:" << name;

    try {
        NetworkRequest::RequestConfig config;
        config.url = getApiUrl(m_apiEndpoint);
        config.method = "POST";
        config.requiresAuth = true;
        config.data["name"] = name;

        m_networkRequest.sendRequest(NetworkRequest::RequestType::CreateCategory, config);
    } catch (const std::exception &e) {
        qCritical() << "创建类别时发生异常:" << e.what();
        emit categoryCreated(name, false, QString("创建类别失败: %1").arg(e.what()));
    }
}

void CategorySyncServer::updateCategoryWithServer(const QString &name, const QString &newName) {
    if (!canPerformSync()) {
        emit categoryUpdated(name, newName, false, "无法更新类别：未登录");
        return;
    }

    if (!isValidCategoryName(newName)) {
        emit categoryUpdated(name, newName, false, "新类别名称无效");
        return;
    }

    m_currentOperationName = name;
    m_currentOperationNewName = newName;
    qDebug() << "更新类别到服务器:" << name << "->" << newName;

    try {
        NetworkRequest::RequestConfig config;
        config.url = getApiUrl(m_apiEndpoint);
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

void CategorySyncServer::deleteCategoryWithServer(const QString &name) {
    if (!canPerformSync()) {
        emit categoryDeleted(name, false, "无法删除类别：未登录");
        return;
    }

    m_currentOperationName = name;
    qDebug() << "删除类别到服务器:" << name;

    try {
        NetworkRequest::RequestConfig config;
        config.url = getApiUrl(m_apiEndpoint);
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
void CategorySyncServer::setCategoryItems(const std::vector<std::unique_ptr<CategorieItem>> &items) {
    m_categoryItems.clear();
    for (const auto &item : items) {
        m_categoryItems.push_back(item.get());
    }
    qDebug() << "已设置" << m_categoryItems.size() << "个类别用于同步";
}

std::vector<CategorieItem *> CategorySyncServer::getUnsyncedItems() const {
    std::vector<CategorieItem *> unsyncedItems;
    int totalItems = 0;
    int syncedItems = 0;

    for (CategorieItem *item : m_categoryItems) {
        totalItems++;
        if (!item->synced()) {
            unsyncedItems.push_back(item);
        } else {
            syncedItems++;
        }
    }

    qDebug() << QString("类别同步状态检查: 总计=%1, 已同步=%2, 未同步=%3")
                    .arg(totalItems)
                    .arg(syncedItems)
                    .arg(unsyncedItems.size());

    return unsyncedItems;
}

void CategorySyncServer::markItemAsSynced(CategorieItem *item) {
    if (item) {
        item->setSynced(true);
    }
}

void CategorySyncServer::markItemAsUnsynced(CategorieItem *item) {
    if (item) {
        item->setSynced(false);
    }
}

// 网络请求回调处理
void CategorySyncServer::onNetworkRequestCompleted(NetworkRequest::RequestType type, const QJsonObject &response) {
    switch (type) {
    case NetworkRequest::RequestType::FetchCategories:
        handleFetchCategoriesSuccess(response);
        break;
    case NetworkRequest::RequestType::CreateCategory:
        handleCreateCategorySuccess(response);
        break;
    case NetworkRequest::RequestType::UpdateCategory:
        handleUpdateCategorySuccess(response);
        break;
    case NetworkRequest::RequestType::DeleteCategory:
        handleDeleteCategorySuccess(response);
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
        typeStr = "获取类别";
        break;
    case NetworkRequest::RequestType::CreateCategory:
        typeStr = "创建类别";
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

// 自动同步定时器和URL变化处理已在基类中实现

// 同步操作实现
void CategorySyncServer::performSync(SyncDirection direction) {
    qDebug() << "开始同步类别，方向:" << direction;

    m_isSyncing = true;
    emit syncingChanged();
    emit syncStarted();

    // 根据同步方向执行不同的操作
    switch (direction) {
    case Bidirectional:
        // 双向同步：先获取服务器数据，然后在handleFetchCategoriesSuccess中推送本地更改
        fetchCategoriesFromServer();
        break;
    case UploadOnly:
        // 仅上传：只推送本地更改
        pushLocalChangesToServer();
        break;
    case DownloadOnly:
        // 仅下载：只获取服务器数据
        fetchCategoriesFromServer();
        break;
    }
}

void CategorySyncServer::fetchCategoriesFromServer() {
    // 注意：此方法被performSync调用时，m_isSyncing已经为true，所以不需要再次检查
    // 只检查用户认证状态即可
    if (m_serverBaseUrl.isEmpty() || m_apiEndpoint.isEmpty()) {
        qDebug() << "同步检查失败：服务器配置为空";
        m_isSyncing = false;
        emit syncingChanged();
        emit syncCompleted(AuthError, "无法同步：服务器配置错误");
        return;
    }

    if (!UserAuth::GetInstance().isLoggedIn()) {
        qDebug() << "同步检查失败：用户未登录或令牌已过期";
        m_isSyncing = false;
        emit syncingChanged();
        emit syncCompleted(AuthError, "无法同步：未登录");
        return;
    }

    qDebug() << "从服务器获取类别...";
    emit syncProgress(25, "正在从服务器获取类别数据...");

    try {
        NetworkRequest::RequestConfig config;
        config.url = getApiUrl(m_apiEndpoint);
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

void CategorySyncServer::pushLocalChangesToServer() {
    qInfo() << "开始推送本地类别更改到服务器...";

    // 注意：此方法被performSync调用时，m_isSyncing已经为true，所以不需要再次检查
    // 只检查用户认证状态即可
    if (m_serverBaseUrl.isEmpty() || m_apiEndpoint.isEmpty()) {
        qDebug() << "同步检查失败：服务器配置为空";
        qInfo() << "无法执行类别同步操作 - 服务器配置错误";
        m_isSyncing = false;
        emit syncingChanged();
        emit syncCompleted(AuthError, "无法同步：服务器配置错误");
        return;
    }

    if (!UserAuth::GetInstance().isLoggedIn()) {
        qDebug() << "同步检查失败：用户未登录或令牌已过期";
        qInfo() << "无法执行类别同步操作 - 检查网络连接、用户认证和服务器配置";
        m_isSyncing = false;
        emit syncingChanged();
        emit syncCompleted(AuthError, "无法同步：未登录");
        return;
    }

    // 找出所有未同步的类别
    std::vector<CategorieItem *> unsyncedItems = getUnsyncedItems();
    qInfo() << "检测到" << unsyncedItems.size() << "个未同步的类别";

    if (unsyncedItems.empty()) {
        qInfo() << "没有需要同步的类别，上传流程完成";

        // 如果是双向同步且没有本地更改，直接完成
        if (m_currentSyncDirection == Bidirectional || m_currentSyncDirection == UploadOnly) {
            m_isSyncing = false;
            emit syncingChanged();
            updateLastSyncTime();
            emit syncCompleted(Success, "类别同步完成");
        }
        return;
    }

    qInfo() << "开始推送" << unsyncedItems.size() << "个类别到服务器";

    // 类别数量通常较少，直接推送所有类别
    m_pendingUnsyncedItems = unsyncedItems;
    pushBatchToServer(unsyncedItems);
}

void CategorySyncServer::pushBatchToServer(const std::vector<CategorieItem *> &batch) {
    emit syncProgress(75, QString("正在推送 %1 个类别更改到服务器...").arg(batch.size()));

    try {
        // 创建一个包含当前批次类别的JSON数组
        QJsonArray jsonArray;
        for (CategorieItem *item : batch) {
            QJsonObject obj;
            obj["id"] = item->id();
            obj["uuid"] = item->uuid().toString(QUuid::WithoutBraces);
            obj["user_uuid"] = item->userUuid().toString(QUuid::WithoutBraces);
            obj["name"] = item->name();
            obj["created_at"] = item->createdAt().toString(Qt::ISODate);

            jsonArray.append(obj);
        }

        NetworkRequest::RequestConfig config;
        config.url = getApiUrl(m_apiEndpoint);
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

void CategorySyncServer::handleFetchCategoriesSuccess(const QJsonObject &response) {
    qDebug() << "获取类别成功";
    emit syncProgress(50, "类别数据获取完成，正在处理...");

    if (response.contains("categories")) {
        QJsonArray categoriesArray = response["categories"].toArray();
        emit categoriesUpdatedFromServer(categoriesArray);
    }

    // 如果是双向同步，成功获取数据后推送本地更改
    if (m_currentSyncDirection == Bidirectional) {
        pushLocalChangesToServer();
    } else {
        // 仅下载模式，直接完成同步
        m_isSyncing = false;
        emit syncingChanged();
        updateLastSyncTime();
        emit syncCompleted(Success, "类别数据获取完成");
    }
}

void CategorySyncServer::handlePushChangesSuccess(const QJsonObject &response) {
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
        for (CategorieItem *item : m_pendingUnsyncedItems) {
            item->setSynced(true);
        }
        // 发出本地更改已上传信号
        emit localChangesUploaded(m_pendingUnsyncedItems);
    }

    emit syncProgress(100, "类别更改推送完成");

    // 清空待同步类别列表
    m_pendingUnsyncedItems.clear();

    m_isSyncing = false;
    emit syncingChanged();
    updateLastSyncTime();
    emit syncCompleted(Success, "类别更改推送完成");
}

void CategorySyncServer::handleCreateCategorySuccess(const QJsonObject &response) {
    qDebug() << "创建类别成功:" << m_currentOperationName;

    QString message = "类别创建成功";
    if (response.contains("message")) {
        message = response["message"].toString();
    }

    emit categoryCreated(m_currentOperationName, true, message);
    emitOperationCompleted("创建类别", true, message);
}

void CategorySyncServer::handleUpdateCategorySuccess(const QJsonObject &response) {
    qDebug() << "更新类别成功:" << m_currentOperationName << "->" << m_currentOperationNewName;

    QString message = "类别更新成功";
    if (response.contains("message")) {
        message = response["message"].toString();
    }

    emit categoryUpdated(m_currentOperationName, m_currentOperationNewName, true, message);
    emitOperationCompleted("更新类别", true, message);
}

void CategorySyncServer::handleDeleteCategorySuccess(const QJsonObject &response) {
    qDebug() << "删除类别成功:" << m_currentOperationName;

    QString message = "类别删除成功";
    if (response.contains("message")) {
        message = response["message"].toString();
    }

    emit categoryDeleted(m_currentOperationName, true, message);
    emitOperationCompleted("删除类别", true, message);
}

// 辅助方法实现（大部分已在基类中实现）

bool CategorySyncServer::isValidCategoryName(const QString &name) const {
    return !name.trimmed().isEmpty() && name.length() <= 50;
}

void CategorySyncServer::emitOperationCompleted(const QString &operation, bool success, const QString &message) {
    emit operationCompleted(operation, success, message);
}