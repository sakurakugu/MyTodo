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
    : QAbstractListModel(parent),                   //
      m_debounceTimer(new QTimer(this)),            //
      m_syncServer(new CategorySyncServer()),   //
      m_dataStorage(new CategoryDataStorage()), //
      m_setting(Setting::GetInstance()),            //
      m_userAuth(UserAuth::GetInstance())           //
{

    // 连接同步服务器信号
    connect(m_syncServer, &CategorySyncServer::categoriesUpdatedFromServer, this,
            &CategoryManager::onCategoriesUpdatedFromServer);

    // 连接数据存储信号
    if (m_dataStorage) {
        connect(m_dataStorage, &CategoryDataStorage::dataOperationCompleted, this,
                [this](bool success, const QString &message) {
                    if (success) {
                        qDebug() << "数据存储操作成功:" << message;
                    } else {
                        qWarning() << "数据存储操作失败:" << message;
                        emit errorOccurred(message);
                    }
                });
    }

    // 设置防抖定时器
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(300);

    // 连接信号槽
    setupConnections(); // 300ms防抖

    // 从存储加载类别数据
    loadCategoriesFromStorage();

    // 如果没有加载到数据，添加默认类别
    if (m_categoryItems.empty()) {
        addDefaultCategories();
    }
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

    // 使用数据存储创建默认类别
    if (m_dataStorage) {
        m_dataStorage->createDefaultCategories(m_categoryItems, m_userAuth.getUuid());
    } else {
        // 备用方案：直接创建
        m_categoryItems.clear();
        auto defaultCategory = std::make_unique<CategorieItem>(1, QUuid::createUuid(), "未分类", m_userAuth.getUuid(),
                                                               QDateTime::currentDateTime(), false);
        m_categoryItems.push_back(std::move(defaultCategory));
    }

    // 更新缓存列表
    m_categories.clear();
    for (const auto &category : m_categoryItems) {
        if (category) {
            m_categories << category->name();
        }
    }

    endModelUpdate();

    // 将类别数据传递给同步服务器
    if (m_syncServer) {
        m_syncServer->setCategoryItems(m_categoryItems);
    }

    // 保存到存储
    saveCategoriesStorage();
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

    // 保存到本地存储
    saveCategoriesStorage();

    // 将更新后的类别数据传递给同步服务器
    if (m_syncServer) {
        m_syncServer->setCategoryItems(m_categoryItems);
        m_syncServer->createCategoryWithServer(name);
    }
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

    // 保存到本地存储
    saveCategoriesStorage();

    // 将更新后的类别数据传递给同步服务器
    if (m_syncServer) {
        m_syncServer->setCategoryItems(m_categoryItems);
        m_syncServer->updateCategoryWithServer(name, newName);
    }
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

    // 保存到本地存储
    saveCategoriesStorage();

    // 将更新后的类别数据传递给同步服务器
    if (m_syncServer) {
        m_syncServer->setCategoryItems(m_categoryItems);
        m_syncServer->deleteCategoryWithServer(name);
    }
}

// 同步相关方法实现
void CategoryManager::syncWithServer() {
    if (m_syncServer) {
        m_syncServer->setCategoryItems(m_categoryItems);
        m_syncServer->syncWithServer(BaseSyncServer::Bidirectional);
    }
}

void CategoryManager::fetchCategoriesFromServer() {
    if (m_syncServer) {
        m_syncServer->setCategoryItems(m_categoryItems);
        m_syncServer->fetchCategories();
    }
}

