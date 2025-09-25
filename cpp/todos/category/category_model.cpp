/**
 * @file category_model.cpp
 * @brief CategoryModel类的实现文件
 *
 * 该文件实现了CategoryModel类，专门负责类别的数据模型显示。
 * 从CategoryManager中拆分出来，专门负责QAbstractListModel的实现。
 *
 * @author Sakurakugu
 * @date 2025-09-23 16:11:01(UTC+8) 周二
 * @change 2025-09-24 03:45:31(UTC+8) 周三
 */

#include "category_model.h"

CategoryModel::CategoryModel(CategoryDataStorage &dataStorage, CategorySyncServer &syncServer, QObject *parent)
    : QAbstractListModel(parent), //
      m_dataStorage(dataStorage), //
      m_syncServer(syncServer) {}

CategoryModel::~CategoryModel() {}

/**
 * @brief 获取模型中的行数（类别数量）
 * @param parent 父索引，默认为无效索引（根）
 * @return 类别数量
 */
int CategoryModel::rowCount(const QModelIndex &parent) const {
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
QVariant CategoryModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || static_cast<size_t>(index.row()) >= m_categoryItems.size())
        return QVariant();

    return 获取项目数据(m_categoryItems[index.row()].get(), role);
}

/**
 * @brief 获取角色名称映射，用于QML访问
 * @return 角色名称哈希表
 */
QHash<int, QByteArray> CategoryModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[UuidRole] = "uuid";
    roles[NameRole] = "name";
    roles[UserUuidRole] = "userUuid";
    roles[CreatedAtRole] = "createdAt";
    roles[UpdatedAtRole] = "updatedAt";
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
bool CategoryModel::setData(const QModelIndex &index, const QVariant &value, int role) {
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
 * @brief 获取类别名称列表
 * @return 类别名称的字符串列表
 */
QStringList CategoryModel::获取类别() const {
    QStringList categories;
    for (const auto &item : m_categoryItems) {
        if (item && item->synced() != 3) // 只返回未删除的类别
            categories << item->name();
    }
    return categories;
}

/**
 * @brief 处理类别数据变化
 */
void CategoryModel::onCategoriesChanged() {
    开始更新模型();
    结束更新模型();
}

/**
 * @brief 根据角色获取项目数据
 * @param item 类别项目指针
 * @param role 数据角色
 * @return 请求的数据值
 */
QVariant CategoryModel::获取项目数据(const CategorieItem *item, int role) const {
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
    case SyncedRole:
        return item->synced();
    default:
        return QVariant();
    }
}

/**
 * @brief 根据名称查找类别项目
 * @param name 类别名称
 * @return 类别项目指针，如果未找到返回nullptr
 */
CategorieItem *CategoryModel::寻找类别(const QString &name) const {
    auto it = std::find_if(m_categoryItems.begin(), m_categoryItems.end(),
                           [&name](const std::unique_ptr<CategorieItem> &item) { return item->name() == name; });
    return (it != m_categoryItems.end()) ? it->get() : nullptr;
}

/**
 * @brief 根据ID查找类别项目
 * @param id 类别ID
 * @return 类别项目指针，如果未找到返回nullptr
 */
CategorieItem *CategoryModel::寻找类别(int id) const {
    auto it = std::find_if(m_categoryItems.begin(), m_categoryItems.end(),
                           [id](const std::unique_ptr<CategorieItem> &item) { return item->id() == id; });
    return (it != m_categoryItems.end()) ? it->get() : nullptr;
}

/**
 * @brief 根据UUID查找类别项目
 * @param uuid 类别UUID
 * @return 找到的类别项目指针，如果未找到则返回nullptr
 */
CategorieItem *CategoryModel::寻找类别(const QUuid &uuid) const {
    auto it = std::find_if(m_categoryItems.begin(), m_categoryItems.end(),
                           [uuid](const std::unique_ptr<CategorieItem> &item) { return item->uuid() == uuid; });
    return (it != m_categoryItems.end()) ? it->get() : nullptr;
}

