/**
 * @file todo_model.cpp
 * @brief TodoModel类的实现文件
 *
 * 该文件实现了TodoModel类，专门负责待办事项的数据模型显示。
 * 从TodoManager中拆分出来，专门负责QAbstractListModel的实现。
 *
 * @author Sakurakugu
 * @date 2025-09-23 18:45:36(UTC+8) 周二
 * @change 2025-09-24 03:45:31(UTC+8) 周三
 */

#include "todo_model.h"
#include "global_state.h"

#include "todo_data_storage.h"
#include "todo_item.h"
#include "todo_queryer.h"
#include "todo_sync_server.h"

TodoModel::TodoModel(TodoDataStorage &dataStorage, TodoSyncServer &syncServer, TodoQueryer &queryer, QObject *parent)
    : QAbstractListModel(parent), //
      m_filterCacheDirty(true),   //
      m_dataManager(dataStorage), //
      m_syncManager(syncServer),  //
      m_queryer(queryer)          //
{
    // 连接查询器信号，当查询条件变化时更新缓存
    connect(&m_queryer, &TodoQueryer::queryConditionsChanged, this, [this]() {
        beginResetModel();
        需要重新筛选();
        endResetModel();
    });

    // 连接同步管理器信号
    connect(&m_syncManager, &TodoSyncServer::todosUpdatedFromServer, this, &TodoModel::onTodosUpdatedFromServer);
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
    if (!m_queryer.是否激活任何查询条件()) {
        return m_todos.size();
    }

    // 使用缓存的过滤结果
    const_cast<TodoModel *>(this)->更新过滤后的待办();
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
    if (!m_queryer.是否激活任何查询条件()) {
        if (static_cast<size_t>(index.row()) >= m_todos.size())
            return QVariant();
        return 获取项目数据(m_todos[index.row()].get(), role);
    }

    // 使用缓存的过滤结果
    const_cast<TodoModel *>(this)->更新过滤后的待办();
    if (index.row() >= m_filteredTodos.size())
        return QVariant();

    return 获取项目数据(m_filteredTodos[index.row()], role);
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
        item->setSynced(2); // 标记为未同步，待更新

        需要重新筛选();
        emit dataChanged(index, index, QVector<int>() << role);
        m_dataManager.更新待办(m_todos, *item);

        更新同步管理器的数据();

        return true;
    }

    return false;
}

bool TodoModel::加载待办() {
    m_dataManager.加载待办(m_todos);
    更新过滤后的待办();
    重建ID索引();
    更新同步管理器的数据();
    return true;
}

bool TodoModel::新增待办(const QString &title, const QUuid &userUuid, const QString &description,      //
                         const QString &category, bool important, const QDateTime &deadline,           //
                         int recurrenceInterval, int recurrenceCount, const QDate &recurrenceStartDate //
) {
    // 因为显示的是排序后的结果，所以直接重置模型
    beginResetModel();
    auto result = m_dataManager.新增待办(m_todos, title, description, category, important, //
                                         deadline, recurrenceInterval, recurrenceCount, recurrenceStartDate, userUuid);
    if (!result) {
        endInsertRows();
        return false;
    }

    // 添加到索引
    if (!m_todos.empty()) {
        添加到ID索引(m_todos.back().get());
    }
    需要重新筛选();
    更新过滤后的待办();
    endResetModel();

    与服务器同步();

    return true;
}