void CategoryManager::pushCategoriesToServer() {
    if (m_syncServer) {
        m_syncServer->setCategoryItems(m_categoryItems);
        m_syncServer->pushCategories();
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
    // 连接防抖定时器（如果需要的话）
    if (m_debounceTimer) {
        // 防抖定时器暂时不连接到任何槽函数
        // connect(m_debounceTimer, &QTimer::timeout, this, &CategoryManager::someFunction);
    }

    qDebug() << "CategoryManager信号槽连接完成";
}

/**
 * @brief 从存储加载类别
 */
void CategoryManager::loadCategoriesFromStorage() {
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
    bool success = m_dataStorage->loadFromLocalStorage(m_categoryItems);

    if (success) {
        // 更新缓存列表
        for (const auto &category : m_categoryItems) {
            if (category) {
                m_categories << category->name();
            }
        }
        qDebug() << "从存储加载类别成功，共" << m_categoryItems.size() << "个类别";

        // 将类别数据传递给同步服务器
        if (m_syncServer) {
            m_syncServer->setCategoryItems(m_categoryItems);
        }
    } else {
        qWarning() << "从存储加载类别失败";
    }

    endModelUpdate();
}

/**
 * @brief 保存类别到存储
 */
void CategoryManager::saveCategoriesStorage() {
    qDebug() << "开始保存类别数据到存储";

    if (!m_dataStorage) {
        qWarning() << "数据存储对象为空，无法保存类别";
        return;
    }

    bool success = m_dataStorage->saveToLocalStorage(m_categoryItems);

    if (success) {
        qDebug() << "保存类别到存储成功，共" << m_categoryItems.size() << "个类别";
    } else {
        qWarning() << "保存类别到存储失败";
    }
}

/**
 * @brief 导入类别数据
 * @param filePath 文件路径
 * @param format 文件格式
 */
void CategoryManager::importCategories(const QString &filePath, CategoryDataStorage::FileFormat format) {
    qDebug() << "开始导入类别数据，文件:" << filePath << "格式:" << static_cast<int>(format);

    if (!m_dataStorage) {
        qWarning() << "数据存储对象为空，无法导入类别";
        emitOperationCompleted("importCategories", false, "数据存储对象为空");
        return;
    }

    std::vector<std::unique_ptr<CategorieItem>> importedCategories;
    bool success = false;

    if (format == CategoryDataStorage::FileFormat::TOML) {
        success = m_dataStorage->importFromToml(filePath, importedCategories);
    } else if (format == CategoryDataStorage::FileFormat::JSON) {
        success = m_dataStorage->importFromJson(filePath, importedCategories);
    }

    if (success && !importedCategories.empty()) {
        beginModelUpdate();

        // 合并导入的类别（避免重复）
        for (auto &importedCategory : importedCategories) {
            if (importedCategory && !m_categories.contains(importedCategory->name())) {
                m_categories.append(importedCategory->name());
                m_categoryItems.push_back(std::move(importedCategory));
            }
        }

        endModelUpdate();

        // 保存到本地存储
        saveCategoriesStorage();

        emit categoriesChanged();
        emitOperationCompleted("importCategories", true, QString("成功导入 %1 个类别").arg(importedCategories.size()));
        qDebug() << "类别导入成功，共" << importedCategories.size() << "个类别";
    } else {
        emitOperationCompleted("importCategories", false, "导入失败");
        qWarning() << "类别导入失败";
    }
}

/**
 * @brief 导出类别数据
 * @param filePath 文件路径
 * @param format 文件格式
 */
void CategoryManager::exportCategories(const QString &filePath, CategoryDataStorage::FileFormat format) {
    qDebug() << "开始导出类别数据，文件:" << filePath << "格式:" << static_cast<int>(format);

    if (!m_dataStorage) {
        qWarning() << "数据存储对象为空，无法导出类别";
        emitOperationCompleted("exportCategories", false, "数据存储对象为空");
        return;
    }

    bool success = false;

    if (format == CategoryDataStorage::FileFormat::TOML) {
        success = m_dataStorage->exportToToml(filePath, m_categoryItems);
    } else if (format == CategoryDataStorage::FileFormat::JSON) {
        success = m_dataStorage->exportToJson(filePath, m_categoryItems);
    }

    if (success) {
        emitOperationCompleted("exportCategories", true, "导出成功");
        qDebug() << "类别导出成功，共" << m_categoryItems.size() << "个类别";
    } else {
        emitOperationCompleted("exportCategories", false, "导出失败");
        qWarning() << "类别导出失败";
    }
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