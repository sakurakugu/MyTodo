/**
 * @brief 构造函数
 *
 * 初始化TodoModel对象，设置父对象为parent。
 *
 * @param parent 父对象指针，默认值为nullptr。
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @version 2025-08-23 21:09:00(UTC+8) 周六
 */
#include "todo_model.h"

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

#include "foundation/network_request.h"

TodoModel::TodoModel(QObject *parent)
    : QAbstractListModel(parent), m_filterCacheDirty(true), m_isOnline(false), m_currentCategory(""),
      m_currentFilter(""), m_currentImportant(false), m_dateFilterEnabled(false),
      m_networkRequest(NetworkRequest::GetInstance()), m_setting(Setting::GetInstance()),
      m_sortType(SortByCreatedTime) {

    // 初始化默认类别列表
    m_categories << "全部" << "工作" << "学习" << "生活" << "其他" << "未分类";
    // 初始化默认服务器配置
    m_setting.initializeDefaultServerConfig();

    // 初始化服务器配置
    initializeServerConfig();

    // 连接网络管理器信号
    connect(&m_networkRequest, &NetworkRequest::requestCompleted, this, &TodoModel::onNetworkRequestCompleted);
    connect(&m_networkRequest, &NetworkRequest::requestFailed, this, &TodoModel::onNetworkRequestFailed);
    connect(&m_networkRequest, &NetworkRequest::networkStatusChanged, this, &TodoModel::onNetworkStatusChanged);
    connect(&m_networkRequest, &NetworkRequest::authTokenExpired, this, &TodoModel::onAuthTokenExpired);

    // 加载本地数据
    if (!loadFromLocalStorage()) {
        qWarning() << "无法从本地存储加载待办事项数据";
    }

    // 初始化在线状态
    m_isOnline = m_setting.get(QStringLiteral("autoSync"), false).toBool();
    emit isOnlineChanged();

    // 尝试使用存储的令牌自动登录
    if (m_setting.contains(QStringLiteral("user/accessToken"))) {
        m_accessToken = m_setting.get(QStringLiteral("user/accessToken")).toString();
        m_refreshToken = m_setting.get(QStringLiteral("user/refreshToken")).toString();
        m_username = m_setting.get(QStringLiteral("user/username")).toString();

        // TODO: 在这里验证令牌是否有效
        qDebug() << "使用存储的凭据自动登录用户：" << m_username;

        // 如果已登录，获取类别列表
        if (m_isOnline) {
            fetchCategories();
        }
    }
}

/**
 * @brief 析构函数
 *
 * 清理资源，保存未同步的数据到本地存储。
 */
TodoModel::~TodoModel() {
    // 保存数据
    saveToLocalStorage();

    m_todos.clear();
    m_filteredTodos.clear();
}

/**
 * @brief 获取模型中的行数（待办事项数量）
 * @param parent 父索引，默认为无效索引（根）
 * @return 待办事项数量
 */
int TodoModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;

    // 如果没有设置过滤条件，返回所有项目
    if (m_currentCategory.isEmpty() && m_currentFilter.isEmpty()) {
        return m_todos.size();
    }

    // 使用缓存的过滤结果
    const_cast<TodoModel *>(this)->updateFilterCache();
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
    if (m_currentCategory.isEmpty() && m_currentFilter.isEmpty()) {
        if (static_cast<size_t>(index.row()) >= m_todos.size())
            return QVariant();
        return getItemData(m_todos[index.row()].get(), role);
    }

    // 使用缓存的过滤结果
    const_cast<TodoModel *>(this)->updateFilterCache();
    if (index.row() >= m_filteredTodos.size())
        return QVariant();

    return getItemData(m_filteredTodos[index.row()], role);
}

// 辅助方法，根据角色返回项目数据
QVariant TodoModel::getItemData(const TodoItem *item, int role) const {
    switch (role) {
    case IdRole:
        return item->id();
    case UuidRole:
        return item->uuid();
    case UserIdRole:
        return item->userId();
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
QHash<int, QByteArray> TodoModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[UuidRole] = "uuid";
    roles[UserIdRole] = "userId";
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
    }

    if (changed) {
        item->setUpdatedAt(QDateTime::currentDateTime());
        item->setSynced(false);
        invalidateFilterCache();
        emit dataChanged(index, index, QVector<int>() << role);
        saveToLocalStorage();
        return true;
    }

    return false;
}

// 性能优化相关方法实现
void TodoModel::updateFilterCache() {
    if (!m_filterCacheDirty) {
        return; // 缓存仍然有效
    }

    m_filteredTodos.clear();

    // 如果没有过滤条件，缓存所有项目
    if (m_currentCategory.isEmpty() && m_currentFilter.isEmpty()) {
        for (auto it = m_todos.begin(); it != m_todos.end(); ++it) {
            m_filteredTodos.append(it->get());
        }
    } else {
        // 应用过滤条件
        for (auto it = m_todos.begin(); it != m_todos.end(); ++it) {
            if (itemMatchesFilter(it->get())) {
                m_filteredTodos.append(it->get());
            }
        }
    }

    m_filterCacheDirty = false;
}

bool TodoModel::itemMatchesFilter(const TodoItem *item) const {
    if (!item)
        return false;

    bool categoryMatch =
        m_currentCategory.isEmpty() || item->category() == m_currentCategory ||
        (m_currentCategory == "uncategorized" && (item->category().isEmpty() || item->category() == "uncategorized"));
    // 状态筛选逻辑：
    // - 如果m_currentFilter为"recycle"，则只显示已删除的项目
    // - 否则只显示未删除的项目，并根据完成状态进一步筛选
    bool statusMatch = true;
    if (m_currentFilter == "recycle") {
        // 回收站模式：只显示已删除的项目
        statusMatch = item->isDeleted();
    } else {
        // 正常模式：只显示未删除的项目
        statusMatch = !item->isDeleted();

        // 在未删除的项目中进一步筛选完成状态
        if (!m_currentFilter.isEmpty()) {
            if (m_currentFilter == "done") {
                statusMatch = statusMatch && item->isCompleted();
            } else if (m_currentFilter == "todo") {
                statusMatch = statusMatch && !item->isCompleted();
            }
        }
    }

    // 重要性筛选逻辑：
    // - 如果m_currentImportant为false且m_currentFilter不是"important"，则显示所有项目
    // - 如果m_currentImportant为true，则只显示重要的项目
    // - 如果m_currentImportant为false且需要筛选，则只显示不重要的项目
    bool importantMatch = true; // 默认显示所有
    if (m_currentFilter == "important") {
        importantMatch = (item->important() == m_currentImportant);
    }

    // 日期筛选逻辑：
    // 如果启用了日期筛选，检查任务的截止日期是否在指定范围内
    bool dateMatch = true; // 默认显示所有
    if (m_dateFilterEnabled && item->deadline().isValid()) {
        QDate itemDate = item->deadline().date();
        bool startMatch = !m_dateFilterStart.isValid() || itemDate >= m_dateFilterStart;
        bool endMatch = !m_dateFilterEnd.isValid() || itemDate <= m_dateFilterEnd;
        dateMatch = startMatch && endMatch;
    } else if (m_dateFilterEnabled && !item->deadline().isValid()) {
        // 如果启用了日期筛选但任务没有截止日期，则不显示
        dateMatch = false;
    }

    return categoryMatch && statusMatch && importantMatch && dateMatch;
}

TodoItem *TodoModel::getFilteredItem(int index) const {
    const_cast<TodoModel *>(this)->updateFilterCache();

    if (index < 0 || index >= m_filteredTodos.size()) {
        return nullptr;
    }

    return m_filteredTodos[index];
}

void TodoModel::invalidateFilterCache() {
    m_filterCacheDirty = true;
}

/**
 * @brief 获取当前在线状态
 * @return 是否在线
 */
bool TodoModel::isOnline() const {
    return m_isOnline;
}

/**
 * @brief 设置在线状态
 * @param online 新的在线状态
 */
