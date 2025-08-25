/**
 * @file category_manager.cpp
 * @brief CategoryManager类的实现文件
 *
 * 该文件实现了CategoryManager类，用于管理待办事项的类别。
 * 从TodoManager中拆分出来，专门负责类别的CRUD操作和服务器同步。
 *
 * @author Sakurakugu
 * @date 2025-01-24
 * @version 1.0.0
 */

#include "category_manager.h"
#include "default_value.h"
#include "user_auth.h"
#include <QDebug>
#include <QJsonDocument>
#include <algorithm>

CategoryManager::CategoryManager(TodoSyncServer *syncManager, QObject *parent)
    : QObject(parent), m_networkRequest(NetworkRequest::GetInstance()), m_syncManager(syncManager),
      m_userAuth(UserAuth::GetInstance()), m_setting(Setting::GetInstance()) {

    // 连接网络请求信号
    connect(&m_networkRequest, &NetworkRequest::requestCompleted, this, &CategoryManager::onNetworkRequestCompleted);
    connect(&m_networkRequest, &NetworkRequest::requestFailed, this, &CategoryManager::onNetworkRequestFailed);

    // 从配置中加载服务器配置
    m_categoriesApiEndpoint = m_setting
                                  .get(QStringLiteral("server/categoriesApiEndpoint"),
                                       QString::fromStdString(std::string{DefaultValues::categoriesApiEndpoint}))
                                  .toString();

    // 添加默认类别
    addDefaultCategories();
}

CategoryManager::~CategoryManager() {
}

/**
 * @brief 获取类别项目列表
 * @return 类别项目的智能指针向量
 */
QStringList CategoryManager::getCategories() const {
    return m_categories;
}

const std::vector<std::unique_ptr<CategorieItem>> &CategoryManager::getCategoryItems() const {
    return m_categoryItems;
}

// 配置管理实现
void CategoryManager::updateServerConfig(const QString &apiEndpoint) {
    bool changed = false;

    if (m_categoriesApiEndpoint != apiEndpoint) {
        m_categoriesApiEndpoint = apiEndpoint;
        m_setting.save(QStringLiteral("server/categoriesApiEndpoint"), apiEndpoint);
        changed = true;
    }

    if (changed) {
        qDebug() << "服务器配置已更新:";
        qDebug() << "  待办类别API端点:" << m_categoriesApiEndpoint;
    }
}

/**
 * @brief 根据名称查找类别项目
 * @param name 类别名称
 * @return 类别项目指针，如果未找到返回nullptr
 */
CategorieItem *CategoryManager::findCategoryByName(const QString &name) const {
    auto it = std::find_if(m_categoryItems.begin(), m_categoryItems.end(),
                           [&name](const std::unique_ptr<CategorieItem> &item) { return item->name() == name; });
    return (it != m_categoryItems.end()) ? it->get() : nullptr;
}

/**
 * @brief 根据ID查找类别项目
 * @param id 类别ID
 * @return 类别项目指针，如果未找到返回nullptr
 */
CategorieItem *CategoryManager::findCategoryById(int id) const {
    auto it = std::find_if(m_categoryItems.begin(), m_categoryItems.end(),
                           [id](const std::unique_ptr<CategorieItem> &item) { return item->id() == id; });
    return (it != m_categoryItems.end()) ? it->get() : nullptr;
}

/**
 * @brief 检查类别名称是否已存在
 * @param name 类别名称
 * @return 如果存在返回true，否则返回false
 */
bool CategoryManager::categoryExists(const QString &name) const {
    return findCategoryByName(name) != nullptr;
}

/**
 * @brief 添加默认类别
 */
void CategoryManager::addDefaultCategories() {
    // 清空现有类别
    m_categoryItems.clear();
    m_categories.clear();

    // 添加默认的"全部"选项
    m_categories << "全部";

    // 添加默认的"未分类"选项
    auto defaultCategory =
        std::make_unique<CategorieItem>(1, "未分类", m_userAuth.getUuid(), QDateTime::currentDateTime(), false);
    m_categoryItems.push_back(std::move(defaultCategory));
    m_categories << "未分类";

    emit categoriesChanged();
}

/**
 * @brief 清空所有类别
 */
void CategoryManager::clearCategories() {
    m_categoryItems.clear();
    m_categories.clear();
    emit categoriesChanged();
}