bool TodoModel::更新待办(int index, const QVariantMap &todoData) {
    // 通过过滤后的索引获取实际的任务项
    auto todoItem = 获取过滤后的待办(index);
    QModelIndex modelIndex = 获取内容在待办列表中的索引(todoItem);

    TodoItem *item = m_todos[modelIndex.row()].get();
    QVector<int> changedRoles; // TODO: 计算实际更改的角色

    try {
        beginResetModel();
        m_dataManager.更新待办(m_todos, item->uuid(), todoData);
        需要重新筛选();
        // TODO: 这里是要过滤后的索引还是过滤前的，到时候都试一下
        emit dataChanged(modelIndex, modelIndex, changedRoles);
        endResetModel();
        与服务器同步();
        qDebug() << "成功更新索引" << index << "处的待办事项";
        return true;

    } catch (const std::exception &e) {
        qCritical() << "更新待办事项时发生异常:" << e.what();
        return false;
    } catch (...) {
        qCritical() << "更新待办事项时发生未知异常";
        return false;
    }
}

bool TodoModel::标记完成(int index, bool completed) {
    // 通过过滤后的索引获取实际的任务项
    auto todoItem = 获取过滤后的待办(index);
    QModelIndex modelIndex = 获取内容在待办列表中的索引(todoItem);

    TodoItem *item = m_todos[modelIndex.row()].get();
    QVector<int> changedRoles;

    bool success = false;
    try {
        QVariantMap todoData;
        todoData["is_completed"] = completed;

        // 如果任务被标记为完成，并且当前过滤器是“待办”或“已完成”，则将其从视图中移除
        if (m_queryer.currentFilter() == "done" || m_queryer.currentFilter() == "todo") {
            // 通知视图即将删除行
            beginRemoveRows(QModelIndex(), index, index);
            m_dataManager.更新待办(m_todos, item->uuid(), todoData);
            需要重新筛选();
            emit dataChanged(modelIndex, modelIndex, changedRoles);
            endRemoveRows();
        } else { // 否则直接更新视图
            beginResetModel();
            m_dataManager.更新待办(m_todos, item->uuid(), todoData);
            QModelIndex modelIndex = createIndex(index, 0);
            需要重新筛选();
            emit dataChanged(modelIndex, modelIndex);
            endResetModel();
        }
        与服务器同步();
        qDebug() << "成功标记索引" << index << "处的待办事项为" << (completed ? "已完成" : "未完成");
        success = true;
    } catch (const std::exception &e) {
        qCritical() << "标记做完/未做待办事项时发生异常:" << e.what();
        success = false;
    } catch (...) {
        qCritical() << "标记做完/未做待办事项时发生未知异常";
        success = false;
    }
    return success;
}

bool TodoModel::标记删除(int index, bool deleted) {
    // 通过过滤后的索引获取实际的任务项
    auto todoItem = 获取过滤后的待办(index);
    QModelIndex modelIndex = 获取内容在待办列表中的索引(todoItem);

    TodoItem *item = m_todos[modelIndex.row()].get();
    QVector<int> changedRoles;

    bool success = false;
    try {
        if (deleted) {
            // 通知视图即将删除行
            beginRemoveRows(QModelIndex(), index, index);
            // 软删除
            m_dataManager.回收待办(m_todos, item->uuid());
            需要重新筛选();
            emit dataChanged(modelIndex, modelIndex, changedRoles);
            // 通知视图删除完成
            endRemoveRows();
            与服务器同步();
            qDebug() << "成功软删除索引" << index << "处的待办事项";
            success = true;
        } else {
            // 检查任务是否已删除
            if (!todoItem->isDeleted()) {
                qWarning() << "尝试恢复未删除的任务，索引:" << index;
                return false;
            }
            beginResetModel();
            QVariantMap todoData;
            todoData["is_deleted"] = false;
            return 更新待办(index, todoData);

            // 通知视图数据已更改
            QModelIndex modelIndex = createIndex(index, 0);

            // 使筛选缓存失效，以便重新筛选
            需要重新筛选();
            emit dataChanged(modelIndex, modelIndex);
            endResetModel();
            与服务器同步();

            qDebug() << "成功恢复索引" << index << "处的待办事项";
            success = true;
        }
    } catch (const std::exception &e) {
        qCritical() << "软删除待办事项时发生异常:" << e.what();
        success = false;
    } catch (...) {
        qCritical() << "软删除待办事项时发生未知异常";
        success = false;
    }
    return success;
}

