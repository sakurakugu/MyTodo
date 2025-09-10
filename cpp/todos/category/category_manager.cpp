/**
 * @file category_manager.cpp
 * @brief CategoryManager类的实现文件
 *
 * 该文件实现了CategoryManager类，用于管理待办事项的类别。
 * 从TodoManager中拆分出来，专门负责类别的CRUD操作和服务器同步。
 *
 * @author Sakurakugu
 * @date 2025-08-25 01:28:49(UTC+8) 周一
 * @change 2025-09-03 00:37:33(UTC+8) 周三
 * @version 0.4.0
 */

#include "category_manager.h"
#include "default_value.h"
#include "user_auth.h"
#include <QDebug>
#include <QJsonDocument>
#include <algorithm>

CategoryManager::CategoryManager(TodoSyncServer *syncManager, QObject *parent)
    : QAbstractListModel(parent),                      //
      m_isLoading(false),                              //
      m_debounceTimer(new QTimer(this)),               //
      m_networkRequest(NetworkRequest::GetInstance()), //
      m_syncManager(syncManager),                      //
      m_userAuth(UserAuth::GetInstance()),             //
      m_setting(Setting::GetInstance())                //
{

    // 连接网络请求信号
    connect(&m_networkRequest, &NetworkRequest::requestCompleted, this, &CategoryManager::onNetworkRequestCompleted);
    connect(&m_networkRequest, &NetworkRequest::requestFailed, this, &CategoryManager::onNetworkRequestFailed);

    // 设置防抖定时器
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(300);

    // 连接信号槽
    setupConnections(); // 300ms防抖

    // 从配置中加载服务器配置
    m_categoriesApiEndpoint = m_setting
                                  .get(QStringLiteral("server/categoriesApiEndpoint"),
                                       QString::fromStdString(std::string{DefaultValues::categoriesApiEndpoint}))
                                  .toString();

    // 添加默认类别
    addDefaultCategories();
}

CategoryManager::~CategoryManager() {
    m_categoryItems.clear();
    m_categories.clear();
}

/**
 * @brief 获取模型中的行数（类别数量）
 * @param parent 父索引，默认为无效索引（根）
 * @return 类别数量
 */
int CategoryManager::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;
    return m_categoryItems.size();
}

/**
 * @brief 获取指定索引和角色的数据
 * @param index 待获取数据的模型索引
 * @param role 数据角色
 * @return 请求的数据值
 */
QVariant CategoryManager::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || static_cast<size_t>(index.row()) >= m_categoryItems.size())
        return QVariant();

    return getItemData(m_categoryItems[index.row()].get(), role);
}

/**
 * @brief 获取角色名称映射，用于QML访问
 * @return 角色名称哈希表
 */
QHash<int, QByteArray> CategoryManager::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[UuidRole] = "uuid";
    roles[NameRole] = "name";
    roles[UserUuidRole] = "userUuid";
    roles[CreatedAtRole] = "createdAt";
    roles[SyncedRole] = "synced";
    return roles;
}

/**
 * @brief 设置指定索引和角色的数据
 * @param index 待设置数据的模型索引
 * @param value 新的数据值
 * @param role 数据角色
 * @return 设置成功返回true，否则返回false
 */
bool CategoryManager::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (!index.isValid() || static_cast<size_t>(index.row()) >= m_categoryItems.size())
        return false;

    CategorieItem *item = m_categoryItems[index.row()].get();
    bool changed = false;

    switch (role) {
    case NameRole:
        if (item->name() != value.toString()) {
            item->setName(value.toString());
            changed = true;
        }
        break;
    default:
        return false;
    }

    if (changed) {
        emit dataChanged(index, index, {role});
        emit categoriesChanged();
    }

    return changed;
}

/**
 * @brief 获取类别项目列表
 * @return 类别项目的智能指针向量
 */
QStringList CategoryManager::getCategories() const {
    return m_categories;
}

/**
 * @brief 根据索引获取类别项目
 * @param index 索引
 * @return 类别项目指针，如果索引无效返回nullptr
 */
CategorieItem *CategoryManager::getCategoryAt(int index) const {
    if (index < 0 || static_cast<size_t>(index) >= m_categoryItems.size())
        return nullptr;
    return m_categoryItems[index].get();
}

/**
 * @brief 根据角色获取项目数据
 * @param item 类别项目指针
 * @param role 数据角色
 * @return 请求的数据值
 */