bool CategoryModel::新增类别(const QString &name, const QUuid &userUuid) {
    qDebug() << "=== 开始创建类别 ===" << name;

    if (!是否是有效名称(name))
        return false;

    if (寻找类别(name) != nullptr)
        return false;

    开始更新模型();
    auto it = m_dataStorage.新增类别(m_categoryItems, name, userUuid);
    与服务器同步();
    qDebug() << "类别创建成功:" << name;
    结束更新模型();
    return it ? true : false;
}

bool CategoryModel::更新类别(const QString &name, const QString &newName) {
    qDebug() << "=== 开始更新本地类别 ===" << name << "->" << newName;

    if (!是否是有效名称(newName)) {
        return false;
    }

    if (name == "未分类") {
        return false;
    }

    // 检查原类别是否存在
    CategorieItem *nowCategory = 寻找类别(name);
    if (nowCategory == nullptr) {
        qWarning() << "待更新的类别不存在:" << name;
        return false;
    }

    // 检查新名称是否与其他类别重复（排除自身）
    CategorieItem *newCategory = 寻找类别(newName);
    if (newCategory != nullptr) {
        qWarning() << "该类别已存在:" << newName;
        return false;
    }

    开始更新模型();
    bool success = m_dataStorage.更新类别(m_categoryItems, name, newName);
    与服务器同步();
    qDebug() << "本地类别更新成功:" << name << "->" << newName;
    结束更新模型();
    return success;
}

bool CategoryModel::删除类别(const QString &name) {
    qDebug() << "=== 开始本地类别 ===" << name;

    if (name == "未分类") {
        return false;
    }

    // 查找要删除的类别
    CategorieItem *category = 寻找类别(name);
    if (!category) {
        qWarning() << "要删除的类别不存在:" << name;
        return false;
    }

    // 检查是否可以删除
    if (!category->canBeDeleted()) {
        return false;
    }

    开始更新模型();
    bool success = m_dataStorage.软删除类别(m_categoryItems, name);
    //^ 内部会判断是直接删还是标记为同步
    与服务器同步();
    qDebug() << "本地类别删除成功:" << name;
    结束更新模型();
    return success;
}

/**
 * @brief 从存储加载类别
 */
bool CategoryModel::加载类别(const QUuid &userUuid) {
    qDebug() << "开始从存储加载类别数据";

    开始更新模型();
    bool success = m_dataStorage.加载类别(m_categoryItems);

    if (success) {
        m_syncServer.设置未同步的对象(m_categoryItems);
    } else {
        qWarning() << "从存储加载类别失败";
        m_dataStorage.创建默认类别(m_categoryItems, userUuid);
    }

    结束更新模型();
    return success;
}

void CategoryModel::与服务器同步() {
    m_syncServer.设置未同步的对象(m_categoryItems);
    m_syncServer.与服务器同步(BaseSyncServer::Bidirectional);
}

void CategoryModel::更新同步成功状态(const std::vector<CategorieItem *> &categories) {
    开始更新模型();
    for (auto item : categories) {
        if (item->synced() != 3) { // 保留已删除状态
            m_dataStorage.更新同步状态(m_categoryItems, item->name());
        } else {
            m_dataStorage.删除类别(m_categoryItems, item->name());
        }
    }
    结束更新模型();
}

bool CategoryModel::导入类别从JSON(const QJsonArray &jsonArray, CategoryDataStorage::ImportSource source) {
    qDebug() << "开始从JSON导入类别数据";

    开始更新模型();
    bool success = m_dataStorage.导入类别从JSON(m_categoryItems, jsonArray, source);
    if (success) {
        m_syncServer.设置未同步的对象(m_categoryItems);
    } else {
        qWarning() << "从JSON导入类别失败";
    }
    结束更新模型();
    return success;
}

/**
 * @brief 验证类别名称
 * @param name 类别名称
 * @return 如果有效返回true，否则返回false
 */
bool CategoryModel::是否是有效名称(const QString &name) const {
    QString trimmedName = name.trimmed();
    return !trimmedName.isEmpty() && trimmedName.length() <= 50;
}

/**
 * @brief 开始模型更新
 */
void CategoryModel::开始更新模型() {
    beginResetModel();
}

/**
 * @brief 结束模型更新
 */
void CategoryModel::结束更新模型() {
    endResetModel();
    emit categoriesChanged();
}