bool TodoModel::软删除待办(int index) {
    // 通过过滤后的索引获取实际的任务项
    auto todoItem = 获取过滤后的待办(index);
    return m_dataManager.软删除待办(m_todos, todoItem->uuid());
}

bool TodoModel::删除待办(int index) {
    try {
        // 通过过滤后的索引获取实际的任务项
        auto todoItem = 获取过滤后的待办(index);

        // 检查任务是否已删除
        if (!todoItem->isDeleted()) {
            qWarning() << "尝试永久删除未删除的任务，索引:" << index;
            return false;
        }

        // 找到该任务在原始数组中的索引
        auto it = std::find_if(m_todos.begin(), m_todos.end(),
                               [todoItem](const std::unique_ptr<TodoItem> &item) { return item.get() == todoItem; });

        if (it == m_todos.end()) {
            qWarning() << "无法在原始数组中找到要删除的任务";
            return false;
        }

        // 先删除数据库中的记录
        if (!m_dataManager.删除待办(m_todos, todoItem->uuid())) {
            qWarning() << "永久删除待办事项时数据库操作失败";
            return false;
        }

        // 永久删除：从列表中物理移除
        beginRemoveRows(QModelIndex(), index, index);
        m_todos.erase(it);
        需要重新筛选();
        endRemoveRows();
        重建ID索引();
        与服务器同步();

        qDebug() << "成功永久删除索引" << index << "处的待办事项";
        return true;
    } catch (const std::exception &e) {
        qCritical() << "永久删除待办事项时发生异常:" << e.what();
        return false;
    } catch (...) {
        qCritical() << "永久删除待办事项时发生未知异常";
        return false;
    }
}

bool TodoModel::删除所有待办(bool deleteLocal, const QUuid &userUuid) {
    bool success = false;
    try {
        qDebug() << "删除所有待办事项" << deleteLocal;
        beginResetModel();
        if (!deleteLocal) {
            qDebug() << "不删除本地数据，只更新用户UUID";
            m_dataManager.更新所有待办用户UUID(m_todos, userUuid, 1);

            更新同步管理器的数据();
        } else {
            // 删除本地数据
            if (m_todos.empty()) {
                qDebug() << "没有待办事项需要删除";
            }

            // 清空所有待办事项
            m_dataManager.删除所有待办(m_todos);
            m_idIndex.clear();
        }
        // 使筛选缓存失效
        需要重新筛选();
        // 结束重置模型
        endResetModel();
        与服务器同步();

        qDebug() << "成功删除所有待办事项";
        success = true;
    } catch (const std::exception &e) {
        qCritical() << "删除所有待办事项时发生异常:" << e.what();
        success = false;
    } catch (...) {
        qCritical() << "删除所有待办事项时发生未知异常";
        success = false;
    }
    return success;
}

// 性能优化相关方法实现
void TodoModel::更新过滤后的待办() {
    if (!m_filterCacheDirty) {
        return;
    }

    m_filteredTodos.clear();

    // 组装查询参数（数据库端过滤 + 排序）
    TodoDataStorage::QueryOptions opt{
        .category = m_queryer.currentCategory(),            //
        .statusFilter = m_queryer.currentFilter(),          //
        .searchText = m_queryer.searchText(),               //
        .dateFilterEnabled = m_queryer.dateFilterEnabled(), //
        .dateStart = m_queryer.dateFilterStart(),           //
        .dateEnd = m_queryer.dateFilterEnd(),               //
        .sortType = m_queryer.sortType(),                   //
        .descending = m_queryer.descending()                //
    }; // 暂不分页，后续可加入 opt.limit/offset

    QList<int> ids = m_dataManager.查询待办ID列表(opt);
    for (int id : ids) {
        auto it = m_idIndex.find(id);
        if (it != m_idIndex.end()) {
            m_filteredTodos.append(it->second);
        }
    }

    m_filterCacheDirty = false;
}