void CategoryManager::fetchCategories() {
    if (!m_userAuth.isLoggedIn()) {
        qWarning() << "用户未登录，无法获取类别列表";
        emit fetchCategoriesCompleted(false, "用户未登录");
        return;
    }

    if (!m_syncManager) {
        qWarning() << "同步管理器未初始化";
        emit fetchCategoriesCompleted(false, "同步管理器未初始化");
        return;
    }

    QJsonObject requestData;
    requestData["action"] = "fetch";

    NetworkRequest::RequestConfig config;
    config.url = m_syncManager->getApiUrl(m_categoriesApiEndpoint);
    config.data = requestData;
    config.requiresAuth = true;
    // 移除重复的Authorization头设置，由addAuthHeader方法统一处理

    m_networkRequest.sendRequest(NetworkRequest::FetchCategories, config);
}

void CategoryManager::createCategory(const QString &name) {
    // TODO: 用户未登录时，可以创建，之后还要处理如何与服务端同步的问题
    if (!m_userAuth.isLoggedIn()) {
        qWarning() << "用户未登录，无法创建类别";
        emit createCategoryCompleted(false, "用户未登录");
        return;
    }

    if (!isValidCategoryName(name)) {
        emit createCategoryCompleted(false, "类别名称不能为空或过长");
        return;
    }

    if (categoryExists(name)) {
        emit createCategoryCompleted(false, "类别名称已存在");
        return;
    }

    // TODO: 同上
    if (!m_syncManager) {
        qWarning() << "同步管理器未初始化";
        emit createCategoryCompleted(false, "同步管理器未初始化");
        return;
    }

    QJsonObject requestData;
    requestData["action"] = "create";
    requestData["name"] = name;

    NetworkRequest::RequestConfig config;
    config.url = m_syncManager->getApiUrl(m_categoriesApiEndpoint);
    config.data = requestData;
    config.requiresAuth = true;
    // 移除重复的Authorization头设置，由addAuthHeader方法统一处理

    m_networkRequest.sendRequest(NetworkRequest::CreateCategory, config);
}

void CategoryManager::updateCategory(int id, const QString &name) {
    if (!m_userAuth.isLoggedIn()) {
        qWarning() << "用户未登录，无法更新类别";
        emit updateCategoryCompleted(false, "用户未登录");
        return;
    }

    if (!isValidCategoryName(name)) {
        emit updateCategoryCompleted(false, "类别名称不能为空或过长");
        return;
    }

    // 检查是否尝试更新为已存在的名称（排除自身）
    CategorieItem *existingCategory = findCategoryByName(name);
    if (existingCategory && existingCategory->id() != id) {
        emit updateCategoryCompleted(false, "类别名称已存在");
        return;
    }

    if (!m_syncManager) {
        qWarning() << "同步管理器未初始化";
        emit updateCategoryCompleted(false, "同步管理器未初始化");
        return;
    }

    QJsonObject requestData;
    requestData["action"] = "update";
    requestData["id"] = id;
    requestData["name"] = name;

    NetworkRequest::RequestConfig config;
    config.url = m_syncManager->getApiUrl(m_categoriesApiEndpoint);
    config.data = requestData;
    config.requiresAuth = true;
    // 移除重复的Authorization头设置，由addAuthHeader方法统一处理

    m_networkRequest.sendRequest(NetworkRequest::UpdateCategory, config);
}

void CategoryManager::deleteCategory(int id) {
    if (!m_userAuth.isLoggedIn()) {
        qWarning() << "用户未登录，无法删除类别";
        emit deleteCategoryCompleted(false, "用户未登录");
        return;
    }

    // 检查是否为默认类别
    CategorieItem *category = findCategoryById(id);
    if (category && !category->canBeDeleted()) {
        emit deleteCategoryCompleted(false, "不能删除系统默认类别");
        return;
    }

    if (!m_syncManager) {
        qWarning() << "同步管理器未初始化";
        emit deleteCategoryCompleted(false, "同步管理器未初始化");
        return;
    }

    QJsonObject requestData;
    requestData["action"] = "delete";
    requestData["id"] = id;

    NetworkRequest::RequestConfig config;
    config.url = m_syncManager->getApiUrl(m_categoriesApiEndpoint);
    config.data = requestData;
    config.requiresAuth = true;
    // 移除重复的Authorization头设置，由addAuthHeader方法统一处理

    m_networkRequest.sendRequest(NetworkRequest::DeleteCategory, config);
}

/**
 * @brief 处理网络请求完成
 * @param type 请求类型
 * @param response 响应数据
 */
