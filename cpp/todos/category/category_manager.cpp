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
#include <QDebug>
#include <QJsonDocument>
#include <algorithm>

CategoryManager::CategoryManager(QObject *parent)
    : QAbstractListModel(parent),                             //
      m_syncServer(std::make_unique<CategorySyncServer>()),   //
      m_dataStorage(std::make_unique<CategoryDataStorage>()), //
      m_setting(Setting::GetInstance()),                      //
      m_userAuth(UserAuth::GetInstance())                     //
{

    // 连接同步服务器信号
    connect(m_syncServer.get(), &CategorySyncServer::categoriesUpdatedFromServer, this,
            &CategoryManager::onCategoriesUpdatedFromServer);

    // 从存储加载类别数据
    loadCategories();
}

CategoryManager::~CategoryManager() {
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
    roles[UpdatedAtRole] = "updatedAt";
    roles[LastModifiedAtRole] = "lastModifiedAt";
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
    case UpdatedAtRole:
        return item->updatedAt();
    case LastModifiedAtRole:
        return item->lastModifiedAt();
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
    if (name == "updatedAt")
        return UpdatedAtRole;
    if (name == "lastModifiedAt")
        return LastModifiedAtRole;
    if (name == "synced")
        return SyncedRole;
    return IdRole; // 默认返回IdRole
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
void CategoryManager::添加默认类别() {

    // 使用数据存储创建默认类别
    m_dataStorage->创建默认类别(m_categoryItems, m_userAuth.getUuid());

    // 更新缓存列表
    m_categories.clear();
    m_categories << "未分类";
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

void CategoryManager::createCategory(const QString &name) {
    qDebug() << "=== 开始创建类别 ===" << name;

    if (!是否是有效名称(name)) {
        return;
    }

    if (categoryExists(name)) {
        return;
    }

    m_dataStorage->新增类别(m_categoryItems, name, m_userAuth.getUuid());
    m_categories << name;

    m_syncServer->设置未同步的对象(m_categoryItems);
    m_syncServer->与服务器同步(BaseSyncServer::UploadOnly);

    qDebug() << "类别创建成功:" << name;
    emit categoriesChanged();
}

void CategoryManager::updateCategory(const QString &name, const QString &newName) {
    qDebug() << "=== 开始更新本地类别 ===" << name << "->" << newName;

    if (!是否是有效名称(newName)) {
        return;
    }

    if (name == "未分类") {
        return;
    }

    // 检查原类别是否存在
    CategorieItem *nowCategory = findCategoryByName(name);
    if (nowCategory == nullptr) {
        qWarning() << "待更新的类别不存在:" << name;
        return;
    }

    // 检查新名称是否与其他类别重复（排除自身）
    CategorieItem *newCategory = findCategoryByName(newName);
    if (newCategory != nullptr) {
        qWarning() << "该类别已存在:" << newName;
        return;
    }

    // 更新类别名称
    m_dataStorage->更新类别(m_categoryItems, name, newName);

    // 更新缓存列表
    int index = m_categories.indexOf(name);
    if (index != -1) {
        m_categories[index] = newName;
    }

    m_syncServer->设置未同步的对象(m_categoryItems);
    m_syncServer->与服务器同步(BaseSyncServer::UploadOnly);

    qDebug() << "本地类别更新成功:" << name << "->" << newName;
    emit categoriesChanged();
}

void CategoryManager::deleteCategory(const QString &name) {
    qDebug() << "=== 开始本地类别 ===" << name;

    if (name == "未分类") {
        return;
    }

    // 查找要删除的类别
    CategorieItem *category = findCategoryByName(name);
    if (!category) {
        qWarning() << "要删除的类别不存在:" << name;
        return;
    }

    // 检查是否可以删除
    if (!category->canBeDeleted()) {
        return;
    }

    // 从列表中移除
    m_dataStorage->软删除类别(m_categoryItems, name);

    // 从缓存列表中移除
    m_categories.removeAll(name);

    m_syncServer->设置未同步的对象(m_categoryItems);
    m_syncServer->与服务器同步(BaseSyncServer::UploadOnly);

    qDebug() << "本地类别删除成功:" << name;
    emit categoriesChanged();
}

// 同步相关方法实现
void CategoryManager::syncWithServer() {
    if (m_syncServer) {
        m_syncServer->设置未同步的对象(m_categoryItems);
        m_syncServer->与服务器同步(BaseSyncServer::Bidirectional);
    }
}

bool CategoryManager::isSyncing() const {
    return m_syncServer ? m_syncServer->isSyncing() : false;
}

/**
 * @brief 处理从服务器更新的类别数据
 * @param categoriesArray 类别数据数组
 */
void CategoryManager::onCategoriesUpdatedFromServer(const QJsonArray &categoriesArray) {
    qDebug() << "从同步服务器接收到类别更新:" << categoriesArray.size() << "个类别";
    updateCategoriesFromJson(categoriesArray);
}

void CategoryManager::onLocalChangesUploaded(const std::vector<CategorieItem *> &m_unsyncedItems) {
    for (CategorieItem *item : m_unsyncedItems) {
        if (item->synced() != 3) { // 保留已删除状态
            m_dataStorage->更新同步状态(m_categoryItems, item->uuid());
        } else {
            m_dataStorage->删除类别(m_categoryItems, item->name());
        }
    }
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
        QString updatedAtStr = categoryObj["updated_at"].toString();
        QString lastModifiedAtStr = categoryObj["last_modified_at"].toString();
        QDateTime createdAt = QDateTime::fromString(createdAtStr, Qt::ISODate);
        QDateTime updatedAt = QDateTime::fromString(updatedAtStr, Qt::ISODate);
        QDateTime lastModifiedAt = QDateTime::fromString(lastModifiedAtStr, Qt::ISODate);

        if (!name.isEmpty() && !uuidStr.isEmpty()) {
            auto categoryItem =
                std::make_unique<CategorieItem>(id, QUuid::fromString(uuidStr), name, QUuid::fromString(userUuid),
                                                createdAt, updatedAt, lastModifiedAt, true);
            m_categoryItems.push_back(std::move(categoryItem));
            m_categories << name;
        }
    }

    // 添加默认的"未分类"选项（如果不存在）
    if (!m_categories.contains("未分类")) {
        auto defaultCategory = std::make_unique<CategorieItem>( //
            1,                                                  //
            QUuid::createUuid(),                                //
            "未分类",                                           //
            m_userAuth.getUuid(),                               //
            QDateTime::currentDateTime(),                       //
            QDateTime::currentDateTime(),                       //
            QDateTime::currentDateTime(),                       //
            0                                                   //
        );
        m_categoryItems.push_back(std::move(defaultCategory));
    }

    endModelUpdate();

    qDebug() << "更新类别列表完成，当前类别:" << m_categories;
}

/**
 * @brief 从存储加载类别
 */
void CategoryManager::loadCategories() {
    qDebug() << "开始从存储加载类别数据";

    if (!m_dataStorage) {
        qWarning() << "数据存储对象为空，无法加载类别";
        return;
    }

    beginModelUpdate();

    // 清空现有数据
    m_categoryItems.clear();
    m_categories.clear();

    // 从存储加载
    bool success = m_dataStorage->加载类别(m_categoryItems);

    if (success) {
        // 更新缓存列表
        for (const auto &category : m_categoryItems) {
            if (category && category->synced() != 3) { // 只缓存未删除的类别
                m_categories << category->name();
            }
        }

        // 将类别数据传递给同步服务器
        if (m_syncServer) {
            m_syncServer->设置未同步的对象(m_categoryItems);
        }
    } else {
        qWarning() << "从存储加载类别失败";
    }

    // 如果没有加载到数据，添加默认类别
    if (m_categoryItems.empty()) {
        添加默认类别();
    }

    endModelUpdate();
}

/**
 * @brief 验证类别名称
 * @param name 类别名称
 * @return 如果有效返回true，否则返回false
 */
bool CategoryManager::是否是有效名称(const QString &name) const {
    QString trimmedName = name.trimmed();
    return !trimmedName.isEmpty() && trimmedName.length() <= 50;
}