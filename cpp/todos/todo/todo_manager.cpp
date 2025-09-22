/**
 * @brief 构造函数
 *
 * 初始化TodoManager对象，设置父对象为parent。
 *
 * @param parent 父对象指针，默认值为nullptr。
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @change 2025-09-06 01:29:53(UTC+8) 周六
 * @version 0.4.0
 */
#include "todo_manager.h"
#include "../category/category_data_storage.h"
#include "../category/category_manager.h"
#include "../category/category_sync_server.h"
#include "foundation/network_request.h"
#include "global_state.h"
#include "todo_queryer.h"
#include "user_auth.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkProxy>
#include <QProcess>
#include <QUuid>

TodoManager::TodoManager(UserAuth &userAuth, CategoryManager &categoryManager,
                         QObject *parent)
    : QAbstractListModel(parent),                        //
      m_filterCacheDirty(true),                          //
      m_networkRequest(NetworkRequest::GetInstance()),   //
      m_userAuth(userAuth),                              //
      m_globalState(GlobalState::GetInstance()),         //
      m_syncManager(new TodoSyncServer(userAuth, this)), //
      m_dataManager(new TodoDataStorage(this)),          //
      m_categoryManager(&categoryManager),               //
      m_queryer(new TodoQueryer(this))                   //
{
    // 连接筛选器信号，当筛选条件变化时更新缓存
    connect(m_queryer, &TodoQueryer::filtersChanged, this, [this]() {
        beginResetModel();
        清除过滤后的待办();
        endResetModel();
    });

    // 连接排序器信号，当排序类型或倒序状态变化时重新排序
    connect(m_queryer, &TodoQueryer::sortTypeChanged, this, &TodoManager::sortTodos);
    connect(m_queryer, &TodoQueryer::descendingChanged, this, &TodoManager::sortTodos);

    // 连接同步管理器信号
    connect(m_syncManager, &TodoSyncServer::syncStarted, this, &TodoManager::onSyncStarted);
    connect(m_syncManager, &TodoSyncServer::syncCompleted, this, &TodoManager::onSyncCompleted);
    connect(m_syncManager, &TodoSyncServer::todosUpdatedFromServer, this, &TodoManager::onTodosUpdatedFromServer);

    // 连接用户认证信号，登录成功后触发同步
    connect(&m_userAuth, &UserAuth::firstAuthCompleted, this, [this]() { syncWithServer(); });

    // 通过数据管理器加载本地数据
    m_dataManager->加载待办事项(m_todos);
    rebuildIdIndex();

    // 设置待办事项数据到同步管理器
    更新同步管理器的数据();

    // 如果已登录，CategorySyncServer 会自动处理类别同步
    if (m_userAuth.isLoggedIn()) {
        // CategorySyncServer 会自动处理类别同步
    }
}

/**
 * @brief 析构函数
 *
 * 清理资源，保存未同步的数据到本地存储。
 */
TodoManager::~TodoManager() {
    m_todos.clear();
    m_filteredTodos.clear();
    m_idIndex.clear();
}

/**
 * @brief 获取模型中的行数（待办事项数量）
 * @param parent 父索引，默认为无效索引（根）
 * @return 待办事项数量
 */