QVariant CategoryManager::getItemData(const CategorieItem *item, int role) const {
    if (!item)
        return QVariant();

    switch (role) {
    case IdRole:
        return item->id();
    case UuidRole:
        return item->uuid();
    case NameRole:
        return item->name();
    case UserUuidRole:
        return item->userUuid();
    case CreatedAtRole:
        return item->createdAt();
    case SyncedRole:
        return item->synced();
    default:
        return QVariant();
    }
}

/**
 * @brief 获取指定CategorieItem的模型索引
 * @param categoryItem 类别项目指针
 * @return 模型索引
 */
QModelIndex CategoryManager::indexFromItem(CategorieItem *categoryItem) const {
    if (!categoryItem)
        return QModelIndex();

    for (size_t i = 0; i < m_categoryItems.size(); ++i) {
        if (m_categoryItems[i].get() == categoryItem) {
            return createIndex(static_cast<int>(i), 0);
        }
    }
    return QModelIndex();
}

/**
 * @brief 从名称获取角色
 * @param name 角色名称
 * @return 对应的角色枚举值
 */
CategoryManager::CategoryRoles CategoryManager::roleFromName(const QString &name) const {
    if (name == "id")
        return IdRole;
    if (name == "uuid")
        return UuidRole;
    if (name == "name")
        return NameRole;
    if (name == "userUuid")
        return UserUuidRole;
    if (name == "createdAt")
        return CreatedAtRole;
    if (name == "synced")
        return SyncedRole;
    return IdRole; // 默认返回IdRole
}

/**
 * @brief 发射操作完成信号
 * @param operation 操作名称
 * @param success 是否成功
 * @param message 消息
 */
void CategoryManager::emitOperationCompleted(const QString &operation, bool success, const QString &message) {
    emit operationCompleted(operation, success, message);
}

/**
 * @brief 开始模型更新
 */
void CategoryManager::beginModelUpdate() {
    beginResetModel();
}

/**
 * @brief 结束模型更新
 */