void CategoryManager::onNetworkRequestCompleted(NetworkRequest::RequestType type, const QJsonObject &response) {
    switch (type) {
    case NetworkRequest::FetchCategories:
        handleFetchCategoriesSuccess(response);
        break;
    case NetworkRequest::CreateCategory:
    case NetworkRequest::UpdateCategory:
    case NetworkRequest::DeleteCategory:
        handleCategoryOperationSuccess(response);
        break;
    default:
        // 不是类别相关的请求，忽略
        break;
    }
}

/**
 * @brief 处理网络请求失败
 * @param type 请求类型
 * @param error 错误类型
 * @param message 错误消息
 */
void CategoryManager::onNetworkRequestFailed(NetworkRequest::RequestType type, NetworkRequest::NetworkError error,
                                             const QString &message) {
    QString errorMessage = QString("网络请求失败: %1, 错误类型: %2").arg(message).arg(error);

    switch (type) {
    case NetworkRequest::FetchCategories:
        qWarning() << "获取类别列表失败:" << errorMessage;
        emit fetchCategoriesCompleted(false, errorMessage);
        break;
    case NetworkRequest::CreateCategory:
        qWarning() << "创建类别失败:" << errorMessage;
        emit createCategoryCompleted(false, errorMessage);
        break;
    case NetworkRequest::UpdateCategory:
        qWarning() << "更新类别失败:" << errorMessage;
        emit updateCategoryCompleted(false, errorMessage);
        break;
    case NetworkRequest::DeleteCategory:
        qWarning() << "删除类别失败:" << errorMessage;
        emit deleteCategoryCompleted(false, errorMessage);
        break;
    default:
        // 不是类别相关的请求，忽略
        break;
    }
}

/**
 * @brief 处理获取类别列表成功响应
 * @param response 服务器响应数据
 */
void CategoryManager::handleFetchCategoriesSuccess(const QJsonObject &response) {
    if (response["success"].toBool()) {
        QJsonArray categoriesArray = response["categories"].toArray();
        updateCategoriesFromJson(categoriesArray);

        qDebug() << "成功获取待办类别列表:" << m_categories;
        emit fetchCategoriesCompleted(true, "获取待办类别列表成功");
    } else {
        QString errorMessage = response["message"].toString();
        qWarning() << "获取待办类别列表失败:" << errorMessage;
        emit fetchCategoriesCompleted(false, errorMessage);
    }
}

/**
 * @brief 处理类别操作成功响应
 * @param response 服务器响应数据
 */
void CategoryManager::handleCategoryOperationSuccess(const QJsonObject &response) {
    bool success = response["success"].toBool();
    QString message = response["message"].toString();

    if (success) {
        // 操作成功后重新获取类别列表
        fetchCategories();
    }

    emit categoryOperationCompleted(success, message);
}

/**
 * @brief 从JSON数组更新类别列表
 * @param categoriesArray JSON类别数组
 */
void CategoryManager::updateCategoriesFromJson(const QJsonArray &categoriesArray) {
    // 清空现有的类别项目（保留默认类别）
    m_categoryItems.clear();
    m_categories.clear();

    // 添加默认的"全部"选项
    m_categories << "全部";

    // 添加从服务器获取的类别
    for (const QJsonValue &value : categoriesArray) {
        QJsonObject categoryObj = value.toObject();

        int id = categoryObj["id"].toInt();
        QString name = categoryObj["name"].toString();
        QString userUuid = categoryObj["user_uuid"].toString();
        QString createdAtStr = categoryObj["created_at"].toString();
        QDateTime createdAt = QDateTime::fromString(createdAtStr, Qt::ISODate);

        if (!name.isEmpty()) {
            auto categoryItem = std::make_unique<CategorieItem>(id, name, QUuid::fromString(userUuid), createdAt, true);
            m_categoryItems.push_back(std::move(categoryItem));
            m_categories << name;
        }
    }

    // 添加默认的"未分类"选项（如果不存在）
    if (!m_categories.contains("未分类")) {
        auto defaultCategory =
            std::make_unique<CategorieItem>(1, "未分类", QUuid(), QDateTime::currentDateTime(), true);
        m_categoryItems.push_back(std::move(defaultCategory));
        m_categories << "未分类";
    }

    emit categoriesChanged();
}

/**
 * @brief 验证类别名称
 * @param name 类别名称
 * @return 如果有效返回true，否则返回false
 */
bool CategoryManager::isValidCategoryName(const QString &name) const {
    QString trimmedName = name.trimmed();
    return !trimmedName.isEmpty() && trimmedName.length() <= 50;
}