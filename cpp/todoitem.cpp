/**
 * @file todoitem.cpp
 * @brief TodoItem类的实现文件
 *
 * 该文件实现了TodoItem类，用于表示待办事项的数据模型。
 * TodoItem包含了待办事项的所有属性，如标题、描述、分类、优先级等，
 * 并提供了相应的getter和setter方法，支持Qt的属性系统和信号槽机制。
 *
 * @author MyTodo Team
 * @date 2024
 */

#include "todoItem.h"

/**
 * @brief 默认构造函数
 *
 * 创建一个空的TodoItem对象，所有字符串属性为空，
 * 时间属性为无效时间，同步状态为false。
 *
 * @param parent 父对象指针，用于Qt的对象树管理
 */
TodoItem::TodoItem(QObject *parent) : QObject(parent), m_synced(false) {
}

/**
 * @brief 带参数的构造函数
 *
 * 使用指定的参数创建TodoItem对象。这个构造函数通常用于
 * 从数据库或网络加载已存在的待办事项数据。
 *
 * @param id 待办事项的唯一标识符
 * @param title 待办事项标题
 * @param description 待办事项详细描述
 * @param category 待办事项分类
 * @param important 重要程度
 * @param status 当前状态（如：待处理、进行中、已完成）
 * @param createdAt 创建时间
 * @param updatedAt 最后更新时间
 * @param synced 是否已与服务器同步
 * @param parent 父对象指针
 */
TodoItem::TodoItem(const QString &id, const QString &title, const QString &description, //
                   const QString &category, bool important,                            //
                   const QString &status, const QString &deadline,                      //
                   int recurrenceInterval, int recurrenceCount,                        //
                   const QString &recurrenceStartDate,                                 //
                   const QDateTime &createdAt, const QDateTime &updatedAt,              //
                   bool synced, const QString &uuid, int userId,                       //
                   bool isCompleted, const QDateTime &completedAt,                     //
                   bool isDeleted, const QDateTime &deletedAt,                         //
                   const QDateTime &lastModifiedAt, QObject *parent)                   //
    : QObject(parent),                                                                  // 初始化父对象
      m_id(id),                                                                         // 初始化待办事项ID
      m_title(title),                                                                   // 初始化待办事项标题
      m_description(description),                                                       // 初始化待办事项描述
      m_category(category),                                                             // 初始化待办事项分类
      m_important(important),                                                         // 初始化待办事项重要程度
      m_status(status),                                                                 // 初始化待办事项状态
      m_deadline(deadline),                                                             // 初始化待办事项截止日期
      m_recurrenceInterval(recurrenceInterval),                                        // 初始化循环间隔
      m_recurrenceCount(recurrenceCount),                                              // 初始化循环次数
      m_recurrenceStartDate(recurrenceStartDate),                                      // 初始化循环开始日期
      m_createdAt(createdAt),                                                           // 初始化待办事项创建时间
      m_updatedAt(updatedAt),                                                           // 初始化待办事项更新时间
      m_synced(synced),                                                                 // 初始化待办事项同步状态
      m_uuid(uuid),                                                                     // 初始化待办事项UUID
      m_userId(userId),                                                                 // 初始化用户ID
      m_isCompleted(isCompleted),                                                       // 初始化完成状态
      m_completedAt(completedAt),                                                       // 初始化完成时间
      m_isDeleted(isDeleted),                                                           // 初始化删除状态
      m_deletedAt(deletedAt),                                                           // 初始化删除时间
      m_lastModifiedAt(lastModifiedAt) {
} // 初始化最后修改时间

/**
 * @brief 获取待办事项的唯一标识符
 * @return 待办事项ID
 */
QString TodoItem::id() const {
    return m_id;
}

/**
 * @brief 设置待办事项的唯一标识符
 *
 * 如果新ID与当前ID相同，则不执行任何操作。
 * 否则更新ID并发出idChanged信号。
 *
 * @param id 新的待办事项ID
 */
void TodoItem::setId(const QString &id) {
    if (m_id == id)
        return;

    m_id = id;
    emit idChanged();
}

/**
 * @brief 获取待办事项标题
 * @return 待办事项标题
 */
QString TodoItem::title() const {
    return m_title;
}

/**
 * @brief 设置待办事项标题
 *
 * 如果新标题与当前标题相同，则不执行任何操作。
 * 否则更新标题并发出titleChanged信号。
 *
 * @param title 新的待办事项标题
 */
void TodoItem::setTitle(const QString &title) {
    if (m_title == title)
        return;

    m_title = title;
    emit titleChanged();
}

/**
 * @brief 获取待办事项描述
 * @return 待办事项详细描述
 */