int TodoManager::rowCount(const QModelIndex &parent) const {
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
QVariant TodoManager::data(const QModelIndex &index, int role) const {
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

// 辅助方法，根据角色返回项目数据
QVariant TodoManager::getItemData(const TodoItem *item, int role) const {
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
 * @brief 获取指定TodoItem的模型索引
 * @param todoItem 待获取索引的TodoItem指针
 * @return 对应的模型索引
 */
QModelIndex TodoManager::获取内容在待办列表中的索引(TodoItem *todoItem) const {
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

/**
 * @brief 从名称获取角色
 * @param name 角色名称
 * @return 对应的角色ID
 */
TodoManager::TodoRoles TodoManager::roleFromName(const QString &name) const {
    if (name == "id")
        return IdRole; // 任务ID
    if (name == "uuid")
        return UuidRole; // 任务UUID
    if (name == "userUuid")
        return UserUuidRole; // 用户UUID
    if (name == "title")
        return TitleRole; // 任务标题
    if (name == "description")
        return DescriptionRole; // 任务描述
    if (name == "category")
        return CategoryRole; // 任务分类
    if (name == "important")
        return ImportantRole; // 任务重要程度
    if (name == "deadline")
        return DeadlineRole; // 任务截止时间
    if (name == "recurrenceInterval")
        return RecurrenceIntervalRole; // 循环间隔
    if (name == "recurrenceCount")
        return RecurrenceCountRole; // 循环次数
    if (name == "recurrenceStartDate")
        return RecurrenceStartDateRole; // 循环开始日期
    if (name == "isCompleted")
        return IsCompletedRole; // 任务是否已完成
    if (name == "completedAt")
        return CompletedAtRole; // 任务完成时间
    if (name == "isDeleted")
        return IsDeletedRole; // 任务是否已删除
    if (name == "deletedAt")
        return DeletedAtRole; // 任务删除时间
    if (name == "createdAt")
        return CreatedAtRole; // 任务创建时间
    if (name == "updatedAt")
        return UpdatedAtRole; // 任务更新时间
    if (name == "synced")
        return SyncedRole; // 任务是否已同步

    return static_cast<TodoRoles>(-1); // 返回一个无效的角色值
}

/**
 * @brief 获取角色名称映射，用于QML访问
 * @return 角色ID到角色名称的映射
 */
QHash<int, QByteArray> TodoManager::roleNames() const {
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
 * @return 设置是否成功
 */
bool TodoManager::setData(const QModelIndex &index, const QVariant &value, int role) {
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

// 性能优化相关方法实现
void TodoManager::更新过滤后的待办() {
    if (!m_filterCacheDirty) {
        return;
    }

    m_filteredTodos.clear();

    // 组装查询参数（数据库端过滤 + 排序）
    TodoDataStorage::QueryOptions opt{
        .category = m_queryer->currentCategory(),            //
        .statusFilter = m_queryer->currentFilter(),          //
        .searchText = m_queryer->searchText(),               //
        .dateFilterEnabled = m_queryer->dateFilterEnabled(), //
        .dateStart = m_queryer->dateFilterStart(),           //
        .dateEnd = m_queryer->dateFilterEnd(),               //
        .sortType = m_queryer->sortType(),                   //
        .descending = m_queryer->descending()                //
    }; // 暂不分页，后续可加入 opt.limit/offset

    QList<int> ids = m_dataManager->查询待办ID列表(opt);
    for (int id : ids) {
        auto it = m_idIndex.find(id);
        if (it != m_idIndex.end()) {
            m_filteredTodos.append(it->second);
        }
    }

    m_filterCacheDirty = false;
}

TodoItem *TodoManager::getFilteredItem(int index) const {
    const_cast<TodoManager *>(this)->更新过滤后的待办();

    if (index < 0 || index >= m_filteredTodos.size()) {
        return nullptr;
    }

    return m_filteredTodos[index];
}

void TodoManager::清除过滤后的待办() {
    m_filterCacheDirty = true;
}

/**
 * @brief 添加新的待办事项
 * @param title 任务标题（必填）
 * @param description 任务描述（可选）
 * @param category 任务分类（默认为"default"）
 * @param important 重要程度（默认为false）
 */
void TodoManager::addTodo(const QString &title, const QString &description, const QString &category, bool important,
                          const QString &deadline, int recurrenceInterval, int recurrenceCount,
                          const QDate &recurrenceStartDate) {
    beginInsertRows(QModelIndex(), rowCount(), rowCount());

    m_dataManager->新增待办(m_todos, title, description, category, important,
                            QDateTime::fromString(deadline, Qt::ISODate), recurrenceInterval, recurrenceCount,
                            recurrenceStartDate, m_userAuth.getUuid());
    if (!m_todos.empty()) {
        addToIndex(m_todos.back().get());
    }

    清除过滤后的待办();

    endInsertRows();

    // 更新同步管理器的数据
    更新同步管理器的数据();

    if (m_globalState.isAutoSyncEnabled() && m_userAuth.isLoggedIn()) {
        // 如果开启自动同步且已登录，立即尝试同步到服务器
        syncWithServer();
    }
}

/**
 * @brief 更新现有待办事项
 * @param index 待更新事项的索引
 * @param todoData 包含要更新字段的映射
 * @return 更新是否成功
 */
bool TodoManager::updateTodo(int index, const QVariantMap &todoData) {
    // 检查索引是否有效
    if (index < 0 || static_cast<size_t>(index) >= m_todos.size()) {
        qWarning() << "尝试更新无效的索引:" << index;
        return false;
    }

    // 获取过滤后的索引，因为QML传入的是过滤后的索引
    更新过滤后的待办();
    if (index >= static_cast<int>(m_filteredTodos.size())) {
        qWarning() << "过滤后的索引超出范围:" << index;
        return false;
    }

    // 通过过滤后的索引获取实际的任务项
    auto todoItem = m_filteredTodos[index];
    QModelIndex modelIndex = 获取内容在待办列表中的索引(todoItem);

    bool anyUpdated = false;
    TodoItem *item = m_todos[modelIndex.row()].get();
    QVector<int> changedRoles;

    try {
        beginResetModel();
        if (todoData.contains("title")) {
            QString newTitle = todoData["title"].toString();
            if (item->title() != newTitle) {
                item->setTitle(newTitle);
                changedRoles << TitleRole;
                anyUpdated = true;
            }
        }

        if (todoData.contains("description")) {
            QString newDescription = todoData["description"].toString();
            if (item->description() != newDescription) {
                item->setDescription(newDescription);
                changedRoles << DescriptionRole;
                anyUpdated = true;
            }
        }

        if (todoData.contains("category")) {
            QString newCategory = todoData["category"].toString();
            if (item->category() != newCategory) {
                item->setCategory(newCategory);
                changedRoles << CategoryRole;
                anyUpdated = true;
            }
        }

        if (todoData.contains("important")) {
            bool newImportant = todoData["important"].toBool();
            if (item->important() != newImportant) {
                item->setImportant(newImportant);
                changedRoles << ImportantRole;
                anyUpdated = true;
            }
        }

        if (todoData.contains("deadline")) {
            QString newDeadlineStr = todoData["deadline"].toString();
            QDateTime newDeadline = QDateTime::fromString(newDeadlineStr, Qt::ISODate);
            qDebug() << "新的截止日期: " << newDeadline;
            if (item->deadline() != newDeadline) {
                item->setDeadline(newDeadline);
                changedRoles << DeadlineRole;
                anyUpdated = true;
            }
        }

        if (todoData.contains("recurrenceInterval")) {
            int newRecurrenceInterval = todoData["recurrenceInterval"].toInt();
            if (item->recurrenceInterval() != newRecurrenceInterval) {
                item->setRecurrenceInterval(newRecurrenceInterval);
                anyUpdated = true;
            }
        }

        if (todoData.contains("recurrenceCount")) {
            int newRecurrenceCount = todoData["recurrenceCount"].toInt();
            if (item->recurrenceCount() != newRecurrenceCount) {
                item->setRecurrenceCount(newRecurrenceCount);
                anyUpdated = true;
            }
        }

        if (todoData.contains("recurrenceStartDate")) {
            QString newRecurrenceStartDateStr = todoData["recurrenceStartDate"].toString();
            QDate newRecurrenceStartDate = QDate::fromString(newRecurrenceStartDateStr, Qt::ISODate);
            qDebug() << "新的循环开始日期: " << newRecurrenceStartDate;
            if (item->recurrenceStartDate() != newRecurrenceStartDate) {
                item->setRecurrenceStartDate(newRecurrenceStartDate);
                anyUpdated = true;
            }
        }

        if (todoData.contains("isCompleted")) {
            bool newCompleted = todoData["isCompleted"].toBool();
            if (item->isCompleted() != newCompleted) {
                item->setIsCompleted(newCompleted);
                if (newCompleted) {
                    item->setCompletedAt(QDateTime::currentDateTime());
                }
                changedRoles << IsCompletedRole;
                anyUpdated = true;
            }
        }
        endResetModel();

        // 如果有任何更新，则触发一次dataChanged信号并保存
        if (anyUpdated) {
            item->setUpdatedAt(QDateTime::currentDateTime());
            item->setSynced(2);
            清除过滤后的待办();

            // 只触发一次dataChanged信号
            emit dataChanged(modelIndex, modelIndex, changedRoles);

            // 使用数据存储更新数据库而不是整表保存
            if (!m_dataManager->更新待办(m_todos, *item)) {
                qWarning() << "更新待办事项后无法更新到数据库";
            }

            // 更新同步管理器的数据
            更新同步管理器的数据();

            if (m_globalState.isAutoSyncEnabled() && m_userAuth.isLoggedIn()) {
                syncWithServer();
            }

            qDebug() << "成功更新索引" << index << "处的待办事项";
            return true;
        } else {
            qDebug() << "没有字段被更新，索引:" << index;
            return false;
        }
    } catch (const std::exception &e) {
        qCritical() << "更新待办事项时发生异常:" << e.what();
        return false;
    } catch (...) {
        qCritical() << "更新待办事项时发生未知异常";
        return false;
    }
}

/**
 * @brief 更新现有待办事项
 * @param index 待更新事项的索引
 * @param todoData 包含要更新字段的映射
 * @return 更新是否成功
 */
bool TodoManager::updateTodo(int index, const QString &roleName, const QVariant &value) {
    QVariantMap todoData;
    todoData[roleName] = value;
    return updateTodo(index, todoData);
}

/**
 * @brief 删除待办事项
 * @param index 待删除事项的索引
 * @return 删除是否成功
 */
bool TodoManager::removeTodo(int index) {
    // 检查索引是否有效
    if (index < 0 || static_cast<size_t>(index) >= m_todos.size()) {
        qWarning() << "尝试删除无效的索引:" << index;
        return false;
    }

    try {
        // 获取过滤后的索引，因为QML传入的是过滤后的索引
        更新过滤后的待办();
        if (index >= static_cast<int>(m_filteredTodos.size())) {
            qWarning() << "过滤后的索引超出范围:" << index;
            return false;
        }

        // 通知视图即将删除行
        beginRemoveRows(QModelIndex(), index, index);

        // 软删除
        auto todoItem = m_filteredTodos[index];
        // 先持久化到数据库
        m_dataManager->回收待办(m_todos, todoItem->id());
        // 再更新内存模型状态
        todoItem->setIsDeleted(true);
        todoItem->setDeletedAt(QDateTime::currentDateTime());
        todoItem->setSynced(2); // 标记为未同步，放到到回收站

        // 使筛选缓存失效，以便重新筛选
        清除过滤后的待办();

        // 通知视图删除完成
        endRemoveRows();

        if (m_globalState.isAutoSyncEnabled() && m_userAuth.isLoggedIn()) {
            syncWithServer();
        }

        qDebug() << "成功软删除索引" << index << "处的待办事项";
        return true;
    } catch (const std::exception &e) {
        qCritical() << "软删除待办事项时发生异常:" << e.what();
        return false;
    } catch (...) {
        qCritical() << "软删除待办事项时发生未知异常";
        return false;
    }
}

/**
 * @brief 从回收站恢复待办事项
 * @param index 待办事项的索引
 * @return 操作是否成功
 */
bool TodoManager::restoreTodo(int index) {
    // 检查索引是否有效
    if (index < 0 || static_cast<size_t>(index) >= m_todos.size()) {
        qWarning() << "尝试恢复无效的索引:" << index;
        return false;
    }

    try {
        auto &todoItem = m_todos[index];

        // 检查任务是否已删除
        if (!todoItem->isDeleted()) {
            qWarning() << "尝试恢复未删除的任务，索引:" << index;
            return false;
        }

        // 恢复任务：设置isDeleted为false，清除deletedAt时间戳
        todoItem->setIsDeleted(false);
        todoItem->setDeletedAt(QDateTime());
        todoItem->setSynced(2);

        // 通知视图数据已更改
        QModelIndex modelIndex = createIndex(index, 0);
        emit dataChanged(modelIndex, modelIndex);

        // 使筛选缓存失效，以便重新筛选
        清除过滤后的待办();

        // 使用数据存储更新数据库
        if (!m_dataManager->更新待办(m_todos, *todoItem)) {
            qWarning() << "恢复待办事项后无法更新到数据库";
        }

        if (m_globalState.isAutoSyncEnabled() && m_userAuth.isLoggedIn()) {
            syncWithServer();
        }

        qDebug() << "成功恢复索引" << index << "处的待办事项";
        return true;
    } catch (const std::exception &e) {
        qCritical() << "恢复待办事项时发生异常:" << e.what();
        return false;
    } catch (...) {
        qCritical() << "恢复待办事项时发生未知异常";
        return false;
    }
}

bool TodoManager::permanentlyDeleteTodo(int index) {
    // 检查索引是否有效
    if (index < 0 || static_cast<size_t>(index) >= m_todos.size()) {
        qWarning() << "尝试永久删除无效的索引:" << index;
        return false;
    }

    try {
        // 获取过滤后的索引，因为QML传入的是过滤后的索引
        更新过滤后的待办();
        if (index >= static_cast<int>(m_filteredTodos.size())) {
            qWarning() << "过滤后的索引超出范围:" << index;
            return false;
        }

        // 通过过滤后的索引获取实际的任务项
        auto todoItem = m_filteredTodos[index];

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
        if (!m_dataManager->删除待办(m_todos, todoItem->id())) {
            qWarning() << "永久删除待办事项时数据库操作失败";
            return false;
        }

        // 永久删除：从列表中物理移除
        beginRemoveRows(QModelIndex(), index, index);
        m_todos.erase(it);
        清除过滤后的待办();
        endRemoveRows();
        rebuildIdIndex();

        if (m_globalState.isAutoSyncEnabled() && m_userAuth.isLoggedIn()) {
            syncWithServer();
        }

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

/**
 * @brief 删除所有待办事项
 * @param deleteLocal 是否删除本地数据
 */
void TodoManager::deleteAllTodos(bool deleteLocal) {
    try {
        qDebug() << "删除所有待办事项" << deleteLocal;
        if (!deleteLocal) {
            // 如果不删除本地数据，只更新用户UUID
            qDebug() << "不删除本地数据，只更新用户UUID";
            updateAllTodosUserUuid();
            return;
        }

        // 删除本地数据

        if (m_todos.empty()) {
            qDebug() << "没有待办事项需要删除";
        }

        // 开始重置模型
        beginResetModel();

        // 清空所有待办事项
        m_todos.clear();
        m_idIndex.clear();

        // 使筛选缓存失效
        清除过滤后的待办();

        // 结束重置模型
        endResetModel();

        // 保存到本地存储
        if (!m_dataManager->saveToLocalStorage(m_todos)) {
            qWarning() << "删除所有待办事项后无法保存到本地存储";
        }

        // 如果开启自动同步且已登录，同步到服务器
        if (m_globalState.isAutoSyncEnabled() && m_userAuth.isLoggedIn()) {
            syncWithServer();
        }

        qDebug() << "成功删除所有待办事项";
    } catch (const std::exception &e) {
        qCritical() << "删除所有待办事项时发生异常:" << e.what();
    } catch (...) {
        qCritical() << "删除所有待办事项时发生未知异常";
    }
}

/**
 * @brief 更新所有待办事项的用户UUID
 * @param newUserUuid 新的用户UUID
 * @return 更新是否成功
 */
bool TodoManager::updateAllTodosUserUuid() {
    try {
        if (m_todos.empty()) {
            qDebug() << "没有待办事项需要更新用户UUID";
            return true;
        }

        bool hasChanges = false;

        // 获取当前用户UUID
        QUuid newUserUuid = m_userAuth.getUuid();

        // 遍历所有待办事项，更新用户UUID
        for (auto &todoItem : m_todos) {
            if (todoItem->userUuid() != newUserUuid) {
                todoItem->setUserUuid(newUserUuid);
                todoItem->setSynced(2); // 标记为未同步
                hasChanges = true;
            }
        }

        if (hasChanges) {
            // 通知模型数据已更改
            beginResetModel();
            清除过滤后的待办();
            endResetModel();

            // 保存到本地存储
            if (!m_dataManager->saveToLocalStorage(m_todos)) {
                qWarning() << "更新用户UUID后无法保存到本地存储";
            }

            // 更新同步管理器的数据
            更新同步管理器的数据();

            // 如果在线且已登录，同步到服务器
            if (m_globalState.isAutoSyncEnabled() && m_userAuth.isLoggedIn()) {
                syncWithServer();
            }

            qDebug() << "成功更新所有待办事项的用户UUID为:" << newUserUuid.toString();
        } else {
            qDebug() << "所有待办事项的用户UUID已经是最新的";
        }

        return true;
    } catch (const std::exception &e) {
        qCritical() << "更新用户UUID时发生异常:" << e.what();
        return false;
    } catch (...) {
        qCritical() << "更新用户UUID时发生未知异常";
        return false;
    }
}

/**
 * @brief 将待办事项标记为已完成
 * @param index 待办事项的索引
 * @return 操作是否成功
 */
bool TodoManager::markAsDoneOrTodo(int index) {
    // 检查索引是否有效
    if (index < 0 || static_cast<size_t>(index) >= m_todos.size()) {
        qWarning() << "尝试永久删除无效的索引:" << index;
        return false;
    }

    try {
        // 获取过滤后的索引，因为QML传入的是过滤后的索引
        更新过滤后的待办();
        if (index >= static_cast<int>(m_filteredTodos.size())) {
            qWarning() << "过滤后的索引超出范围:" << index;
            return false;
        }

        // 通过过滤后的索引获取实际的任务项
        auto todoItem = m_filteredTodos[index];
        QModelIndex modelIndex = 获取内容在待办列表中的索引(todoItem);

        beginResetModel();
        // qInfo() << "modelIndex.row(): " << index.row() << "标题: " <<
        // todoItem->title()
        //         << (todoItem->isCompleted()? "已完成" : "未完成");
        bool success = false;
        if (todoItem->isCompleted()) {
            success = setData(modelIndex, false, IsCompletedRole);
        } else {
            success = setData(modelIndex, true, IsCompletedRole);
        }
        // qInfo() << "modelIndex.row(): " << index.row() << "标题: " <<
        // todoItem->title()
        //         << (todoItem->isCompleted()? "已完成" : "未完成");

        if (success) {
            if (m_globalState.isAutoSyncEnabled() && m_userAuth.isLoggedIn()) {
                syncWithServer();
            }
            qDebug() << "成功将索引" << index << "处的待办事项标记为已完成";
        } else {
            qWarning() << "无法将索引" << index << "处的待办事项标记为已完成";
        }

        endResetModel();

        return success;
    } catch (const std::exception &e) {
        qCritical() << "标记待办事项为已完成时发生异常:" << e.what();
        return false;
    } catch (...) {
        qCritical() << "标记待办事项为已完成时发生未知异常";
        return false;
    }
}

/**
 * @brief 与服务器同步待办事项数据
 *
 * 该方法会先获取服务器上的最新数据，然后将本地更改推送到服务器。
 * 操作结果通过syncCompleted信号通知。
 */
void TodoManager::syncWithServer() {
    // 更新同步管理器的数据
    更新同步管理器的数据();

    // 委托给同步管理器处理
    m_syncManager->与服务器同步();
}

// 同步管理器信号处理槽函数
void TodoManager::onSyncStarted() {
    emit syncStarted();
}

void TodoManager::onSyncCompleted(TodoSyncServer::SyncResult result, const QString &message) {
    bool success = (result == TodoSyncServer::Success);
    emit syncCompleted(success, message); // TODO:触发了两次

    // 如果同步成功，保存到本地存储
    if (success) {
        if (!m_dataManager->saveToLocalStorage(m_todos)) {
            qWarning() << "同步成功后无法保存到本地存储";
        }
    }
}

void TodoManager::onTodosUpdatedFromServer(const QJsonArray &todosArray) {
    updateTodosFromServer(todosArray);
}

void TodoManager::updateTodosFromServer(const QJsonArray &todosArray) {
    qDebug() << "从服务器更新" << todosArray.size() << "个待办事项";
    qDebug() << "当前本地有" << m_todos.size() << "个待办事项";

    beginResetModel();

    // 解析服务器返回的待办事项数据
    for (const QJsonValue &value : todosArray) {
        QJsonObject todoObj = value.toObject();

        // 查找是否已存在相同UUID的项目
        QString uuid = todoObj["uuid"].toString();
        TodoItem *existingItem = nullptr;

        // qDebug() << "处理服务器项目，UUID:" << uuid << ", 标题:" <<
        // todoObj["title"].toString();

        for (const auto &item : m_todos) {
            QString localUuid = item->uuid().toString(QUuid::WithoutBraces); // 去掉花括号
            // qDebug() << "比较本地项目 UUID:" << localUuid << " vs 服务器 UUID:" <<
            // uuid;
            if (localUuid == uuid && !uuid.isEmpty()) {
                existingItem = item.get();
                // qDebug() << "找到现有项目，UUID:" << uuid;
                break;
            }
        }

        if (existingItem) {
            // 更新现有项目
            existingItem->setTitle(todoObj["title"].toString());
            existingItem->setDescription(todoObj["description"].toString());
            existingItem->setCategory(todoObj["category"].toString());
            existingItem->setImportant(todoObj["important"].toBool());
            existingItem->setDeadline(QDateTime::fromString(todoObj["deadline"].toString(), Qt::ISODate));
            existingItem->setRecurrenceInterval(todoObj["recurrenceInterval"].toInt());
            existingItem->setRecurrenceCount(todoObj["recurrenceCount"].toInt());
            existingItem->setRecurrenceStartDate(
                QDate::fromString(todoObj["recurrenceStartDate"].toString(), Qt::ISODate));
            existingItem->setIsCompleted(todoObj["is_completed"].toBool());
            existingItem->setCompletedAt(QDateTime::fromString(todoObj["completed_at"].toString(), Qt::ISODate));
            existingItem->setIsDeleted(todoObj["is_deleted"].toBool());
            existingItem->setDeletedAt(QDateTime::fromString(todoObj["deleted_at"].toString(), Qt::ISODate));
            existingItem->setUpdatedAt(QDateTime::fromString(todoObj["updated_at"].toString(), Qt::ISODate));
            existingItem->setSynced(0);
        } else {
            // 创建新项目
            qDebug() << "未找到现有项目，创建新项目，UUID:" << uuid;
            auto newItem = std::make_unique<TodoItem>();
            newItem->setId(todoObj["id"].toInt());
            newItem->setUuid(QUuid(todoObj["uuid"].toString()));
            newItem->setUserUuid(QUuid(todoObj["user_uuid"].toString()));
            newItem->setTitle(todoObj["title"].toString());
            newItem->setDescription(todoObj["description"].toString());
            newItem->setCategory(todoObj["category"].toString());
            newItem->setImportant(todoObj["important"].toBool());
            newItem->setDeadline(QDateTime::fromString(todoObj["deadline"].toString(), Qt::ISODate));
            newItem->setRecurrenceInterval(todoObj["recurrenceInterval"].toInt());
            newItem->setRecurrenceCount(todoObj["recurrenceCount"].toInt());
            newItem->setRecurrenceStartDate(QDate::fromString(todoObj["recurrenceStartDate"].toString(), Qt::ISODate));
            newItem->setIsCompleted(todoObj["is_completed"].toBool());
            newItem->setCompletedAt(QDateTime::fromString(todoObj["completed_at"].toString(), Qt::ISODate));
            newItem->setIsDeleted(todoObj["is_deleted"].toBool());
            newItem->setDeletedAt(QDateTime::fromString(todoObj["deleted_at"].toString(), Qt::ISODate));
            newItem->setCreatedAt(QDateTime::fromString(todoObj["created_at"].toString(), Qt::ISODate));
            newItem->setUpdatedAt(QDateTime::fromString(todoObj["updated_at"].toString(), Qt::ISODate));
            newItem->setSynced(0);

            m_todos.push_back(std::move(newItem));
        }
    }

    endResetModel();
    清除过滤后的待办();
    rebuildIdIndex();

    // 保存到本地存储
    if (!m_dataManager->saveToLocalStorage(m_todos)) {
        qWarning() << "无法在服务器更新后保存本地存储";
    }

    // 更新同步管理器的数据
    更新同步管理器的数据();
}

// 更新同步管理器的待办事项数据
void TodoManager::更新同步管理器的数据() {
    QList<TodoItem *> todoItems;
    for (const auto &item : m_todos) {
        todoItems.append(item.get());
    }
    m_syncManager->setTodoItems(todoItems);
}

// 访问器方法
TodoQueryer *TodoManager::queryer() const {
    return m_queryer;
}

TodoSyncServer *TodoManager::syncServer() const {
    return m_syncManager;
}

// 排序相关实现
void TodoManager::sortTodos() {
    // 现在排序在数据库端完成，只需标记缓存失效并刷新视图
    beginResetModel();
    清除过滤后的待办();
    endResetModel();
}

// ===================== 索引维护实现 =====================
/**
 * @brief 重建ID索引映射
 * 该方法会清空现有的ID索引映射，并重新遍历m_todos列表，构建新的映射。
 * 适用于在批量修改m_todos后需要重新生成索引的场景。
 */
void TodoManager::rebuildIdIndex() {
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
void TodoManager::addToIndex(TodoItem *item) {
    if (item) {
        m_idIndex[item->id()] = item;
    }
}

/**
 * @brief 从ID索引映射中移除一个待办事项
 */
void TodoManager::removeFromIndex(int id) {
    m_idIndex.erase(id);
}