void TodoModel::需要重新筛选() {
    m_filterCacheDirty = true;
}

// 更新同步管理器的待办事项数据
void TodoModel::更新同步管理器的数据() {
    QList<TodoItem *> todoItems;
    for (const auto &item : m_todos) {
        todoItems.append(item.get());
    }
    m_syncManager.setTodoItems(todoItems);
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
 * @brief 与服务器同步待办事项数据
 *
 * 该方法会先获取服务器上的最新数据，然后将本地更改推送到服务器。
 * 操作结果通过syncCompleted信号通知。
 */
void TodoModel::与服务器同步() {
    if (!GlobalState::GetInstance().isAutoSyncEnabled()) {
        return;
    }
    更新同步管理器的数据();
    m_syncManager.与服务器同步();
}

void TodoModel::强制与服务器同步() {
    更新同步管理器的数据();
    m_syncManager.与服务器同步();
}

void TodoModel::onTodosUpdatedFromServer(const QJsonArray &todosArray) {
    qDebug() << "从服务器更新" << todosArray.size() << "个待办事项，当前本地有" << m_todos.size() << "个待办事项";

    beginResetModel();
    m_dataManager.导入待办事项从JSON(m_todos, todosArray);
    endResetModel();

    需要重新筛选();
    重建ID索引();
    更新同步管理器的数据();
}

/**
 * @brief 获取指定TodoItem的模型索引
 * @param todoItem 待获取索引的TodoItem指针
 * @return 对应的模型索引
 */
QModelIndex TodoModel::获取内容在待办列表中的索引(TodoItem *todoItem) const {
    if (!todoItem) {
        return QModelIndex();
    }

    // 查找TodoItem在m_todos中的位置
    auto it = std::find_if(m_todos.begin(), m_todos.end(),
                           [todoItem](const std::unique_ptr<TodoItem> &todo) { return todo.get() == todoItem; });

    if (it == m_todos.end()) {
        return QModelIndex(); // 未找到对应的TodoItem
    }

    // 计算索引
    int index = static_cast<int>(std::distance(m_todos.begin(), it));
    return createIndex(index, 0);
}

TodoItem *TodoModel::获取过滤后的待办(int index) const {
    if (index < 0 || static_cast<size_t>(index) >= m_todos.size()) {
        qWarning() << "尝试更新无效的索引:" << index;
        return nullptr;
    }
    const_cast<TodoModel *>(this)->更新过滤后的待办();
    if (index < 0 || index >= static_cast<int>(m_filteredTodos.size())) {
        qWarning() << "过滤后的索引超出范围:" << index;
        return nullptr;
    }
    return m_filteredTodos[index];
}

// ===================== 索引维护实现 =====================
/**
 * @brief 重建ID索引映射
 * 该方法会清空现有的ID索引映射，并重新遍历m_todos列表，构建新的映射。
 * 适用于在批量修改m_todos后需要重新生成索引的场景。
 */
void TodoModel::重建ID索引() {
    m_idIndex.clear();
    for (auto &ptr : m_todos) {
        if (ptr) {
            m_idIndex[ptr->id()] = ptr.get();
        }
    }
}

/**
 * @brief 向ID索引映射中添加一个新的待办事项
 */
void TodoModel::添加到ID索引(TodoItem *item) {
    if (item) {
        m_idIndex[item->id()] = item;
    }
}

/**
 * @brief 从ID索引映射中移除一个待办事项
 */
void TodoModel::从ID索引中移除(int id) {
    m_idIndex.erase(id);
}

/**
 * @brief 根据角色获取项目数据
 * @param item 待办事项指针
 * @param role 数据角色
 * @return 请求的数据值
 */
QVariant TodoModel::获取项目数据(const TodoItem *item, int role) const {
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