QString TodoItem::description() const {
    return m_description;
}

/**
 * @brief 设置待办事项描述
 *
 * 如果新描述与当前描述相同，则不执行任何操作。
 * 否则更新描述并发出descriptionChanged信号。
 *
 * @param description 新的待办事项描述
 */
void TodoItem::setDescription(const QString &description) {
    if (m_description == description)
        return;

    m_description = description;
    emit descriptionChanged();
}

/**
 * @brief 获取待办事项分类
 * @return 待办事项分类
 */
QString TodoItem::category() const {
    return m_category;
}

/**
 * @brief 设置待办事项分类
 *
 * 如果新分类与当前分类相同，则不执行任何操作。
 * 否则更新分类并发出categoryChanged信号。
 *
 * @param category 新的待办事项分类
 */
void TodoItem::setCategory(const QString &category) {
    if (m_category == category)
        return;

    m_category = category;
    emit categoryChanged();
}

/**
 * @brief 获取待办事项重要程度
 * @return 重要程度
 */
bool TodoItem::important() const {
    return m_important;
}

/**
 * @brief 设置待办事项重要程度
 *
 * 如果新重要程度与当前重要程度相同，则不执行任何操作。
 * 否则更新重要程度并发出importantChanged信号。
 *
 * @param important 新的重要程度
 */
void TodoItem::setImportant(bool important) {
    if (m_important == important)
        return;

    m_important = important;
    emit importantChanged();
}

/**
 * @brief 获取待办事项截止日期
 * @return 截止日期
 */
QString TodoItem::deadline() const {
    return m_deadline;
}

/**
 * @brief 设置待办事项截止日期
 *
 * 如果新截止日期与当前截止日期相同，则不执行任何操作。
 * 否则更新截止日期并发出deadlineChanged信号。
 *
 * @param deadline 新的截止日期
 */
void TodoItem::setDeadline(const QString &deadline) {
    if (m_deadline == deadline)
        return;

    m_deadline = deadline;
    emit deadlineChanged();
}

/**
 * @brief 获取待办事项状态
 * @return 当前状态（如：待处理、进行中、已完成）
 */
QString TodoItem::status() const {
    return m_status;
}

/**
 * @brief 设置待办事项状态
 *
 * 如果新状态与当前状态相同，则不执行任何操作。
 * 否则更新状态并发出statusChanged信号。
 *
 * @param status 新的待办事项状态
 */
void TodoItem::setStatus(const QString &status) {
    if (m_status == status)
        return;

    m_status = status;
    emit statusChanged();
}

/**
 * @brief 获取待办事项创建时间
 * @return 创建时间
 */
QDateTime TodoItem::createdAt() const {
    return m_createdAt;
}

/**
 * @brief 设置待办事项创建时间
 *
 * 如果新创建时间与当前创建时间相同，则不执行任何操作。
 * 否则更新创建时间并发出createdAtChanged信号。
 *
 * @param createdAt 新的创建时间
 */
void TodoItem::setCreatedAt(const QDateTime &createdAt) {
    if (m_createdAt == createdAt)
        return;

    m_createdAt = createdAt;
    emit createdAtChanged();
}

/**
 * @brief 获取待办事项最后更新时间
 * @return 最后更新时间
 */
QDateTime TodoItem::updatedAt() const {
    return m_updatedAt;
}

/**
 * @brief 设置待办事项最后更新时间
 *
 * 如果新更新时间与当前更新时间相同，则不执行任何操作。
 * 否则更新时间并发出updatedAtChanged信号。
 *
 * @param updatedAt 新的更新时间
 */
void TodoItem::setUpdatedAt(const QDateTime &updatedAt) {
    if (m_updatedAt == updatedAt)
        return;

    m_updatedAt = updatedAt;
    emit updatedAtChanged();
}

/**
 * @brief 获取待办事项同步状态
 * @return 是否已与服务器同步
 */
bool TodoItem::synced() const {
    return m_synced;
}

/**
 * @brief 设置待办事项同步状态
 *
 * 如果新同步状态与当前同步状态相同，则不执行任何操作。
 * 否则更新同步状态并发出syncedChanged信号。
 *
 * @param synced 新的同步状态
 */
void TodoItem::setSynced(bool synced) {
    if (m_synced == synced)
        return;

    m_synced = synced;
    emit syncedChanged();
}

/**
 * @brief 获取循环间隔
 *
 * @return 循环间隔（天数）
 */
int TodoItem::recurrenceInterval() const {
    return m_recurrenceInterval;
}

/**
 * @brief 设置循环间隔
 *
 * 设置待办事项的循环间隔，并发出recurrenceIntervalChanged信号。
 *
 * @param recurrenceInterval 新的循环间隔
 */