void TodoModel::setIsOnline(bool online) {
    // 如果已经是目标状态，则不做任何操作
    if (m_isOnline == online) {
        return;
    }

    if (online) {
        // TODO: 这部分是不是可以复用

        // 尝试连接服务器，验证是否可以切换到在线模式
        NetworkRequest::RequestConfig config;
        config.url = getApiUrl(m_todoApiEndpoint);
        config.requiresAuth = isLoggedIn();
        config.timeout = 5000; // 5秒超时

        // 发送测试请求
        m_networkRequest.sendRequest(NetworkRequest::FetchTodos, config);

        // TODO: 暂时设置为在线状态，实际状态将在请求回调中确定
        m_isOnline = online;
        emit isOnlineChanged();
        // 保存到设置，保持与autoSync一致
        m_setting.save(QStringLiteral("setting/autoSync"), m_isOnline);

        if (m_isOnline && isLoggedIn()) {
            syncWithServer();
        }
    } else {
        // 切换到离线模式不需要验证，直接更新状态
        m_isOnline = online;
        emit isOnlineChanged();
        // 保存到设置，保持与autoSync一致
        m_setting.save(QStringLiteral("setting/autoSync"), m_isOnline);
    }
}

/**
 * @brief 获取当前激活的分类筛选器
 * @return 当前分类名称
 */
QString TodoModel::currentCategory() const {
    return m_currentCategory;
}

/**
 * @brief 设置分类筛选器
 * @param category 分类名称，空字符串表示显示所有分类
 */
void TodoModel::setCurrentCategory(const QString &category) {
    if (m_currentCategory != category) {
        beginResetModel();
        m_currentCategory = category;
        invalidateFilterCache();
        endResetModel();
        emit currentCategoryChanged();
    }
}

/**
 * @brief 获取当前激活的筛选条件
 * @return 当前筛选条件
 */
QString TodoModel::currentFilter() const {
    return m_currentFilter;
}

/**
 * @brief 设置筛选条件
 * @param filter 筛选条件（如"完成"、"未完成"等）
 */
void TodoModel::setCurrentFilter(const QString &filter) {
    if (m_currentFilter != filter) {
        beginResetModel();
        m_currentFilter = filter;
        invalidateFilterCache();
        endResetModel();
        emit currentFilterChanged();
    }
}

/**
 * @brief 获取当前激活的重要程度筛选器
 * @return 当前重要程度筛选器
 */
bool TodoModel::currentImportant() const {
    return m_currentImportant;
}

/**
 * @brief 设置重要程度筛选器
 * @param important 重要程度筛选器（true表示重要，false表示不重要）
 */
void TodoModel::setCurrentImportant(bool important) {
    if (m_currentImportant != important) {
        beginResetModel();
        m_currentImportant = important;
        invalidateFilterCache();
        endResetModel();
        emit currentImportantChanged();
    }
}

/**
 * @brief 获取日期筛选开始日期
 * @return 日期筛选开始日期
 */
QDate TodoModel::dateFilterStart() const {
    return m_dateFilterStart;
}

/**
 * @brief 设置日期筛选开始日期
 * @param date 开始日期
 */
void TodoModel::setDateFilterStart(const QDate &date) {
    if (m_dateFilterStart != date) {
        beginResetModel();
        m_dateFilterStart = date;
        invalidateFilterCache();
        endResetModel();
        emit dateFilterStartChanged();
    }
}

/**
 * @brief 获取日期筛选结束日期
 * @return 日期筛选结束日期
 */
QDate TodoModel::dateFilterEnd() const {
    return m_dateFilterEnd;
}

/**
 * @brief 设置日期筛选结束日期
 * @param date 结束日期
 */
void TodoModel::setDateFilterEnd(const QDate &date) {
    if (m_dateFilterEnd != date) {
        beginResetModel();
        m_dateFilterEnd = date;
        invalidateFilterCache();
        endResetModel();
        emit dateFilterEndChanged();
    }
}

/**
 * @brief 获取日期筛选是否启用
 * @return 日期筛选是否启用
 */
bool TodoModel::dateFilterEnabled() const {
    return m_dateFilterEnabled;
}

/**
 * @brief 设置日期筛选是否启用
 * @param enabled 是否启用日期筛选
 */
void TodoModel::setDateFilterEnabled(bool enabled) {
    if (m_dateFilterEnabled != enabled) {
        beginResetModel();
        m_dateFilterEnabled = enabled;
        invalidateFilterCache();
        endResetModel();
        emit dateFilterEnabledChanged();
    }
}

/**
 * @brief 添加新的待办事项
 * @param title 任务标题（必填）
 * @param description 任务描述（可选）
 * @param category 任务分类（默认为"default"）
 * @param important 重要程度（默认为"medium"）
 */
void TodoModel::addTodo(const QString &title, const QString &description, const QString &category, bool important,
                        const QString &deadline) {
    beginInsertRows(QModelIndex(), rowCount(), rowCount());

    auto newItem = std::make_unique<TodoItem>(0,                                            // id (auto-generated)
                                              QUuid::createUuid(),                          // uuid
                                              0,                                            // userId
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
                                              this);

    m_todos.push_back(std::move(newItem));
    invalidateFilterCache();

    endInsertRows();

    saveToLocalStorage();

    if (m_isOnline && isLoggedIn()) {
        // 如果在线且已登录，立即尝试同步到服务器
        syncWithServer();
    }
}

/**
 * @brief 更新现有待办事项
 * @param index 待更新事项的索引
 * @param todoData 包含要更新字段的映射
 * @return 更新是否成功
 */
bool TodoModel::updateTodo(int index, const QVariantMap &todoData) {
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

            if (!saveToLocalStorage()) {
                qWarning() << "更新待办事项后无法保存到本地存储";
            }

            if (m_isOnline && isLoggedIn()) {
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
        logError("更新待办事项", QString("异常: %1").arg(e.what()));
        return false;
    } catch (...) {
        qCritical() << "更新待办事项时发生未知异常";
        logError("更新待办事项", "未知异常");
        return false;
    }
}

/**
 * @brief 删除待办事项
 * @param index 待删除事项的索引
 * @return 删除是否成功
 */
bool TodoModel::removeTodo(int index) {
    // 检查索引是否有效
    if (index < 0 || static_cast<size_t>(index) >= m_todos.size()) {
        qWarning() << "尝试删除无效的索引:" << index;
        return false;
    }

    try {
        // 软删除：设置isDeleted为true和deletedAt时间戳，而不是物理删除
        auto &todoItem = m_todos[index];
        todoItem->setIsDeleted(true);
        todoItem->setDeletedAt(QDateTime::currentDateTime());

        // 通知视图数据已更改
        QModelIndex modelIndex = createIndex(index, 0);
        emit dataChanged(modelIndex, modelIndex);

        // 使筛选缓存失效，以便重新筛选
        invalidateFilterCache();

        if (!saveToLocalStorage()) {
            qWarning() << "软删除待办事项后无法保存到本地存储";
        }

        if (m_isOnline && isLoggedIn()) {
            syncWithServer();
        }

        qDebug() << "成功软删除索引" << index << "处的待办事项";
        return true;
    } catch (const std::exception &e) {
        qCritical() << "软删除待办事项时发生异常:" << e.what();
        logError("软删除待办事项", QString("异常: %1").arg(e.what()));
        return false;
    } catch (...) {
        qCritical() << "软删除待办事项时发生未知异常";
        logError("软删除待办事项", "未知异常");
        return false;
    }
}

/**
 * @brief 将待办事项标记为已完成
 * @param index 待办事项的索引
 * @return 操作是否成功
 */
bool TodoModel::restoreTodo(int index) {
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

        if (!saveToLocalStorage()) {
            qWarning() << "恢复待办事项后无法保存到本地存储";
        }

        if (m_isOnline && isLoggedIn()) {
            syncWithServer();
        }

        qDebug() << "成功恢复索引" << index << "处的待办事项";
        return true;
    } catch (const std::exception &e) {
        qCritical() << "恢复待办事项时发生异常:" << e.what();
        logError("恢复待办事项", QString("异常: %1").arg(e.what()));
        return false;
    } catch (...) {
        qCritical() << "恢复待办事项时发生未知异常";
        logError("恢复待办事项", "未知异常");
        return false;
    }
}

