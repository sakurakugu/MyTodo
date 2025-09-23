/**
 * @file todo_model.cpp
 * @brief TodoModel类的实现文件
 *
 * 该文件实现了TodoModel类，专门负责待办事项的数据模型显示。
 * 从TodoManager中拆分出来，专门负责QAbstractListModel的实现。
 *
 * @author Sakurakugu
 * @date 2025-09-23 (UTC+8)
 * @version 0.4.0
 */

#include "todo_model.h"
#include "todo_manager.h"
#include <QDebug>

TodoModel::TodoModel(TodoQueryer queryer, QObject *parent) : QAbstractListModel(parent), m_queryer(queryer) {
    // 连接筛选器信号，当筛选条件变化时更新缓存
    connect(m_queryer, &TodoQueryer::filtersChanged, this, [this]() {
        beginResetModel();
        清除过滤后的待办();
        endResetModel();
    });
}

TodoModel::~TodoModel() {}

/**
 * @brief 获取模型中的行数（待办事项数量）
 * @param parent 父索引，默认为无效索引（根）
 * @return 待办事项数量
 */
int TodoModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;

    // 使用筛选器获取筛选后的项目数量
    if (!m_queryer->hasActiveFilters()) {
        return m_todos.size();
    }

    // 使用缓存的过滤结果
    const_cast<TodoManager *>(this)->更新过滤后的待办();
    return m_filteredTodos.count();
}

/**
 * @brief 获取指定索引和角色的数据
 * @param index 待获取数据的模型索引
 * @param role 数据角色
 * @return 请求的数据值
 */
QVariant TodoModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return QVariant();

    // 如果没有设置过滤条件，直接返回对应索引的项目
    if (!m_queryer->hasActiveFilters()) {
        if (static_cast<size_t>(index.row()) >= m_todos.size())
            return QVariant();
        return getItemData(m_todos[index.row()].get(), role);
    }

    // 使用缓存的过滤结果
    const_cast<TodoManager *>(this)->更新过滤后的待办();
    if (index.row() >= m_filteredTodos.size())
        return QVariant();

    return getItemData(m_filteredTodos[index.row()], role);
}

/**
 * @brief 获取角色名称映射，用于QML访问
 * @return 角色名称哈希表
 */
QHash<int, QByteArray> TodoModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[UuidRole] = "uuid";
    roles[UserUuidRole] = "userUuid";
    roles[TitleRole] = "title";
    roles[DescriptionRole] = "description";
    roles[CategoryRole] = "category";
    roles[ImportantRole] = "important";
    roles[DeadlineRole] = "deadline";
    roles[RecurrenceIntervalRole] = "recurrenceInterval";
    roles[RecurrenceCountRole] = "recurrenceCount";
    roles[RecurrenceStartDateRole] = "recurrenceStartDate";
    roles[IsCompletedRole] = "isCompleted";
    roles[CompletedAtRole] = "completedAt";
    roles[IsDeletedRole] = "isDeleted";
    roles[DeletedAtRole] = "deletedAt";
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
bool TodoModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (!index.isValid() || static_cast<size_t>(index.row()) >= m_todos.size())
        return false;

    TodoItem *item = m_todos.at(index.row()).get();
    bool changed = false;

    switch (role) {
    case TitleRole:
        item->setTitle(value.toString());
        changed = true;
        break;
    case DescriptionRole:
        item->setDescription(value.toString());
        changed = true;
        break;
    case CategoryRole:
        item->setCategory(value.toString());
        changed = true;
        break;
    case ImportantRole:
        item->setImportant(value.toBool());
        changed = true;
        break;
    case RecurrenceIntervalRole:
        item->setRecurrenceInterval(value.toInt());
        changed = true;
        break;
    case RecurrenceCountRole:
        item->setRecurrenceCount(value.toInt());
        changed = true;
        break;
    case RecurrenceStartDateRole:
        item->setRecurrenceStartDate(value.toDate());
        changed = true;
        break;
    case DeadlineRole:
        item->setDeadline(value.toDateTime());
        changed = true;
        break;
    case IsCompletedRole:
        item->setIsCompleted(value.toBool());
        changed = true;
        break;
    case IsDeletedRole:
        item->setIsDeleted(value.toBool());
        changed = true;
        break;
    }

    if (changed) {
        item->setUpdatedAt(QDateTime::currentDateTime());
        if (item->synced() != 1) { // 如果之前不是未同步+新建状态
            item->setSynced(2);
        }
        清除过滤后的待办();
        emit dataChanged(index, index, QVector<int>() << role);
        m_dataManager->更新待办(m_todos, *item);

        // 更新同步管理器的数据
        更新同步管理器的数据();

        return true;
    }

    return false;
}

/**
 * @brief 处理数据变化
 */
void TodoModel::onDataChanged() {
    beginResetModel();
    endResetModel();
    emit dataUpdated();
}

/**
 * @brief 处理行插入
 */
void TodoModel::onRowsInserted() {
    // 当有新的待办事项添加时调用
    onDataChanged();
}

/**
 * @brief 处理行删除
 */
void TodoModel::onRowsRemoved() {
    // 当有待办事项删除时调用
    onDataChanged();
}

/**
 * @brief 根据角色获取项目数据
 * @param item 待办事项指针
 * @param role 数据角色
 * @return 请求的数据值
 */
QVariant TodoModel::getItemData(const TodoItem *item, int role) const {
    if (!item)
        return QVariant();

    switch (role) {
    case IdRole:
        return item->id();
    case UuidRole:
        return item->uuid();
    case UserUuidRole:
        return item->userUuid();
    case TitleRole:
        return item->title();
    case DescriptionRole:
        return item->description();
    case CategoryRole:
        return item->category();
    case ImportantRole:
        return item->important();
    case DeadlineRole:
        return item->deadline();
    case RecurrenceIntervalRole:
        return item->recurrenceInterval();
    case RecurrenceCountRole:
        return item->recurrenceCount();
    case RecurrenceStartDateRole:
        return item->recurrenceStartDate();
    case IsCompletedRole:
        return item->isCompleted();
    case CompletedAtRole:
        return item->completedAt();
    case IsDeletedRole:
        return item->isDeleted();
    case DeletedAtRole:
        return item->deletedAt();
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
 * @brief 从名称获取角色
 * @param name 角色名称
 * @return 对应的角色枚举值
 */
TodoModel::TodoRoles TodoModel::roleFromName(const QString &name) const {
    static const QHash<QString, TodoRoles> roleMap = //
        {
            {"id", IdRole},
            {"uuid", UuidRole},
            {"userUuid", UserUuidRole},
            {"title", TitleRole},
            {"description", DescriptionRole},
            {"category", CategoryRole},
            {"important", ImportantRole},
            {"deadline", DeadlineRole},
            {"recurrenceInterval", RecurrenceIntervalRole},
            {"recurrenceCount", RecurrenceCountRole},
            {"recurrenceStartDate", RecurrenceStartDateRole},
            {"isCompleted", IsCompletedRole},
            {"completedAt", CompletedAtRole},
            {"isDeleted", IsDeletedRole},
            {"deletedAt", DeletedAtRole},
            {"createdAt", CreatedAtRole},
            {"updatedAt", UpdatedAtRole},
            {"synced", SyncedRole} //
        };

    return roleMap.value(name, static_cast<TodoRoles>(Qt::DisplayRole));
}