/**
 * @brief 构造函数
 *
 * 初始化TodoManager对象，设置父对象为parent。
 *
 * @param parent 父对象指针，默认值为nullptr。
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @version 2025-08-23 21:09:00(UTC+8) 周六
 */
#include "todo_manager.h"

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
#include <algorithm>
#include <map>

#include "category_manager.h"
#include "foundation/network_request.h"
#include "todo_filter.h"
#include "todo_sorter.h"
#include "user_auth.h"

TodoManager::TodoManager(QObject *parent)
    : QAbstractListModel(parent),                      //
      m_filterCacheDirty(true),                        //
      m_networkRequest(NetworkRequest::GetInstance()), //
      m_setting(Setting::GetInstance()),               //
      m_isAutoSync(false)                              //
{

    // 初始化默认服务器配置
    m_setting.initializeDefaultServerConfig();

    // 初始化筛选管理器
    m_filter = new TodoFilter(this);

    // 初始化排序管理器
    m_sorter = new TodoSorter(this);

    // 连接筛选器信号，当筛选条件变化时更新缓存
    connect(m_filter, &TodoFilter::filtersChanged, this, [this]() {
        beginResetModel();
        invalidateFilterCache();
        endResetModel();
    });

    // 连接排序器信号，当排序类型变化时重新排序
    connect(m_sorter, &TodoSorter::sortTypeChanged, this, &TodoManager::sortTodos);

    // 创建数据管理器
    m_dataManager = new TodoDataStorage(m_setting, this);

    // 创建同步管理器
    m_syncManager = new TodoSyncServer(this);

    // 连接同步管理器信号
    connect(m_syncManager, &TodoSyncServer::syncStarted, this, &TodoManager::onSyncStarted);
    connect(m_syncManager, &TodoSyncServer::syncCompleted, this, &TodoManager::onSyncCompleted);
    connect(m_syncManager, &TodoSyncServer::todosUpdatedFromServer, this, &TodoManager::onTodosUpdatedFromServer);

    // 创建待办事项类别管理器
    m_categoryManager = new CategoryManager(m_syncManager, this);
    connect(m_categoryManager, &CategoryManager::categoryOperationCompleted, this,
            &TodoManager::categoryOperationCompleted);

    // 通过数据管理器加载本地数据
    m_dataManager->loadFromLocalStorage(m_todos);

    // 初始化在线状态
    m_isAutoSync = m_setting.get(QStringLiteral("autoSync"), false).toBool();
    m_syncManager->setAutoSyncEnabled(m_isAutoSync);

    // 设置待办事项数据到同步管理器
    updateSyncManagerData();

    // 如果已登录，获取类别列表
    if (m_isAutoSync && UserAuth::GetInstance().isLoggedIn()) {
        m_categoryManager->fetchCategories();
    }
}

/**
 * @brief 析构函数
 *
 * 清理资源，保存未同步的数据到本地存储。
 */
TodoManager::~TodoManager() {
    // 通过数据管理器保存数据
    if (m_dataManager) {
        m_dataManager->saveToLocalStorage(m_todos);
    }

    m_todos.clear();
    m_filteredTodos.clear();
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
    if (!m_filter->hasActiveFilters()) {
        return m_todos.size();
    }

    // 使用缓存的过滤结果
    const_cast<TodoManager *>(this)->updateFilterCache();
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
    if (!m_filter->hasActiveFilters()) {
        if (static_cast<size_t>(index.row()) >= m_todos.size())
            return QVariant();
        return getItemData(m_todos[index.row()].get(), role);
    }

    // 使用缓存的过滤结果
    const_cast<TodoManager *>(this)->updateFilterCache();
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
    case LastModifiedAtRole:
        return item->lastModifiedAt();
    case SyncedRole:
        return item->synced();
    default:
        return QVariant();
    }
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
    roles[LastModifiedAtRole] = "lastModifiedAt";
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
        item->setSynced(false);
        invalidateFilterCache();
        emit dataChanged(index, index, QVector<int>() << role);
        m_dataManager->saveToLocalStorage(m_todos);
        return true;
    }

    return false;
}

