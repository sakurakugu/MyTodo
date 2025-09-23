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
    : QObject(parent),                                   //
      m_filterCacheDirty(true),                          //
      m_networkRequest(NetworkRequest::GetInstance()),   //
      m_userAuth(userAuth),                              //
      m_globalState(GlobalState::GetInstance()),         //
      m_syncManager(new TodoSyncServer(userAuth, this)), //
      m_dataManager(new TodoDataStorage(this)),          //
      m_categoryManager(&categoryManager),               //
      m_queryer(new TodoQueryer(this)),                  //
      m_todoModel(new TodoModel(m_queryer, this))        //
{

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
        beginResetModel();
        if (!deleteLocal) {
            qDebug() << "不删除本地数据，只更新用户UUID";
            m_dataManager->更新所有待办用户UUID(m_todos, m_userAuth.getUuid(), 1);

            更新同步管理器的数据();
        } else {
            // 删除本地数据
            if (m_todos.empty()) {
                qDebug() << "没有待办事项需要删除";
            }

            // 清空所有待办事项
            m_dataManager->删除所有待办(m_todos);
            m_idIndex.clear();
        }

        // 使筛选缓存失效
        清除过滤后的待办();

        // 结束重置模型
        endResetModel();

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
    qDebug() << "从服务器更新" << todosArray.size() << "个待办事项，当前本地有" << m_todos.size() << "个待办事项";

    beginResetModel();
    m_dataManager->导入类别从JSON(m_todos, todosArray);
    endResetModel();

    清除过滤后的待办();
    rebuildIdIndex();
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