void CategoryManager::endModelUpdate() {
    endResetModel();
    emit categoriesChanged();
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
 * @brief 是否可以同步
 * @return 如果可以同步返回true，否则返回false
 */
bool CategoryManager::isCanSync() {
    if (!m_userAuth.isLoggedIn()) {
        qWarning() << "用户未登录，无法推送类别列表";
        return false;
    }

    if (!m_syncManager) {
        qWarning() << "同步管理器未初始化";
        return false;
    }
    return true;
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
 * @brief 根据UUID查找类别项目
 * @param uuid 类别UUID
 * @return 找到的类别项目指针，如果未找到则返回nullptr
 */
CategorieItem *CategoryManager::findCategoryByUuid(const QUuid &uuid) const {
    auto it = std::find_if(m_categoryItems.begin(), m_categoryItems.end(),
                           [uuid](const std::unique_ptr<CategorieItem> &item) { return item->uuid() == uuid; });
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
    beginModelUpdate();

    // 清空现有类别
    m_categoryItems.clear();
    m_categories.clear();

    // 添加默认的"未分类"选项
    auto defaultCategory = std::make_unique<CategorieItem>( //
        1,                                                  //
        QUuid::createUuid(),                                //
        "未分类",                                           //
        m_userAuth.getUuid(),                               //
        QDateTime::currentDateTime(),                       //
        false                                               //
    );
    m_categoryItems.push_back(std::move(defaultCategory));
    m_categories << "未分类";

    endModelUpdate();
}

/**
 * @brief 清空所有类别
 */
void CategoryManager::clearCategories() {
    beginModelUpdate();
    m_categoryItems.clear();
    m_categories.clear();
    endModelUpdate();
}

void CategoryManager::fetchCategories() {

    if (!isCanSync()) {
        return;
    }

    QJsonObject requestData;
    requestData["action"] = "fetch";

    NetworkRequest::RequestConfig config;
    config.url = m_syncManager->getApiUrl(m_categoriesApiEndpoint);
    config.method = "GET"; // 获取分类使用GET方法
    config.data = requestData;
    config.requiresAuth = true;

    qDebug() << "开始获取类别列表...";
    m_networkRequest.sendRequest(NetworkRequest::FetchCategories, config);
}

/**
 * @brief 推送类别列表到服务器
 */
void CategoryManager::pushCategories() {

    if (!isCanSync()) {
        return;
    }
}

void CategoryManager::createCategory(const QString &name) {
    qDebug() << "=== 开始创建类别 ===" << name;

    if (!isValidCategoryName(name)) {
        emitOperationCompleted("createCategory", false, "类别名称不能为空或过长");
        return;
    }

    if (categoryExists(name)) {
        emitOperationCompleted("createCategory", false, "类别名称已存在");
        return;
    }

    // 创建新的类别项目
    auto newCategory = std::make_unique<CategorieItem>(m_categoryItems.size() + 1, // 简单的ID分配
                                                       QUuid::createUuid(), name, m_userAuth.getUuid(),
                                                       QDateTime::currentDateTime(), false);

    // 添加到列表中
    m_categoryItems.push_back(std::move(newCategory));
    m_categories << name;

    qDebug() << "类别创建成功:" << name;
    emit categoriesChanged();
    emitOperationCompleted("createCategory", true, "类别创建成功");

    createCategoryWithServer(name);
}

void CategoryManager::updateCategory(const QString &name, const QString &newName) {
    qDebug() << "=== 开始更新本地类别 ===" << name << "->" << newName;

    if (!isValidCategoryName(newName)) {
        emitOperationCompleted("updateCategory", false, "新类别名称不能为空或过长");
        return;
    }

    if (name == "未分类") {
        emitOperationCompleted("updateCategory", false, "不能更新系统默认类别 \"未分类\"");
        return;
    }

    // 检查原类别是否存在
    CategorieItem *existingCategory = findCategoryByName(name);
    if (!existingCategory) {
        qWarning() << "要更新的类别不存在:" << name;
        emitOperationCompleted("updateCategory", false, "要更新的类别不存在");
        return;
    }

    // 检查新名称是否与其他类别重复（排除自身）
    CategorieItem *duplicateCategory = findCategoryByName(newName);
    if (duplicateCategory && duplicateCategory->id() != existingCategory->id()) {
        qWarning() << "新类别名称已存在:" << newName;
        emitOperationCompleted("updateCategory", false, "类别名称已存在");
        return;
    }

    // 更新类别名称
    existingCategory->setName(newName);

    // 更新缓存列表
    int index = m_categories.indexOf(name);
    if (index != -1) {
        m_categories[index] = newName;
    }

    qDebug() << "本地类别更新成功:" << name << "->" << newName;
    emit categoriesChanged();
    emitOperationCompleted("updateCategory", true, "类别更新成功");

    updateCategoryWithServer(name, newName);
}

void CategoryManager::deleteCategory(const QString &name) {
    qDebug() << "=== 开始本地类别 ===" << name;

    if (name == "未分类") {
        emitOperationCompleted("deleteCategory", false, "不能删除系统默认类别 \"未分类\"");
        return;
    }

    // 查找要删除的类别
    CategorieItem *category = findCategoryByName(name);
    if (!category) {
        qWarning() << "要删除的类别不存在:" << name;
        emitOperationCompleted("deleteCategory", false, "要删除的类别不存在");
        return;
    }

    // 检查是否可以删除
    if (!category->canBeDeleted()) {
        emitOperationCompleted("deleteCategory", false, "不能删除系统默认类别");
        return;
    }

    // 从列表中移除
    auto it = std::find_if(m_categoryItems.begin(), m_categoryItems.end(),
                           [&name](const std::unique_ptr<CategorieItem> &item) { return item->name() == name; });

    if (it != m_categoryItems.end()) {
        m_categoryItems.erase(it);
    }

    // 从缓存列表中移除
    m_categories.removeAll(name);

    qDebug() << "本地类别删除成功:" << name;
    emit categoriesChanged();
    emitOperationCompleted("deleteCategory", true, "类别删除成功");

    deleteCategoryWithServer(name);
}

void CategoryManager::createCategoryWithServer(const QString &name) {

    if (!isCanSync()) {
        return;
    }

    if (name.trimmed().isEmpty()) {
        const QString error = "类别名称不能为空";
        qWarning() << error;
        emitOperationCompleted("createWithServer", false, error);
        return;
    }

    // 检查是否已存在同名类别
    if (m_categories.contains(name.trimmed())) {
        const QString error = "类别名称已存在";
        qWarning() << error;
        emitOperationCompleted("createWithServer", false, error);
        return;
    }

    QJsonObject requestData;
    requestData["action"] = "create";
    requestData["name"] = name.trimmed();

    NetworkRequest::RequestConfig config;
    config.url = m_syncManager->getApiUrl(m_categoriesApiEndpoint);
    config.method = "POST"; // 创建分类使用POST方法
    config.data = requestData;
    config.requiresAuth = true;

    qDebug() << "开始创建类别:" << name.trimmed();
    m_networkRequest.sendRequest(NetworkRequest::CreateCategory, config);
}

void CategoryManager::updateCategoryWithServer(const QString &name, const QString &newName) {

    if (!isCanSync()) {
        return;
    }

    if (name == "未分类") {
        const QString error = "不能更新系统默认类别 \"未分类\"";
        emitOperationCompleted("updateWithServer", false, error);
        return;
    }

    // 检查原类别是否存在
    CategorieItem *existingCategory = findCategoryByName(name);
    if (!existingCategory) {
        const QString error = "要更新的类别不存在";
        qWarning() << error << name;
        emitOperationCompleted("updateWithServer", false, error);
        return;
    }

    // 检查新名称是否与其他类别重复（排除自身）
    CategorieItem *duplicateCategory = findCategoryByName(newName);
    if (duplicateCategory && duplicateCategory->id() != existingCategory->id()) {
        const QString error = "类别名称已存在";
        qWarning() << error << newName;
        emitOperationCompleted("updateWithServer", false, error);
        return;
    }

    if (newName.trimmed().isEmpty()) {
        const QString error = "新类别名称不能为空";
        qWarning() << error;
        emitOperationCompleted("updateWithServer", false, error);
        return;
    }

    QJsonObject requestData;
    requestData["action"] = "update";
    requestData["uuid"] = existingCategory->uuid().toString(QUuid::WithoutBraces);
    requestData["name"] = newName.trimmed();

    qDebug() << "开始更新类别 - UUID:" << existingCategory->uuid().toString(QUuid::WithoutBraces) << "原名称:" << name
             << "新名称:" << newName.trimmed();

    NetworkRequest::RequestConfig config;
    config.url = m_syncManager->getApiUrl(m_categoriesApiEndpoint);
    config.method = "PUT"; // 更新分类使用PUT方法
    config.data = requestData;
    config.requiresAuth = true;

    m_networkRequest.sendRequest(NetworkRequest::UpdateCategory, config);
}

void CategoryManager::deleteCategoryWithServer(const QString &name) {

    if (!isCanSync()) {
        return;
    }

    // 检查是否为默认类别
    CategorieItem *category = findCategoryByName(name);
    if (!category) {
        const QString error = "要删除的类别不存在";
        qWarning() << error << name;
        emitOperationCompleted("deleteWithServer", false, error);
        return;
    }

    if (!category->canBeDeleted()) {
        const QString error = "不能删除系统默认类别";
        emitOperationCompleted("deleteWithServer", false, error);
        return;
    }

    qDebug() << "开始删除类别:" << name << "UUID:" << category->uuid().toString(QUuid::WithoutBraces);

    // DELETE 请求通过 URL 参数传递 UUID，不使用请求体
    NetworkRequest::RequestConfig config;
    config.url =
        m_syncManager->getApiUrl(m_categoriesApiEndpoint) + "?uuid=" + category->uuid().toString(QUuid::WithoutBraces);
    config.method = "DELETE"; // 删除分类使用DELETE方法
    config.requiresAuth = true;

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
        qWarning() << "未知的网络请求类型:" << type;
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

    qWarning() << "网络请求失败:" << errorMessage;

    switch (type) {
    case NetworkRequest::FetchCategories:
        qWarning() << "获取类别列表失败:" << errorMessage;
        emitOperationCompleted("fetch", false, errorMessage);
        break;
    case NetworkRequest::CreateCategory:
        qWarning() << "创建类别失败:" << errorMessage;
        emitOperationCompleted("createWithServer", false, errorMessage);
        break;
    case NetworkRequest::UpdateCategory:
        qWarning() << "更新类别失败:" << errorMessage;
        emitOperationCompleted("updateWithServer", false, errorMessage);
        break;
    case NetworkRequest::DeleteCategory:
        qWarning() << "删除类别失败:" << errorMessage;
        emitOperationCompleted("deleteWithServer", false, errorMessage);
        break;
    default:
        qWarning() << "未知的网络请求类型:" << type;
        break;
    }
}

/**
 * @brief 处理获取类别列表成功响应
 * @param response 服务器响应数据
 */
void CategoryManager::handleFetchCategoriesSuccess(const QJsonObject &response) {
    // 添加调试信息，输出完整的服务器响应
    qDebug() << "服务器响应内容:" << QJsonDocument(response).toJson(QJsonDocument::Compact);

    QJsonArray categoriesArray;

    // 检查是否是标准响应格式
    if (response.contains("success")) {
        // 标准响应格式
        if (response["success"].toBool()) {
            // 修复：从data字段中获取categories数组
            QJsonObject dataObj = response["data"].toObject();
            categoriesArray = dataObj["categories"].toArray();

            qDebug() << "成功获取待办类别列表（标准格式）:" << categoriesArray.size() << "个类别";
            updateCategoriesFromJson(categoriesArray);
            emitOperationCompleted("fetch", true, "获取待办类别列表成功");
        } else {
            // 修复：错误响应中消息字段是"error"，不是"message"
            QString errorMessage =
                response.contains("error") ? response["error"].toString() : response["message"].toString();
            if (errorMessage.isEmpty()) {
                errorMessage = "获取待办类别列表失败";
            }
            qWarning() << "获取待办类别列表失败:" << errorMessage;
            qWarning() << "响应包含的字段:" << response.keys();
            emitOperationCompleted("fetch", false, errorMessage);
        }
    } else {
        // 非标准响应格式，直接处理categories数组
        if (response.contains("categories")) {
            categoriesArray = response["categories"].toArray();
            updateCategoriesFromJson(categoriesArray);
            qWarning() << "成功获取待办类别列表（非标准格式）:" << categoriesArray.size() << "个类别";
            emitOperationCompleted("fetch", true, "获取待办类别列表成功");
        } else {
            QString errorMessage = "响应格式错误：缺少categories字段";
            qWarning() << "响应中没有找到categories字段";
            qWarning() << "响应包含的字段:" << response.keys();
            emitOperationCompleted("fetch", false, errorMessage);
        }
    }
}

/**
 * @brief 处理类别操作成功响应
 * @param response 服务器响应数据
 */
void CategoryManager::handleCategoryOperationSuccess(const QJsonObject &response) {
    bool success = response["success"].toBool();
    // 修复：根据响应类型获取正确的消息字段
    QString message =
        success ? response["message"].toString()
                : (response.contains("error") ? response["error"].toString() : response["message"].toString());

    qDebug() << "类别操作结果:" << success << "消息:" << message;

    if (success) {
        // 操作成功后重新获取类别列表
        qDebug() << "类别操作成功，重新获取类别列表...";
        fetchCategories();

        // 立即发射信号通知UI更新
        emit categoriesChanged();
    } else {
    }

    emitOperationCompleted("categoryOperation", success, message);
}

/**
 * @brief 从JSON数组更新类别列表
 * @param categoriesArray JSON类别数组
 */
void CategoryManager::updateCategoriesFromJson(const QJsonArray &categoriesArray) {
    beginModelUpdate();

    // 清空现有的类别项目（保留默认类别）
    m_categoryItems.clear();
    m_categories.clear();

    // 添加从服务器获取的类别
    for (const QJsonValue &value : categoriesArray) {
        QJsonObject categoryObj = value.toObject();

        int id = categoryObj["id"].toInt();
        QString uuidStr = categoryObj["uuid"].toString();
        QString name = categoryObj["name"].toString();
        QString userUuid = categoryObj["user_uuid"].toString();
        QString createdAtStr = categoryObj["created_at"].toString();
        QDateTime createdAt = QDateTime::fromString(createdAtStr, Qt::ISODate);

        if (!name.isEmpty() && !uuidStr.isEmpty()) {
            auto categoryItem = std::make_unique<CategorieItem>(id, QUuid::fromString(uuidStr), name,
                                                                QUuid::fromString(userUuid), createdAt, true);
            m_categoryItems.push_back(std::move(categoryItem));
            m_categories << name;
        }
    }

    // 添加默认的"未分类"选项（如果不存在）
    if (!m_categories.contains("未分类")) {
        auto defaultCategory = std::make_unique<CategorieItem>(1, QUuid::createUuid(), "未分类", QUuid(),
                                                               QDateTime::currentDateTime(), true);
        m_categoryItems.push_back(std::move(defaultCategory));
        m_categories << "未分类";
    }

    endModelUpdate();

    qDebug() << "更新类别列表完成，当前类别:" << m_categories;
}

/**
 * @brief 设置信号槽连接
 */
void CategoryManager::setupConnections() {
    // 连接防抖定时器
    if (m_debounceTimer) {
        connect(m_debounceTimer, &QTimer::timeout, this, &CategoryManager::fetchCategories);
    }

    // 连接网络请求信号
    connect(&m_networkRequest, &NetworkRequest::requestCompleted, this, &CategoryManager::onNetworkRequestCompleted);
    connect(&m_networkRequest, &NetworkRequest::requestFailed, this, &CategoryManager::onNetworkRequestFailed);

    qDebug() << "CategoryManager信号槽连接完成";
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