// 性能优化相关方法实现
void TodoManager::updateFilterCache() {
    if (!m_filterCacheDirty) {
        return;
    }

    m_filteredTodos.clear();

    // 使用新的筛选器进行筛选
    m_filteredTodos = m_filter->filterTodos(m_todos);

    m_filterCacheDirty = false;
}

TodoItem *TodoManager::getFilteredItem(int index) const {
    const_cast<TodoManager *>(this)->updateFilterCache();

    if (index < 0 || index >= m_filteredTodos.size()) {
        return nullptr;
    }

    return m_filteredTodos[index];
}

void TodoManager::invalidateFilterCache() {
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
                          const QString &deadline) {
    beginInsertRows(QModelIndex(), rowCount(), rowCount());

    auto newItem = std::make_unique<TodoItem>(        //
        0,                                            // id (auto-generated)
        QUuid::createUuid(),                          // uuid
        QUuid(),                                      // userUuid
        title,                                        // title
        description,                                  // description
        category,                                     // category
        important,                                    // important
        QDateTime::fromString(deadline, Qt::ISODate), // deadline
        0,                                            // recurrenceInterval
        -1,                                           // recurrenceCount
        QDate(),                                      // recurrenceStartDate
        false,                                        // isCompleted
        QDateTime(),                                  // completedAt
        false,                                        // isDeleted
        QDateTime(),                                  // deletedAt
        QDateTime::currentDateTime(),                 // createdAt
        QDateTime::currentDateTime(),                 // updatedAt
        QDateTime::currentDateTime(),                 // lastModifiedAt
        false,                                        // synced
        this                                          //
    );

    m_todos.push_back(std::move(newItem));
    invalidateFilterCache();

    endInsertRows();

    m_dataManager->saveToLocalStorage(m_todos);

    if (m_isAutoSync && UserAuth::GetInstance().isLoggedIn()) {
        // 如果在线且已登录，立即尝试同步到服务器
        // syncWithServer();
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

    QModelIndex modelIndex = createIndex(index, 0);
    bool anyUpdated = false;
    TodoItem *item = m_todos[index].get();
    QVector<int> changedRoles;

    try {
        // 直接更新TodoItem对象，避免多次触发dataChanged信号
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
            if (item->deadline() != newDeadline) {
                item->setDeadline(newDeadline);
                changedRoles << DeadlineRole;
                anyUpdated = true;
            }
        }

        if (todoData.contains("recurrence_interval")) {
            int newRecurrenceInterval = todoData["recurrence_interval"].toInt();
            if (item->recurrenceInterval() != newRecurrenceInterval) {
                item->setRecurrenceInterval(newRecurrenceInterval);
                anyUpdated = true;
            }
        }

        if (todoData.contains("recurrence_count")) {
            int newRecurrenceCount = todoData["recurrence_count"].toInt();
            if (item->recurrenceCount() != newRecurrenceCount) {
                item->setRecurrenceCount(newRecurrenceCount);
                anyUpdated = true;
            }
        }

        if (todoData.contains("recurrence_start_date")) {
            QString newRecurrenceStartDateStr = todoData["recurrence_start_date"].toString();
            QDate newRecurrenceStartDate = QDate::fromString(newRecurrenceStartDateStr, Qt::ISODate);
            if (item->recurrenceStartDate() != newRecurrenceStartDate) {
                item->setRecurrenceStartDate(newRecurrenceStartDate);
                anyUpdated = true;
            }
        }

        if (todoData.contains("status")) {
            QString newStatus = todoData["status"].toString();
            bool newCompleted = (newStatus == "done");
            if (item->isCompleted() != newCompleted) {
                item->setIsCompleted(newCompleted);
                if (newCompleted) {
                    item->setCompletedAt(QDateTime::currentDateTime());
                }
                changedRoles << IsCompletedRole;
                anyUpdated = true;
            }
        }

        // 如果有任何更新，则触发一次dataChanged信号并保存
        if (anyUpdated) {
            item->setUpdatedAt(QDateTime::currentDateTime());
            item->setSynced(false);
            invalidateFilterCache();

            // 只触发一次dataChanged信号
            emit dataChanged(modelIndex, modelIndex, changedRoles);

            if (!m_dataManager->saveToLocalStorage(m_todos)) {
                qWarning() << "更新待办事项后无法保存到本地存储";
            }

            if (m_isAutoSync && UserAuth::GetInstance().isLoggedIn()) {
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
        // 软删除
        auto &todoItem = m_todos[index];
        todoItem->setIsDeleted(true);
        todoItem->setDeletedAt(QDateTime::currentDateTime());

        // 通知视图数据已更改
        QModelIndex modelIndex = createIndex(index, 0);
        emit dataChanged(modelIndex, modelIndex);

        // 使筛选缓存失效，以便重新筛选
        invalidateFilterCache();

        if (!m_dataManager->saveToLocalStorage(m_todos)) {
            qWarning() << "软删除待办事项后无法保存到本地存储";
        }

        if (m_isAutoSync && UserAuth::GetInstance().isLoggedIn()) {
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
 * @brief 将待办事项标记为已完成
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

        // 通知视图数据已更改
        QModelIndex modelIndex = createIndex(index, 0);
        emit dataChanged(modelIndex, modelIndex);

        // 使筛选缓存失效，以便重新筛选
        invalidateFilterCache();

        if (!m_dataManager->saveToLocalStorage(m_todos)) {
            qWarning() << "恢复待办事项后无法保存到本地存储";
        }

        if (m_isAutoSync && UserAuth::GetInstance().isLoggedIn()) {
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
        auto &todoItem = m_todos[index];

        // 检查任务是否已删除
        if (!todoItem->isDeleted()) {
            qWarning() << "尝试永久删除未删除的任务，索引:" << index;
            return false;
        }

        // 永久删除：从列表中物理移除
        beginRemoveRows(QModelIndex(), index, index);
        m_todos.erase(m_todos.begin() + index);
        invalidateFilterCache();
        endRemoveRows();

        if (!m_dataManager->saveToLocalStorage(m_todos)) {
            qWarning() << "永久删除待办事项后无法保存到本地存储";
        }

        if (m_isAutoSync && UserAuth::GetInstance().isLoggedIn()) {
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

bool TodoManager::markAsDone(int index) {
    // 检查索引是否有效
    if (index < 0 || static_cast<size_t>(index) >= m_todos.size()) {
        qWarning() << "尝试标记无效索引的待办事项为已完成:" << index;
        return false;
    }

    try {
        QModelIndex modelIndex = createIndex(index, 0);
        bool success = setData(modelIndex, true, IsCompletedRole);

        if (success) {
            if (m_isAutoSync && UserAuth::GetInstance().isLoggedIn()) {
                syncWithServer();
            }
            qDebug() << "成功将索引" << index << "处的待办事项标记为已完成";
        } else {
            qWarning() << "无法将索引" << index << "处的待办事项标记为已完成";
        }

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
    updateSyncManagerData();

    // 委托给同步管理器处理
    m_syncManager->syncWithServer(TodoSyncServer::Bidirectional);
}

// 同步管理器信号处理槽函数
void TodoManager::onSyncStarted() {
    emit syncStarted();
}

void TodoManager::onSyncCompleted(TodoSyncServer::SyncResult result, const QString &message) {
    bool success = (result == TodoSyncServer::Success);
    emit syncCompleted(success, message);

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

    beginResetModel();

    // 解析服务器返回的待办事项数据
    for (const QJsonValue &value : todosArray) {
        QJsonObject todoObj = value.toObject();

        // 查找是否已存在相同UUID的项目
        QString uuid = todoObj["uuid"].toString();
        TodoItem *existingItem = nullptr;

        for (const auto &item : m_todos) {
            if (item->uuid().toString() == uuid) {
                existingItem = item.get();
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
            existingItem->setRecurrenceInterval(todoObj["recurrence_interval"].toInt());
            existingItem->setRecurrenceCount(todoObj["recurrence_count"].toInt());
            existingItem->setRecurrenceStartDate(
                QDate::fromString(todoObj["recurrence_start_date"].toString(), Qt::ISODate));
            existingItem->setIsCompleted(todoObj["is_completed"].toBool());
            existingItem->setCompletedAt(QDateTime::fromString(todoObj["completed_at"].toString(), Qt::ISODate));
            existingItem->setIsDeleted(todoObj["is_deleted"].toBool());
            existingItem->setDeletedAt(QDateTime::fromString(todoObj["deleted_at"].toString(), Qt::ISODate));
            existingItem->setUpdatedAt(QDateTime::fromString(todoObj["updated_at"].toString(), Qt::ISODate));
            existingItem->setLastModifiedAt(QDateTime::fromString(todoObj["last_modified_at"].toString(), Qt::ISODate));
            // Note: setStatus method does not exist in TodoItem class
            existingItem->setSynced(true);
        } else {
            // 创建新项目
            auto newItem = std::make_unique<TodoItem>();
            newItem->setId(todoObj["id"].toInt());
            newItem->setUuid(QUuid(todoObj["uuid"].toString()));
            newItem->setUserUuid(QUuid(todoObj["user_uuid"].toString()));
            newItem->setTitle(todoObj["title"].toString());
            newItem->setDescription(todoObj["description"].toString());
            newItem->setCategory(todoObj["category"].toString());
            newItem->setImportant(todoObj["important"].toBool());
            newItem->setDeadline(QDateTime::fromString(todoObj["deadline"].toString(), Qt::ISODate));
            newItem->setRecurrenceInterval(todoObj["recurrence_interval"].toInt());
            newItem->setRecurrenceCount(todoObj["recurrence_count"].toInt());
            newItem->setRecurrenceStartDate(
                QDate::fromString(todoObj["recurrence_start_date"].toString(), Qt::ISODate));
            newItem->setIsCompleted(todoObj["is_completed"].toBool());
            newItem->setCompletedAt(QDateTime::fromString(todoObj["completed_at"].toString(), Qt::ISODate));
            newItem->setIsDeleted(todoObj["is_deleted"].toBool());
            newItem->setDeletedAt(QDateTime::fromString(todoObj["deleted_at"].toString(), Qt::ISODate));
            newItem->setCreatedAt(QDateTime::fromString(todoObj["created_at"].toString(), Qt::ISODate));
            newItem->setUpdatedAt(QDateTime::fromString(todoObj["updated_at"].toString(), Qt::ISODate));
            newItem->setLastModifiedAt(QDateTime::fromString(todoObj["last_modified_at"].toString(), Qt::ISODate));
            // newItem->setStatus(todoObj["status"].toString());
            newItem->setSynced(true);

            m_todos.push_back(std::move(newItem));
        }
    }

    endResetModel();
    invalidateFilterCache();

    // 保存到本地存储
    if (!m_dataManager->saveToLocalStorage(m_todos)) {
        qWarning() << "无法在服务器更新后保存本地存储";
    }
}

// 更新同步管理器的待办事项数据
void TodoManager::updateSyncManagerData() {
    QList<TodoItem *> todoItems;
    for (const auto &item : m_todos) {
        todoItems.append(item.get());
    }
    m_syncManager->setTodoItems(todoItems);
}

// 访问器方法
TodoFilter *TodoManager::filter() const {
    return m_filter;
}

TodoSorter *TodoManager::sorter() const {
    return m_sorter;
}

// 排序相关实现
void TodoManager::sortTodos() {
    if (m_todos.empty()) {
        return;
    }

    beginResetModel();

    // 委托给排序器进行排序
    m_sorter->sortTodos(m_todos);

    // 排序后需要更新过滤缓存
    invalidateFilterCache();

    endResetModel();

    // 保存到本地存储
    m_dataManager->saveToLocalStorage(m_todos);
}