bool TodoModel::permanentlyDeleteTodo(int index) {
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

        if (!saveToLocalStorage()) {
            qWarning() << "永久删除待办事项后无法保存到本地存储";
        }

        if (m_isOnline && isLoggedIn()) {
            syncWithServer();
        }

        qDebug() << "成功永久删除索引" << index << "处的待办事项";
        return true;
    } catch (const std::exception &e) {
        qCritical() << "永久删除待办事项时发生异常:" << e.what();
        logError("永久删除待办事项", QString("异常: %1").arg(e.what()));
        return false;
    } catch (...) {
        qCritical() << "永久删除待办事项时发生未知异常";
        logError("永久删除待办事项", "未知异常");
        return false;
    }
}

bool TodoModel::markAsDone(int index) {
    // 检查索引是否有效
    if (index < 0 || static_cast<size_t>(index) >= m_todos.size()) {
        qWarning() << "尝试标记无效索引的待办事项为已完成:" << index;
        return false;
    }

    try {
        QModelIndex modelIndex = createIndex(index, 0);
        bool success = setData(modelIndex, true, IsCompletedRole);

        if (success) {
            if (m_isOnline && isLoggedIn()) {
                syncWithServer();
            }
            qDebug() << "成功将索引" << index << "处的待办事项标记为已完成";
        } else {
            qWarning() << "无法将索引" << index << "处的待办事项标记为已完成";
        }

        return success;
    } catch (const std::exception &e) {
        qCritical() << "标记待办事项为已完成时发生异常:" << e.what();
        logError("标记为已完成", QString("异常: %1").arg(e.what()));
        return false;
    } catch (...) {
        qCritical() << "标记待办事项为已完成时发生未知异常";
        logError("标记为已完成", "未知异常");
        return false;
    }
}

/**
 * @brief 与服务器同步待办事项数据
 *
 * 该方法会先获取服务器上的最新数据，然后将本地更改推送到服务器。
 * 操作结果通过syncCompleted信号通知。
 */
void TodoModel::syncWithServer() {
    // 检查是否可以进行同步
    if (!m_isOnline) {
        qDebug() << "无法同步：离线模式";
        return;
    }

    if (!isLoggedIn()) {
        qDebug() << "无法同步：未登录";
        return;
    }

    qDebug() << "开始同步待办事项...";
    emit syncStarted();

    // 准备同步请求配置
    NetworkRequest::RequestConfig config;
    config.url = getApiUrl(m_todoApiEndpoint);
    config.requiresAuth = true;

    // 发送同步请求
    m_networkRequest.sendRequest(NetworkRequest::RequestType::Sync, config);
}

/**
 * @brief 使用用户凭据登录服务器
 * @param username 用户名
 * @param password 密码
 *
 * 登录结果会通过loginSuccessful或loginFailed信号通知。
 */
void TodoModel::login(const QString &username, const QString &password) {
    if (username.isEmpty() || password.isEmpty()) {
        qWarning() << "尝试使用空的用户名或密码登录";
        emit loginFailed("用户名和密码不能为空");
        return;
    }

    qDebug() << "尝试登录用户:" << username;

    // 准备请求配置
    NetworkRequest::RequestConfig config;
    config.url = getApiUrl(m_authApiEndpoint) + "?action=login";
    config.requiresAuth = false; // 登录请求不需要认证

    // 创建登录数据
    config.data["username"] = username;
    config.data["password"] = password;

    // 发送登录请求
    emit syncStarted();
    m_networkRequest.sendRequest(NetworkRequest::RequestType::Login, config);
}

/**
 * @brief 注销当前用户
 *
 * 清除存储的凭据并将所有项标记为未同步。
 */
void TodoModel::logout() {
    m_accessToken.clear();
    m_refreshToken.clear();
    m_username.clear();

    m_setting.remove(QStringLiteral("user/accessToken"));
    m_setting.remove(QStringLiteral("user/refreshToken"));
    m_setting.remove(QStringLiteral("user/username"));

    // 标记所有项为未同步
    for (size_t i = 0; i < m_todos.size(); ++i) {
        m_todos[i]->setSynced(false);
    }

    // 发出用户名变化信号
    emit usernameChanged();
    // 发出登录状态变化信号
    emit isLoggedInChanged();
    // 发出退出登录成功信号
    // qDebug() << "用户" << m_username << "已成功退出登录";
    // emit logoutSuccessful();
}

/**
 * @brief 检查用户是否已登录
 * @return 是否已登录
 */
bool TodoModel::isLoggedIn() const {
    return !m_accessToken.isEmpty();
}

/**
 * @brief 获取用户名
 * @return 用户名
 */
QString TodoModel::getUsername() const {
    return m_username;
}

/**
 * @brief 获取邮箱
 * @return 邮箱
 */
QString TodoModel::getEmail() const {
    return m_email;
}

void TodoModel::onNetworkRequestCompleted(NetworkRequest::RequestType type, const QJsonObject &response) {
    switch (type) {
    case NetworkRequest::RequestType::Login:
        handleLoginSuccess(response);
        break;
    case NetworkRequest::RequestType::Sync:
        handleSyncSuccess(response);
        break;
    case NetworkRequest::RequestType::FetchTodos:
        handleFetchTodosSuccess(response);
        break;
    case NetworkRequest::RequestType::PushTodos:
        handlePushChangesSuccess(response);
        break;
    case NetworkRequest::RequestType::Logout:
        // 注销成功处理
        emit logoutSuccessful();
        break;
    case NetworkRequest::RequestType::FetchCategories:
        handleFetchCategoriesSuccess(response);
        break;
    case NetworkRequest::RequestType::CreateCategory:
    case NetworkRequest::RequestType::UpdateCategory:
    case NetworkRequest::RequestType::DeleteCategory:
        handleCategoryOperationSuccess(response);
        break;
    }
}

void TodoModel::onNetworkRequestFailed(NetworkRequest::RequestType type, NetworkRequest::NetworkError error,
                                       const QString &errorMessage) {
    Q_UNUSED(error) // 标记未使用的参数

    QString typeStr;
    switch (type) {
    case NetworkRequest::RequestType::Login:
        typeStr = "登录";
        emit loginFailed(errorMessage);
        break;
    case NetworkRequest::RequestType::Sync:
        typeStr = "同步";
        emit syncCompleted(false, errorMessage);
        break;
    case NetworkRequest::RequestType::FetchTodos:
        typeStr = "获取待办事项";
        emit syncCompleted(false, errorMessage);
        break;
    case NetworkRequest::RequestType::PushTodos:
        typeStr = "推送更改";
        emit syncCompleted(false, errorMessage);
        break;
    case NetworkRequest::RequestType::Logout:
        typeStr = "注销";
        emit logoutSuccessful();
        break;
    case NetworkRequest::RequestType::FetchCategories:
        typeStr = "获取类别";
        emit categoryOperationCompleted(false, errorMessage);
        break;
    case NetworkRequest::RequestType::CreateCategory:
        typeStr = "创建类别";
        emit categoryOperationCompleted(false, errorMessage);
        break;
    case NetworkRequest::RequestType::UpdateCategory:
        typeStr = "更新类别";
        emit categoryOperationCompleted(false, errorMessage);
        break;
    case NetworkRequest::RequestType::DeleteCategory:
        typeStr = "删除类别";
        emit categoryOperationCompleted(false, errorMessage);
        break;
    }

    qWarning() << typeStr << "失败:" << errorMessage;
    logError(typeStr, errorMessage);
}

void TodoModel::onNetworkStatusChanged(bool isOnline) {
    if (m_isOnline != isOnline) {
        m_isOnline = isOnline;
        emit isOnlineChanged();
        qDebug() << "网络状态变更:" << (isOnline ? "在线" : "离线");
    }
}

void TodoModel::onAuthTokenExpired() {
    qWarning() << "认证令牌已过期，需要重新登录";
    logout();
    emit loginRequired();
}

