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

CategoryManager::CategoryManager(UserAuth &userAuth, QObject *parent)
    : QAbstractListModel(parent),                                   //
      m_syncServer(new CategorySyncServer(userAuth,this)), //
      m_dataStorage(new CategoryDataStorage(this)),       //
      m_userAuth(userAuth)                                          //
{

    // 连接同步服务器信号
    connect(m_syncServer, &CategorySyncServer::categoriesUpdatedFromServer, this,
            &CategoryManager::onCategoriesUpdatedFromServer);

    // 从存储加载类别数据
    加载类别();
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
    QStringList categories;
    for (const auto &item : m_categoryItems) {
        if (item && item->synced() != 3) // 只返回未删除的类别
            categories << item->name();
    }
    return categories;
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
 * @brief 开始模型更新
 */
void CategoryManager::开始更新模型() {
    beginResetModel();
}

/**
 * @brief 结束模型更新
 */
void CategoryManager::结束更新模型() {
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
CategorieItem *CategoryManager::寻找类别(const QString &name) const {
    auto it = std::find_if(m_categoryItems.begin(), m_categoryItems.end(),
                           [&name](const std::unique_ptr<CategorieItem> &item) { return item->name() == name; });
    return (it != m_categoryItems.end()) ? it->get() : nullptr;
}

/**
 * @brief 根据ID查找类别项目
 * @param id 类别ID
 * @return 类别项目指针，如果未找到返回nullptr
 */
CategorieItem *CategoryManager::寻找类别(int id) const {
    auto it = std::find_if(m_categoryItems.begin(), m_categoryItems.end(),
                           [id](const std::unique_ptr<CategorieItem> &item) { return item->id() == id; });
    return (it != m_categoryItems.end()) ? it->get() : nullptr;
}

/**
 * @brief 根据UUID查找类别项目
 * @param uuid 类别UUID
 * @return 找到的类别项目指针，如果未找到则返回nullptr
 */
CategorieItem *CategoryManager::寻找类别(const QUuid &uuid) const {
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
    return 寻找类别(name) != nullptr;
}

/**
 * @brief 添加默认类别
 */
void CategoryManager::添加默认类别() {
    // 使用数据存储创建默认类别
    m_dataStorage->创建默认类别(m_categoryItems, m_userAuth.getUuid());
}

void CategoryManager::createCategory(const QString &name) {
    qDebug() << "=== 开始创建类别 ===" << name;

    if (!是否是有效名称(name)) {
        return;
    }

    if (categoryExists(name)) {
        return;
    }

    开始更新模型();
    m_dataStorage->新增类别(m_categoryItems, name, m_userAuth.getUuid());

    m_syncServer->设置未同步的对象(m_categoryItems);
    m_syncServer->与服务器同步(BaseSyncServer::UploadOnly);

    qDebug() << "类别创建成功:" << name;
    结束更新模型();
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
    CategorieItem *nowCategory = 寻找类别(name);
    if (nowCategory == nullptr) {
        qWarning() << "待更新的类别不存在:" << name;
        return;
    }

    // 检查新名称是否与其他类别重复（排除自身）
    CategorieItem *newCategory = 寻找类别(newName);
    if (newCategory != nullptr) {
        qWarning() << "该类别已存在:" << newName;
        return;
    }

    开始更新模型();
    // 更新类别名称
    m_dataStorage->更新类别(m_categoryItems, name, newName);

    m_syncServer->设置未同步的对象(m_categoryItems);
    m_syncServer->与服务器同步(BaseSyncServer::UploadOnly);

    qDebug() << "本地类别更新成功:" << name << "->" << newName;
    结束更新模型();
}

void CategoryManager::deleteCategory(const QString &name) {
    qDebug() << "=== 开始本地类别 ===" << name;

    if (name == "未分类") {
        return;
    }

    // 查找要删除的类别
    CategorieItem *category = 寻找类别(name);
    if (!category) {
        qWarning() << "要删除的类别不存在:" << name;
        return;
    }

    // 检查是否可以删除
    if (!category->canBeDeleted()) {
        return;
    }

    开始更新模型();
    // 从列表中移除
    m_dataStorage->软删除类别(m_categoryItems, name);

    m_syncServer->设置未同步的对象(m_categoryItems);
    m_syncServer->与服务器同步(BaseSyncServer::UploadOnly);

    qDebug() << "本地类别删除成功:" << name;
    结束更新模型();
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
    开始更新模型();
    m_dataStorage->导入类别从JSON(m_categoryItems, categoriesArray);
    结束更新模型();
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
 * @brief 从存储加载类别
 */
void CategoryManager::加载类别() {
    qDebug() << "开始从存储加载类别数据";

    if (!m_dataStorage) {
        qWarning() << "数据存储对象为空，无法加载类别";
        return;
    }

    开始更新模型();

    // 清空现有数据
    m_categoryItems.clear();

    // 从存储加载
    bool success = m_dataStorage->加载类别(m_categoryItems);

    if (success) {
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

    结束更新模型();
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