void TodoItem::setRecurrenceInterval(int recurrenceInterval) {
    if (m_recurrenceInterval != recurrenceInterval) {
        m_recurrenceInterval = recurrenceInterval;
        emit recurrenceIntervalChanged();
    }
}

/**
 * @brief 获取循环次数
 *
 * @return 循环次数
 */
int TodoItem::recurrenceCount() const {
    return m_recurrenceCount;
}

/**
 * @brief 设置循环次数
 *
 * 设置待办事项的循环次数，并发出recurrenceCountChanged信号。
 *
 * @param recurrenceCount 新的循环次数
 */
void TodoItem::setRecurrenceCount(int recurrenceCount) {
    if (m_recurrenceCount != recurrenceCount) {
        m_recurrenceCount = recurrenceCount;
        emit recurrenceCountChanged();
    }
}

/**
 * @brief 获取循环开始日期
 *
 * @return 循环开始日期
 */
QString TodoItem::recurrenceStartDate() const {
    return m_recurrenceStartDate;
}

/**
 * @brief 设置循环开始日期
 *
 * 设置待办事项的循环开始日期，并发出recurrenceStartDateChanged信号。
 *
 * @param recurrenceStartDate 新的循环开始日期
 */
void TodoItem::setRecurrenceStartDate(const QString &recurrenceStartDate) {
    if (m_recurrenceStartDate != recurrenceStartDate) {
        m_recurrenceStartDate = recurrenceStartDate;
        emit recurrenceStartDateChanged();
    }
}

/**
 * @brief 获取待办事项的UUID
 * @return 待办事项UUID
 */
QString TodoItem::uuid() const {
    return m_uuid;
}

/**
 * @brief 设置待办事项的UUID
 * @param uuid 新的UUID
 */
void TodoItem::setUuid(const QString &uuid) {
    if (m_uuid != uuid) {
        m_uuid = uuid;
        emit uuidChanged();
    }
}

/**
 * @brief 获取用户ID
 * @return 用户ID
 */
int TodoItem::userId() const {
    return m_userId;
}

/**
 * @brief 设置用户ID
 * @param userId 新的用户ID
 */
void TodoItem::setUserId(int userId) {
    if (m_userId != userId) {
        m_userId = userId;
        emit userIdChanged();
    }
}

/**
 * @brief 获取是否已完成
 * @return 是否已完成
 */
bool TodoItem::isCompleted() const {
    return m_isCompleted;
}

/**
 * @brief 设置是否已完成
 * @param completed 新的完成状态
 */
void TodoItem::setIsCompleted(bool completed) {
    if (m_isCompleted != completed) {
        m_isCompleted = completed;
        emit isCompletedChanged();
    }
}

/**
 * @brief 获取完成时间
 * @return 完成时间
 */
QDateTime TodoItem::completedAt() const {
    return m_completedAt;
}

/**
 * @brief 设置完成时间
 * @param completedAt 新的完成时间
 */
void TodoItem::setCompletedAt(const QDateTime &completedAt) {
    if (m_completedAt != completedAt) {
        m_completedAt = completedAt;
        emit completedAtChanged();
    }
}

/**
 * @brief 获取是否已删除
 * @return 是否已删除
 */
bool TodoItem::isDeleted() const {
    return m_isDeleted;
}

/**
 * @brief 设置是否已删除
 * @param deleted 新的删除状态
 */
void TodoItem::setIsDeleted(bool deleted) {
    if (m_isDeleted != deleted) {
        m_isDeleted = deleted;
        emit isDeletedChanged();
    }
}

/**
 * @brief 获取删除时间
 * @return 删除时间
 */
QDateTime TodoItem::deletedAt() const {
    return m_deletedAt;
}

/**
 * @brief 设置删除时间
 * @param deletedAt 新的删除时间
 */
void TodoItem::setDeletedAt(const QDateTime &deletedAt) {
    if (m_deletedAt != deletedAt) {
        m_deletedAt = deletedAt;
        emit deletedAtChanged();
    }
}

/**
 * @brief 获取最后修改时间
 * @return 最后修改时间
 */
QDateTime TodoItem::lastModifiedAt() const {
    return m_lastModifiedAt;
}

/**
 * @brief 设置最后修改时间
 * @param lastModifiedAt 新的最后修改时间
 */
void TodoItem::setLastModifiedAt(const QDateTime &lastModifiedAt) {
    if (m_lastModifiedAt != lastModifiedAt) {
        m_lastModifiedAt = lastModifiedAt;
        emit lastModifiedAtChanged();
    }
}