void TodoModel::handleLoginSuccess(const QJsonObject &response) {

    // 验证响应中包含必要的字段
    if (!response.contains("access_token") || !response.contains("refresh_token") || !response.contains("user")) {
        emit loginFailed("服务器响应缺少必要字段");
        return;
    }

    m_accessToken = response["access_token"].toString();
    m_refreshToken = response["refresh_token"].toString();
    m_username = response["user"].toObject()["username"].toString();

    // 设置网络管理器的认证令牌
    m_networkRequest.setAuthToken(m_accessToken);

    // 保存令牌
    m_setting.save(QStringLiteral("user/accessToken"), m_accessToken);
    m_setting.save(QStringLiteral("user/refreshToken"), m_refreshToken);
    m_setting.save(QStringLiteral("user/username"), m_username);

    qDebug() << "用户" << m_username << "登录成功";
    // 发出用户名变化信号
    emit usernameChanged();
    // 发出登录状态变化信号
    emit isLoggedInChanged();
    // 发出登录成功信号，不知道为什么会发两次该信号
    emit loginSuccessful(m_username);
    // 登录成功后获取类别列表和自动同步
    if (m_isOnline) {
        fetchCategories();
        syncWithServer();
    }
}

void TodoModel::handleSyncSuccess(const QJsonObject &response) {
    qDebug() << "同步成功";

    // 处理同步响应数据
    if (response.contains("todos")) {
        QJsonArray todosArray = response["todos"].toArray();
        // 更新本地数据
        updateTodosFromServer(todosArray);
    }

    emit syncCompleted(true, "同步完成");
}

void TodoModel::handleFetchTodosSuccess(const QJsonObject &response) {
    qDebug() << "获取待办事项成功";

    if (response.contains("todos")) {
        QJsonArray todosArray = response["todos"].toArray();
        updateTodosFromServer(todosArray);
    }

    // 成功获取数据后，推送本地更改
    pushLocalChangesToServer();

    emit syncCompleted(true, "数据获取完成");
}

void TodoModel::handlePushChangesSuccess(const QJsonObject &response) {
    qDebug() << "推送更改成功";

    // 标记待同步项目为已同步
    for (TodoItem *item : m_pendingUnsyncedItems) {
        item->setSynced(true);
    }

    // 清空待同步项目列表
    m_pendingUnsyncedItems.clear();

    // 保存到本地存储
    if (!saveToLocalStorage()) {
        qWarning() << "无法在同步后保存本地存储";
    }

    // 处理推送响应
    if (response.contains("updated_count")) {
        int updatedCount = response["updated_count"].toInt();
        qDebug() << "已更新" << updatedCount << "个待办事项";
    }

    emit syncCompleted(true, "更改推送完成");
}

void TodoModel::updateTodosFromServer(const QJsonArray &todosArray) {
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
            newItem->setUuid(QUuid::fromString(uuid));
            newItem->setUserId(todoObj["user_id"].toInt());
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
    if (!saveToLocalStorage()) {
        qWarning() << "无法在服务器更新后保存本地存储";
    }
}

/**
 * @brief 从本地存储加载待办事项
 * @return 加载是否成功
 */
bool TodoModel::loadFromLocalStorage() {
    beginResetModel();
    bool success = true;

    try {
        // 清除当前数据
        m_todos.clear();
        invalidateFilterCache();

        // 从设置中加载数据
        int count = m_setting.get(QStringLiteral("todos/size"), 0).toInt();
        qDebug() << "从本地存储加载" << count << "个待办事项";

        for (int i = 0; i < count; ++i) {
            QString prefix = QString("todos/%1/").arg(i);

            // 验证必要字段
            if (!m_setting.contains(prefix + "id") || !m_setting.contains(prefix + "title")) {
                qWarning() << "跳过无效的待办事项记录（索引" << i << "）：缺少必要字段";
                continue;
            }

            auto item = std::make_unique<TodoItem>(
                m_setting.get(prefix + "id").toInt(),                                              // id
                QUuid::fromString(m_setting.get(prefix + "uuid").toString()),                      // uuid
                m_setting.get(prefix + "userId", 0).toInt(),                                       // userId
                m_setting.get(prefix + "title").toString(),                                        // title
                m_setting.get(prefix + "description").toString(),                                  // description
                m_setting.get(prefix + "category").toString(),                                     // category
                m_setting.get(prefix + "important").toBool(),                                      // important
                QDateTime::fromString(m_setting.get(prefix + "deadline").toString(), Qt::ISODate), // deadline
                m_setting.get(prefix + "recurrenceInterval", 0).toInt(),                           // recurrenceInterval
                m_setting.get(prefix + "recurrenceCount", -1).toInt(),                             // recurrenceCount
                QDate::fromString(m_setting.get(prefix + "recurrenceStartDate").toString(),
                                  Qt::ISODate),                        // recurrenceStartDate
                m_setting.get(prefix + "isCompleted", false).toBool(), // isCompleted
                m_setting.get(prefix + "completedAt").toDateTime(),    // completedAt
                m_setting.get(prefix + "isDeleted", false).toBool(),   // isDeleted
                m_setting.get(prefix + "deletedAt").toDateTime(),      // deletedAt
                m_setting.get(prefix + "createdAt").toDateTime(),      // createdAt
                m_setting.get(prefix + "updatedAt").toDateTime(),      // updatedAt
                m_setting.get(prefix + "lastModifiedAt").toDateTime(), // lastModifiedAt
                m_setting.get(prefix + "synced").toBool(),             // synced
                this);

            m_todos.push_back(std::move(item));
        }
    } catch (const std::exception &e) {
        qCritical() << "加载本地存储时发生异常:" << e.what();
        success = false;
        logError("加载本地存储", QString("异常: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "加载本地存储时发生未知异常";
        success = false;
        logError("加载本地存储", "未知异常");
    }

    endResetModel();
    return success;
}

/**
 * @brief 将待办事项保存到本地存储
 * @return 保存是否成功
 */
bool TodoModel::saveToLocalStorage() {
    bool success = true;

    try {

        // 保存待办事项数量
        m_setting.save(QStringLiteral("todos/size"), m_todos.size());

        // 保存每个待办事项
        for (size_t i = 0; i < m_todos.size(); ++i) {
            const TodoItem *item = m_todos.at(i).get();
            QString prefix = QString("todos/%1/").arg(i);

            m_setting.save(prefix + "id", item->id());
            m_setting.save(prefix + "uuid", item->uuid());
            m_setting.save(prefix + "userId", item->userId());
            m_setting.save(prefix + "title", item->title());
            m_setting.save(prefix + "description", item->description());
            m_setting.save(prefix + "category", item->category());
            m_setting.save(prefix + "important", item->important());
            // m_setting.save(prefix + "status", item->status());
            m_setting.save(prefix + "createdAt", item->createdAt());
            m_setting.save(prefix + "updatedAt", item->updatedAt());
            m_setting.save(prefix + "synced", item->synced());
            m_setting.save(prefix + "deadline", item->deadline());
            m_setting.save(prefix + "recurrenceInterval", item->recurrenceInterval());
            m_setting.save(prefix + "recurrenceCount", item->recurrenceCount());
            m_setting.save(prefix + "recurrenceStartDate", item->recurrenceStartDate());
            m_setting.save(prefix + "isCompleted", item->isCompleted());
            m_setting.save(prefix + "completedAt", item->completedAt());
            m_setting.save(prefix + "isDeleted", item->isDeleted());
            m_setting.save(prefix + "deletedAt", item->deletedAt());
            m_setting.save(prefix + "lastModifiedAt", item->lastModifiedAt());
        }

        qDebug() << "已成功保存" << m_todos.size() << "个待办事项到本地存储";
        success = true;
    } catch (const std::exception &e) {
        qCritical() << "保存到本地存储时发生异常:" << e.what();
        success = false;
        logError("保存本地存储", QString("异常: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "保存到本地存储时发生未知异常";
        success = false;
        logError("保存本地存储", "未知异常");
    }

    return success;
}

/**
 * @brief 从服务器获取最新的待办事项
 */
void TodoModel::fetchTodosFromServer() {
    if (!m_isOnline || !isLoggedIn()) {
        qWarning() << "无法获取服务器数据：离线或未登录";
        return;
    }

    qDebug() << "从服务器获取待办事项...";

    try {
        NetworkRequest::RequestConfig config;
        config.url = getApiUrl(m_todoApiEndpoint);
        config.requiresAuth = true;

        m_networkRequest.sendRequest(NetworkRequest::RequestType::FetchTodos, config);
    } catch (const std::exception &e) {
        qCritical() << "获取服务器数据时发生异常:" << e.what();
        logError("获取服务器数据", QString("异常: %1").arg(e.what()));
        emit syncCompleted(false, QString("获取服务器数据失败: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "获取服务器数据时发生未知异常";
        logError("获取服务器数据", "未知异常");
        emit syncCompleted(false, "获取服务器数据失败：未知错误");
    }
}

/**
 * @brief 记录错误信息
 * @param context 错误发生的上下文
 * @param error 错误信息
 */
void TodoModel::logError(const QString &context, const QString &error) {
    qCritical() << context << ":" << error;

    // 可以将错误记录到日志文件中
    // TODO: 实现日志文件记录

    // 也可以在这里添加错误报告机制
}

/**
 * @brief 将本地更改推送到服务器
 */
void TodoModel::pushLocalChangesToServer() {
    // 检查网络和登录状态
    if (!m_isOnline || !isLoggedIn()) {
        qDebug() << "无法推送更改：离线或未登录";
        return;
    }

    // 找出所有未同步的项目
    QList<TodoItem *> unsyncedItems;
    for (const auto &item : std::as_const(m_todos)) {
        if (!item->synced()) {
            unsyncedItems.append(item.get());
        }
    }

    if (unsyncedItems.isEmpty()) {
        qDebug() << "没有需要同步的项目";
        return; // 没有未同步的项目
    }

    qDebug() << "推送" << unsyncedItems.size() << "个项目到服务器";

    try {
        // 创建一个包含所有未同步项目的JSON数组
        QJsonArray jsonArray;
        for (TodoItem *item : std::as_const(unsyncedItems)) {
            QJsonObject obj;
            obj["id"] = item->id();
            obj["uuid"] = item->uuid().toString(); // 将QUuid转换为QString以便JSON序列化
            obj["user_id"] = item->userId();
            obj["title"] = item->title();
            obj["description"] = item->description();
            obj["category"] = item->category();
            obj["important"] = item->important();
            obj["deadline"] = item->deadline().toString(Qt::ISODate);
            obj["recurrence_interval"] = item->recurrenceInterval();
            obj["recurrence_count"] = item->recurrenceCount();
            obj["recurrence_start_date"] = item->recurrenceStartDate().toString(Qt::ISODate);
            obj["is_completed"] = item->isCompleted();
            obj["completed_at"] = item->completedAt().toString(Qt::ISODate);
            obj["is_deleted"] = item->isDeleted();
            obj["deleted_at"] = item->deletedAt().toString(Qt::ISODate);
            obj["created_at"] = item->createdAt().toString(Qt::ISODate);
            obj["updated_at"] = item->updatedAt().toString(Qt::ISODate);
            obj["last_modified_at"] = item->lastModifiedAt().toString(Qt::ISODate);
            // obj["status"] = item->status();

            jsonArray.append(obj);
        }

        NetworkRequest::RequestConfig config;
        config.url = getApiUrl(m_todoApiEndpoint);
        config.requiresAuth = true;
        config.data["todos"] = jsonArray;

        // 存储未同步项目的引用，用于成功后标记为已同步
        m_pendingUnsyncedItems = unsyncedItems;

        m_networkRequest.sendRequest(NetworkRequest::RequestType::PushTodos, config);
    } catch (const std::exception &e) {
        qCritical() << "推送更改时发生异常:" << e.what();
        logError("推送更改", QString("异常: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "推送更改时发生未知异常";
        logError("推送更改", "未知异常");
    }
}

/**
 * @brief 初始化服务器配置
 */
void TodoModel::initializeServerConfig() {
    // 从设置中读取服务器配置，如果不存在则使用默认值
    m_serverBaseUrl = m_setting.get(QStringLiteral("server/baseUrl"), "https://api.example.com").toString();
    m_todoApiEndpoint = m_setting.get(QStringLiteral("server/todoApiEndpoint"), "/todo/todo_api.php").toString();
    m_authApiEndpoint = m_setting.get(QStringLiteral("server/authApiEndpoint"), "/auth_api.php").toString();

    qDebug() << "服务器配置已初始化:";
    qDebug() << "  基础URL:" << m_serverBaseUrl;
    qDebug() << "  待办事项API:" << m_todoApiEndpoint;
    qDebug() << "  认证API:" << m_authApiEndpoint;
}

/**
 * @brief 获取完整的API URL
 * @param endpoint API端点路径
 * @return 完整的API URL
 */
QString TodoModel::getApiUrl(const QString &endpoint) const {
    return m_serverBaseUrl + endpoint;
}

/**
 * @brief 检查URL是否使用HTTPS协议
 * @param url 要检查的URL
 * @return 如果使用HTTPS则返回true，否则返回false
 */
bool TodoModel::isHttpsUrl(const QString &url) const {
    return url.startsWith("https://", Qt::CaseInsensitive);
}

/**
 * @brief 更新服务器配置
 * @param baseUrl 新的服务器基础URL
 */
void TodoModel::updateServerConfig(const QString &baseUrl) {
    if (baseUrl.isEmpty()) {
        qWarning() << "尝试设置空的服务器URL";
        return;
    }

    // 更新内存中的配置
    m_serverBaseUrl = baseUrl;

    // 保存到设置中
    m_setting.save(QStringLiteral("server/baseUrl"), baseUrl);

    qDebug() << "服务器配置已更新:" << baseUrl;
    qDebug() << "HTTPS状态:" << (isHttpsUrl(baseUrl) ? "安全" : "不安全");
}

bool TodoModel::exportTodos(const QString &filePath) {
    QJsonArray todosArray;

    // 将所有待办事项转换为JSON格式
    for (const auto &todo : m_todos) {
        QJsonObject todoObj;
        todoObj["id"] = todo->id();
        todoObj["title"] = todo->title();
        todoObj["description"] = todo->description();
        todoObj["category"] = todo->category();
        todoObj["important"] = todo->important();
        // Note: status method does not exist in TodoItem class
        todoObj["createdAt"] = todo->createdAt().toString(Qt::ISODate);
        todoObj["updatedAt"] = todo->updatedAt().toString(Qt::ISODate);
        todoObj["synced"] = todo->synced();
        todoObj["deadline"] = todo->deadline().toString(Qt::ISODate);

        todosArray.append(todoObj);
    }

    // 创建根JSON对象
    QJsonObject rootObj;
    rootObj["version"] = "1.0";
    rootObj["exportDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    rootObj["todos"] = todosArray;

    // 写入文件
    QJsonDocument doc(rootObj);
    QFile file(filePath);

    // 确保目录存在
    QFileInfo fileInfo(filePath);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "无法打开文件进行写入:" << filePath;
        return false;
    }

    file.write(doc.toJson());
    file.close();

    qDebug() << "成功导出" << m_todos.size() << "个待办事项到" << filePath;
    return true;
}

// 类别管理相关方法实现
QStringList TodoModel::getCategories() const {
    return m_categories;
}

// 排序相关实现
int TodoModel::sortType() const {
    return m_sortType;
}

void TodoModel::setSortType(int type) {
    if (m_sortType != type) {
        m_sortType = type;
        sortTodos();
        emit sortTypeChanged();
    }
}

void TodoModel::sortTodos() {
    if (m_todos.empty()) {
        return;
    }

    beginResetModel();

    switch (m_sortType) {
    case SortByDeadline:
        std::sort(m_todos.begin(), m_todos.end(),
                  [](const std::unique_ptr<TodoItem> &a, const std::unique_ptr<TodoItem> &b) {
                      // 获取截止日期
                      QDateTime deadlineA = a->deadline();
                      QDateTime deadlineB = b->deadline();

                      // 如果A有截止日期，B没有，A排在前面
                      if (deadlineA.isValid() && !deadlineB.isValid()) {
                          return true;
                      }
                      // 如果B有截止日期，A没有，B排在前面
                      if (!deadlineA.isValid() && deadlineB.isValid()) {
                          return false;
                      }
                      // 如果都没有截止日期，按创建时间排序
                      if (!deadlineA.isValid() && !deadlineB.isValid()) {
                          return a->createdAt() > b->createdAt();
                      }
                      // 如果都有截止日期，按截止日期排序（早的在前）
                      return deadlineA < deadlineB;
                  });
        break;
    case SortByImportance:
        std::sort(m_todos.begin(), m_todos.end(),
                  [](const std::unique_ptr<TodoItem> &a, const std::unique_ptr<TodoItem> &b) {
                      // 重要的任务排在前面
                      if (a->important() != b->important()) {
                          return a->important() > b->important();
                      }
                      // 如果重要程度相同，按创建时间排序
                      return a->createdAt() > b->createdAt();
                  });
        break;
    case SortByTitle:
        std::sort(m_todos.begin(), m_todos.end(),
                  [](const std::unique_ptr<TodoItem> &a, const std::unique_ptr<TodoItem> &b) {
                      return a->title().compare(b->title(), Qt::CaseInsensitive) < 0;
                  });
        break;
    case SortByCreatedTime:
    default:
        std::sort(m_todos.begin(), m_todos.end(),
                  [](const std::unique_ptr<TodoItem> &a, const std::unique_ptr<TodoItem> &b) {
                      return a->createdAt() > b->createdAt();
                  });
        break;
    }

    // 排序后需要更新过滤缓存
    invalidateFilterCache();

    endResetModel();

    // 保存到本地存储
    saveToLocalStorage();
}

void TodoModel::fetchCategories() {
    if (!isLoggedIn()) {
        qWarning() << "用户未登录，无法获取类别列表";
        emit categoryOperationCompleted(false, "用户未登录");
        return;
    }

    QJsonObject requestData;
    requestData["action"] = "list";

    NetworkRequest::RequestConfig config;
    config.url = getApiUrl("categories_api.php");
    config.data = requestData;
    config.requiresAuth = true;
    config.headers["Authorization"] = "Bearer " + m_accessToken;

    m_networkRequest.sendRequest(NetworkRequest::FetchCategories, config);
}

void TodoModel::createCategory(const QString &name) {
    if (!isLoggedIn()) {
        qWarning() << "用户未登录，无法创建类别";
        emit categoryOperationCompleted(false, "用户未登录");
        return;
    }

    if (name.trimmed().isEmpty()) {
        emit categoryOperationCompleted(false, "类别名称不能为空");
        return;
    }

    QJsonObject requestData;
    requestData["action"] = "create";
    requestData["name"] = name;

    NetworkRequest::RequestConfig config;
    config.url = getApiUrl("categories_api.php");
    config.data = requestData;
    config.requiresAuth = true;
    config.headers["Authorization"] = "Bearer " + m_accessToken;

    m_networkRequest.sendRequest(NetworkRequest::CreateCategory, config);
}

void TodoModel::updateCategory(int id, const QString &name) {
    if (!isLoggedIn()) {
        qWarning() << "用户未登录，无法更新类别";
        emit categoryOperationCompleted(false, "用户未登录");
        return;
    }

    if (name.trimmed().isEmpty()) {
        emit categoryOperationCompleted(false, "类别名称不能为空");
        return;
    }

    QJsonObject requestData;
    requestData["action"] = "update";
    requestData["id"] = id;
    requestData["name"] = name;

    NetworkRequest::RequestConfig config;
    config.url = getApiUrl("categories_api.php");
    config.data = requestData;
    config.requiresAuth = true;
    config.headers["Authorization"] = "Bearer " + m_accessToken;

    m_networkRequest.sendRequest(NetworkRequest::UpdateCategory, config);
}

void TodoModel::deleteCategory(int id) {
    if (!isLoggedIn()) {
        qWarning() << "用户未登录，无法删除类别";
        emit categoryOperationCompleted(false, "用户未登录");
        return;
    }

    QJsonObject requestData;
    requestData["action"] = "delete";
    requestData["id"] = id;

    NetworkRequest::RequestConfig config;
    config.url = getApiUrl("categories_api.php");
    config.data = requestData;
    config.requiresAuth = true;
    config.headers["Authorization"] = "Bearer " + m_accessToken;

    m_networkRequest.sendRequest(NetworkRequest::DeleteCategory, config);
}

void TodoModel::handleFetchCategoriesSuccess(const QJsonObject &response) {
    if (response["success"].toBool()) {
        QJsonArray categoriesArray = response["categories"].toArray();
        QStringList newCategories;

        // 添加默认的"全部"选项
        newCategories << "全部";

        // 添加从服务器获取的类别
        for (const QJsonValue &value : categoriesArray) {
            QJsonObject categoryObj = value.toObject();
            QString categoryName = categoryObj["name"].toString();
            if (!categoryName.isEmpty()) {
                newCategories << categoryName;
            }
        }

        // 添加默认的"未分类"选项
        if (!newCategories.contains("未分类")) {
            newCategories << "未分类";
        }

        m_categories = newCategories;
        emit categoriesChanged();

        qDebug() << "成功获取类别列表:" << m_categories;
    } else {
        QString errorMessage = response["message"].toString();
        qWarning() << "获取类别列表失败:" << errorMessage;
        emit categoryOperationCompleted(false, errorMessage);
    }
}

void TodoModel::handleCategoryOperationSuccess(const QJsonObject &response) {
    bool success = response["success"].toBool();
    QString message = response["message"].toString();

    if (success) {
        // 操作成功后重新获取类别列表
        fetchCategories();
    }

    emit categoryOperationCompleted(success, message);
}

QVariantList TodoModel::importTodosWithAutoResolution(const QString &filePath) {
    QVariantList conflicts;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件进行读取:" << filePath;
        return conflicts;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON解析错误:" << error.errorString();
        return conflicts;
    }

    QJsonObject rootObj = doc.object();

    // 检查版本兼容性
    QString version = rootObj["version"].toString();
    if (version != "1.0") {
        qWarning() << "不支持的文件版本:" << version;
        return conflicts;
    }

    QJsonArray todosArray = rootObj["todos"].toArray();

    // 分离冲突和非冲突项目
    QJsonArray nonConflictTodos;

    qDebug() << "开始检查导入冲突，现有项目数量:" << m_todos.size() << "，导入项目数量:" << todosArray.size();

    // 打印现有项目的ID
    for (size_t i = 0; i < m_todos.size(); ++i) {
        qDebug() << "现有项目" << i << "ID:" << m_todos[i]->id() << "标题:" << m_todos[i]->title();
    }

    for (const QJsonValue &value : todosArray) {
        QJsonObject todoObj = value.toObject();
        QString id = todoObj["id"].toString();

        bool hasConflict = false;
        bool shouldSkip = false;
        TodoItem *existingTodo = nullptr;

        // 查找是否存在相同ID的待办事项
        for (const auto &todo : m_todos) {
            if (todo->id() == id.toInt()) {
                // 检查内容是否真的不同
                QString importTitle = todoObj["title"].toString();
                QString importDescription = todoObj["description"].toString();
                QString importCategory = todoObj["category"].toString();
                QString importStatus = todoObj["status"].toString();

                if (todo->title() != importTitle || todo->description() != importDescription ||
                    todo->category() != importCategory) {
                    hasConflict = true;
                    existingTodo = todo.get();
                    qDebug() << "发现真正冲突项目 ID:" << id << "现有标题:" << todo->title()
                             << "导入标题:" << importTitle;
                } else {
                    qDebug() << "ID相同且内容一致，直接跳过 ID:" << id << "标题:" << importTitle;
                    // 内容完全一致的项目直接跳过，既不导入也不显示冲突
                    shouldSkip = true;
                }
                break;
            }
        }

        if (shouldSkip) {
            // 跳过内容完全一致的项目
            continue;
        } else if (hasConflict) {
            // 发现冲突，添加到冲突列表
            QVariantMap conflictInfo;
            conflictInfo["id"] = id;
            conflictInfo["existingTitle"] = existingTodo->title();
            conflictInfo["existingDescription"] = existingTodo->description();
            conflictInfo["existingCategory"] = existingTodo->category();
            // conflictInfo["existingStatus"] = existingTodo->status();
            conflictInfo["existingUpdatedAt"] = existingTodo->updatedAt();

            conflictInfo["importTitle"] = todoObj["title"].toString();
            conflictInfo["importDescription"] = todoObj["description"].toString();
            conflictInfo["importCategory"] = todoObj["category"].toString();
            conflictInfo["importStatus"] = todoObj["status"].toString();
            conflictInfo["importUpdatedAt"] = QDateTime::fromString(todoObj["updatedAt"].toString(), Qt::ISODate);

            conflicts.append(conflictInfo);
        } else {
            // 无冲突，添加到非冲突列表
            qDebug() << "无冲突项目 ID:" << id << "标题:" << todoObj["title"].toString();
            nonConflictTodos.append(value);
        }
    }

    qDebug() << "冲突检查完成，冲突项目数量:" << conflicts.size() << "，无冲突项目数量:" << nonConflictTodos.size();

    // 导入无冲突的项目
    if (nonConflictTodos.size() > 0) {
        beginInsertRows(QModelIndex(), m_todos.size(), m_todos.size() + nonConflictTodos.size() - 1);

        for (const QJsonValue &value : nonConflictTodos) {
            QJsonObject todoObj = value.toObject();

            auto newTodo = std::make_unique<TodoItem>(
                todoObj["id"].toInt(),                                                       // id
                QUuid::fromString(todoObj["uuid"].toString()),                               // uuid
                todoObj["userId"].toInt(0),                                                  // userId
                todoObj["title"].toString(),                                                 // title
                todoObj["description"].toString(),                                           // description
                todoObj["category"].toString(),                                              // category
                todoObj["important"].toBool(),                                               // important
                QDateTime::fromString(todoObj["deadline"].toString(), Qt::ISODate),          // deadline
                todoObj["recurrence_interval"].toInt(0),                                     // recurrenceInterval
                todoObj["recurrence_count"].toInt(-1),                                       // recurrenceCount
                QDate::fromString(todoObj["recurrence_start_date"].toString(), Qt::ISODate), // recurrenceStartDate
                todoObj["isCompleted"].toBool(false),                                        // isCompleted
                QDateTime::fromString(todoObj["completedAt"].toString(), Qt::ISODate),       // completedAt
                todoObj["isDeleted"].toBool(false),                                          // isDeleted
                QDateTime::fromString(todoObj["deletedAt"].toString(), Qt::ISODate),         // deletedAt
                QDateTime::fromString(todoObj["createdAt"].toString(), Qt::ISODate),         // createdAt
                QDateTime::fromString(todoObj["updatedAt"].toString(), Qt::ISODate),         // updatedAt
                QDateTime::fromString(todoObj["lastModifiedAt"].toString(), Qt::ISODate),    // lastModifiedAt
                false,                                                                       // synced
                this);

            m_todos.push_back(std::move(newTodo));
        }

        endInsertRows();
    }

    // 保存到本地存储
    if (nonConflictTodos.size() > 0) {
        saveToLocalStorage();
    }

    return conflicts;
}

bool TodoModel::importTodos(const QString &filePath) {
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件进行读取:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON解析错误:" << error.errorString();
        return false;
    }

    QJsonObject rootObj = doc.object();

    // 检查版本兼容性
    QString version = rootObj["version"].toString();
    if (version != "1.0") {
        qWarning() << "不支持的文件版本:" << version;
        return false;
    }

    QJsonArray todosArray = rootObj["todos"].toArray();
    int importedCount = 0;
    int skippedCount = 0;

    beginResetModel();

    for (const QJsonValue &value : todosArray) {
        QJsonObject todoObj = value.toObject();

        QString id = todoObj["id"].toString();

        // 检查是否已存在相同ID的待办事项
        bool exists = false;
        for (const auto &existingTodo : m_todos) {
            if (existingTodo->id() == id.toInt()) {
                exists = true;
                skippedCount++;
                break;
            }
        }

        if (!exists) {
            // 创建新的待办事项
            auto newTodo = std::make_unique<TodoItem>(
                id.toInt(),                                                                  // id
                QUuid::fromString(todoObj["uuid"].toString()),                               // uuid
                todoObj["userId"].toInt(0),                                                  // userId
                todoObj["title"].toString(),                                                 // title
                todoObj["description"].toString(),                                           // description
                todoObj["category"].toString(),                                              // category
                todoObj["important"].toBool(),                                               // important
                QDateTime::fromString(todoObj["deadline"].toString(), Qt::ISODate),          // deadline
                todoObj["recurrence_interval"].toInt(0),                                     // recurrenceInterval
                todoObj["recurrence_count"].toInt(-1),                                       // recurrenceCount
                QDate::fromString(todoObj["recurrence_start_date"].toString(), Qt::ISODate), // recurrenceStartDate
                todoObj["isCompleted"].toBool(false),                                        // isCompleted
                QDateTime::fromString(todoObj["completedAt"].toString(), Qt::ISODate),       // completedAt
                false,                                                                       // isDeleted
                QDateTime(),                                                                 // deletedAt
                QDateTime::fromString(todoObj["createdAt"].toString(), Qt::ISODate),         // createdAt
                QDateTime::fromString(todoObj["updatedAt"].toString(), Qt::ISODate),         // updatedAt
                QDateTime::fromString(todoObj["updatedAt"].toString(), Qt::ISODate),         // lastModifiedAt
                todoObj["synced"].toBool(),                                                  // synced
                nullptr                                                                      // parent
            );

            // 设置deadline字段（如果存在）
            if (todoObj.contains("deadline")) {
                newTodo->setDeadline(QDateTime::fromString(todoObj["deadline"].toString(), Qt::ISODate));
            }

            m_todos.push_back(std::move(newTodo));
            importedCount++;
        }
    }

    endResetModel();

    // 保存到本地存储
    saveToLocalStorage();

    qDebug() << "导入完成 - 新增:" << importedCount << "个，跳过:" << skippedCount << "个";
    return true;
}

QVariantList TodoModel::checkImportConflicts(const QString &filePath) {
    QVariantList conflicts;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件进行读取:" << filePath;
        return conflicts;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON解析错误:" << error.errorString();
        return conflicts;
    }

    QJsonObject rootObj = doc.object();

    // 检查版本兼容性
    QString version = rootObj["version"].toString();
    if (version != "1.0") {
        qWarning() << "不支持的文件版本:" << version;
        return conflicts;
    }

    QJsonArray todosArray = rootObj["todos"].toArray();

    for (const QJsonValue &value : todosArray) {
        QJsonObject todoObj = value.toObject();
        QString id = todoObj["id"].toString();

        // 查找是否存在相同ID的待办事项
        for (const auto &existingTodo : m_todos) {
            if (existingTodo->id() == id.toInt()) {
                // 发现冲突，创建冲突信息
                QVariantMap conflictInfo;
                conflictInfo["id"] = id;
                conflictInfo["existingTitle"] = existingTodo->title();
                conflictInfo["existingDescription"] = existingTodo->description();
                conflictInfo["existingCategory"] = existingTodo->category();
                // conflictInfo["existingStatus"] = existingTodo->status();
                conflictInfo["existingUpdatedAt"] = existingTodo->updatedAt();

                conflictInfo["importTitle"] = todoObj["title"].toString();
                conflictInfo["importDescription"] = todoObj["description"].toString();
                conflictInfo["importCategory"] = todoObj["category"].toString();
                conflictInfo["importStatus"] = todoObj["status"].toString();
                conflictInfo["importUpdatedAt"] = QDateTime::fromString(todoObj["updatedAt"].toString(), Qt::ISODate);

                conflicts.append(conflictInfo);
                break;
            }
        }
    }

    return conflicts;
}

bool TodoModel::importTodosWithConflictResolution(const QString &filePath, const QString &conflictResolution) {
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件进行读取:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON解析错误:" << error.errorString();
        return false;
    }

    QJsonObject rootObj = doc.object();

    // 检查版本兼容性
    QString version = rootObj["version"].toString();
    if (version != "1.0") {
        qWarning() << "不支持的文件版本:" << version;
        return false;
    }

    QJsonArray todosArray = rootObj["todos"].toArray();
    int importedCount = 0;
    int skippedCount = 0;
    int overwrittenCount = 0;

    beginResetModel();

    for (const QJsonValue &value : todosArray) {
        QJsonObject todoObj = value.toObject();
        QString id = todoObj["id"].toString();

        // 查找是否已存在相同ID的待办事项
        bool exists = false;
        int existingIndex = -1;
        for (size_t i = 0; i < m_todos.size(); ++i) {
            if (m_todos[i]->id() == id.toInt()) {
                exists = true;
                existingIndex = static_cast<int>(i);
                break;
            }
        }

        if (exists) {
            if (conflictResolution == "overwrite") {
                // 覆盖现有项目
                TodoItem *existingItem = m_todos[existingIndex].get();
                existingItem->setTitle(todoObj["title"].toString());
                existingItem->setDescription(todoObj["description"].toString());
                existingItem->setCategory(todoObj["category"].toString());
                existingItem->setImportant(todoObj["important"].toBool());
                // Note: setStatus method does not exist in TodoItem class
                existingItem->setUpdatedAt(QDateTime::fromString(todoObj["updatedAt"].toString(), Qt::ISODate));
                existingItem->setSynced(todoObj["synced"].toBool());
                overwrittenCount++;
            } else if (conflictResolution == "merge") {
                // 合并：保留较新的更新时间的版本
                TodoItem *existingItem = m_todos[existingIndex].get();
                QDateTime importUpdatedAt = QDateTime::fromString(todoObj["updatedAt"].toString(), Qt::ISODate);

                if (importUpdatedAt > existingItem->updatedAt()) {
                    // 导入的版本更新，使用导入的数据
                    existingItem->setTitle(todoObj["title"].toString());
                    existingItem->setDescription(todoObj["description"].toString());
                    existingItem->setCategory(todoObj["category"].toString());
                    existingItem->setImportant(todoObj["important"].toBool());
                    existingItem->setUpdatedAt(importUpdatedAt);
                    existingItem->setSynced(todoObj["synced"].toBool());
                    overwrittenCount++;
                }
                // 如果现有版本更新或相同，则保持不变
            } else if (conflictResolution == "skip") {
                // 跳过冲突项目
                skippedCount++;
            }
        } else {
            // 创建新的待办事项
            auto newTodo = std::make_unique<TodoItem>(
                id.toInt(),                                                                  // id
                QUuid::fromString(todoObj["uuid"].toString()),                               // uuid
                todoObj["userId"].toInt(0),                                                  // userId
                todoObj["title"].toString(),                                                 // title
                todoObj["description"].toString(),                                           // description
                todoObj["category"].toString(),                                              // category
                todoObj["important"].toBool(),                                               // important
                QDateTime::fromString(todoObj["deadline"].toString(), Qt::ISODate),          // deadline
                todoObj["recurrence_interval"].toInt(0),                                     // recurrenceInterval
                todoObj["recurrence_count"].toInt(-1),                                       // recurrenceCount
                QDate::fromString(todoObj["recurrence_start_date"].toString(), Qt::ISODate), // recurrenceStartDate
                todoObj["isCompleted"].toBool(false),                                        // isCompleted
                QDateTime::fromString(todoObj["completedAt"].toString(), Qt::ISODate),       // completedAt
                todoObj["isDeleted"].toBool(false),                                          // isDeleted
                QDateTime::fromString(todoObj["deletedAt"].toString(), Qt::ISODate),         // deletedAt
                QDateTime::fromString(todoObj["createdAt"].toString(), Qt::ISODate),         // createdAt
                QDateTime::fromString(todoObj["updatedAt"].toString(), Qt::ISODate),         // updatedAt
                QDateTime::fromString(todoObj["lastModifiedAt"].toString(), Qt::ISODate),    // lastModifiedAt
                todoObj["synced"].toBool(),                                                  // synced
                this                                                                         // parent
            );

            // 设置deadline字段（如果存在）
            if (todoObj.contains("deadline")) {
                newTodo->setDeadline(QDateTime::fromString(todoObj["deadline"].toString(), Qt::ISODate));
            }

            m_todos.push_back(std::move(newTodo));
            importedCount++;
        }
    }

    endResetModel();

    // 保存到本地存储
    saveToLocalStorage();

    qDebug() << "导入完成 - 新增:" << importedCount << "个，覆盖:" << overwrittenCount << "个，跳过:" << skippedCount
             << "个";
    return true;
}

bool TodoModel::importTodosWithIndividualResolution(const QString &filePath, const QVariantMap &resolutions) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件进行读取:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON解析错误:" << error.errorString();
        return false;
    }

    if (!doc.isArray()) {
        qWarning() << "JSON文档不是数组格式";
        return false;
    }

    QJsonArray jsonArray = doc.array();
    int importedCount = 0;
    int updatedCount = 0;
    int skippedCount = 0;

    for (const QJsonValue &value : jsonArray) {
        if (!value.isObject())
            continue;

        QJsonObject obj = value.toObject();
        QString id = obj["id"].toString();

        // 查找是否存在相同ID的项目
        TodoItem *existingItem = nullptr;
        for (const auto &item : m_todos) {
            if (item->id() == id.toInt()) {
                existingItem = item.get();
                break;
            }
        }

        if (existingItem) {
            // 获取该项目的处理方式
            QString resolution = resolutions.value(id, "skip").toString();

            if (resolution == "overwrite") {
                // 覆盖现有数据
                existingItem->setTitle(obj["title"].toString());
                existingItem->setDescription(obj["description"].toString());
                existingItem->setCategory(obj["category"].toString());
                // Note: setStatus method does not exist in TodoItem class
                existingItem->setCreatedAt(QDateTime::fromString(obj["createdAt"].toString(), Qt::ISODate));
                existingItem->setUpdatedAt(QDateTime::fromString(obj["updatedAt"].toString(), Qt::ISODate));
                existingItem->setSynced(false); // 标记为未同步
                updatedCount++;
            } else if (resolution == "merge") {
                // 智能合并：保留更新时间较新的版本
                QDateTime existingUpdated = existingItem->updatedAt();
                QDateTime importUpdated = QDateTime::fromString(obj["updatedAt"].toString(), Qt::ISODate);

                if (importUpdated > existingUpdated) {
                    existingItem->setTitle(obj["title"].toString());
                    existingItem->setDescription(obj["description"].toString());
                    existingItem->setCategory(obj["category"].toString());
                    // Note: setStatus method does not exist in TodoItem class
                    existingItem->setCreatedAt(QDateTime::fromString(obj["createdAt"].toString(), Qt::ISODate));
                    existingItem->setUpdatedAt(importUpdated);
                    existingItem->setSynced(false); // 标记为未同步
                    updatedCount++;
                } else {
                    skippedCount++; // 现有数据更新，跳过导入
                }
            } else {
                // skip - 跳过冲突项目
                skippedCount++;
            }
        } else {
            // 创建新项目（没有冲突的项目直接导入）
            auto newItem = std::make_unique<TodoItem>(this);
            newItem->setId(id.toInt());
            newItem->setTitle(obj["title"].toString());
            newItem->setDescription(obj["description"].toString());
            newItem->setCategory(obj["category"].toString());
            // Note: setStatus method does not exist in TodoItem class
            newItem->setCreatedAt(QDateTime::fromString(obj["createdAt"].toString(), Qt::ISODate));
            newItem->setUpdatedAt(QDateTime::fromString(obj["updatedAt"].toString(), Qt::ISODate));
            newItem->setSynced(false); // 标记为未同步

            beginInsertRows(QModelIndex(), m_todos.size(), m_todos.size());
            m_todos.push_back(std::move(newItem));
            endInsertRows();
            importedCount++;
        }
    }

    // 保存到本地存储
    saveToLocalStorage();

    qDebug() << "个别冲突处理导入完成 - 新增:" << importedCount << "个，更新:" << updatedCount
             << "个，跳过:" << skippedCount << "个";
    return true;
}
