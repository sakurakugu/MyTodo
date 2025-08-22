/**
 * @file todo_item.cpp
 * @brief TodoItem类的实现文件
 *
 * 该文件实现了TodoItem类，用于表示待办事项的数据模型。
 * TodoItem包含了待办事项的所有属性，如标题、描述、分类、优先级等，
 * 并提供了相应的getter和setter方法，支持Qt的属性系统和信号槽机制。
 *
 * @author Sakurakugu
 * @date 2025
 */

#include "todo_item.h"

/**
 * @brief 默认构造函数
 *
 * 创建一个空的TodoItem对象，所有字符串属性为空，
 * 时间属性为无效时间，同步状态为false。
 *
 * @param parent 父对象指针，用于Qt的对象树管理
 */
TodoItem::TodoItem(QObject *parent)
    : QObject(parent),                                // 初始化父对象
      m_id(0),                                        // 初始化待办事项ID为0
      m_uuid(QUuid::createUuid()),                    // 初始化UUID
      m_userId(0),                                    // 初始化用户ID为0
      m_important(false),                             // 初始化重要程度为false
      m_recurrenceInterval(0),                        // 初始化循环间隔为0
      m_recurrenceCount(0),                           // 初始化循环次数为0
      m_isCompleted(false),                           // 初始化是否已完成为false
      m_isDeleted(false),                             // 初始化是否已删除为false
      m_createdAt(QDateTime::currentDateTime()),      // 初始化创建时间为当前时间
      m_updatedAt(QDateTime::currentDateTime()),      // 初始化更新时间为当前时间
      m_lastModifiedAt(QDateTime::currentDateTime()), // 初始化最后修改时间为当前时间
      m_synced(false)                                 // 初始化是否已同步为false
{
}

/**
 * @brief 带参数的构造函数
 *
 * 使用指定的参数创建TodoItem对象。这个构造函数通常用于
 * 从数据库或网络加载已存在的待办事项数据。
 */
TodoItem::TodoItem(int id,                           ///< 待办事项唯一标识符
                   const QUuid &uuid,                ///< 唯一标识符
                   int userId,                       ///< 用户ID
                   const QString &title,             ///< 待办事项标题
                   const QString &description,       ///< 待办事项详细描述
                   const QString &category,          ///< 待办事项分类
                   bool important,                   ///< 重要程度
                   const QDateTime &deadline,        ///< 截止日期、重复结束日期
                   int recurrenceInterval,           ///< 重复间隔
                   int recurrenceCount,              ///< 重复次数
                   const QDate &recurrenceStartDate, ///< 重复开始日期
                   bool isCompleted,                 ///< 是否已完成
                   const QDateTime &completedAt,     ///< 完成时间
                   bool isDeleted,                   ///< 是否已删除
                   const QDateTime &deletedAt,       ///< 删除时间
                   const QDateTime &createdAt,       ///< 创建时间
                   const QDateTime &updatedAt,       ///< 最后更新时间
                   const QDateTime &lastModifiedAt,  ///< 最后修改时间
                   bool synced,                      ///< 是否已与服务器同步
                   QObject *parent)                  ///< 父对象指针
                                                     //
    : QObject(parent),                               ///< 初始化父对象
      m_id(id),                                      ///< 初始化待办事项ID
      m_uuid(uuid),                                  ///< 初始化待办事项UUID
      m_userId(userId),                              ///< 初始化用户ID
      m_title(title),                                ///< 初始化待办事项标题
      m_description(description),                    ///< 初始化待办事项描述
      m_category(category),                          ///< 初始化待办事项分类
      m_important(important),                        ///< 初始化待办事项重要程度
      m_deadline(deadline),                          ///< 初始化待办事项截止日期
      m_recurrenceInterval(recurrenceInterval),      ///< 初始化循环间隔
      m_recurrenceCount(recurrenceCount),            ///< 初始化循环次数
      m_recurrenceStartDate(recurrenceStartDate),    ///< 初始化循环开始日期
      m_isCompleted(isCompleted),                    ///< 初始化完成状态
      m_completedAt(completedAt),                    ///< 初始化完成时间
      m_isDeleted(isDeleted),                        ///< 初始化删除状态
      m_deletedAt(deletedAt),                        ///< 初始化删除时间
      m_createdAt(createdAt),                        ///< 初始化待办事项创建时间
      m_updatedAt(updatedAt),                        ///< 初始化待办事项更新时间
      m_lastModifiedAt(lastModifiedAt),              ///< 初始化最后修改时间
      m_synced(synced)                               // 初始化待办事项同步状态
{
}

/**
 * @brief 设置待办事项的唯一标识符
 * @param id 新的待办事项ID
 */
void TodoItem::setId(int id) {
    setProperty(m_id, id, &TodoItem::idChanged);
}

/**
 * @brief 设置待办事项的UUID
 * @param uuid 新的UUID
 */
void TodoItem::setUuid(const QUuid &uuid) {
    setProperty(m_uuid, uuid, &TodoItem::uuidChanged);
}

/**
 * @brief 设置用户ID
 * @param userId 新的用户ID
 */
void TodoItem::setUserId(int userId) {
    setProperty(m_userId, userId, &TodoItem::userIdChanged);
}

/**
 * @brief 设置待办事项标题
 * @param title 新的待办事项标题
 */
void TodoItem::setTitle(const QString &title) {
    setProperty(m_title, title, &TodoItem::titleChanged);
}

/**
 * @brief 设置待办事项描述
 * @param description 新的待办事项描述
 */
void TodoItem::setDescription(const QString &description) {
    setProperty(m_description, description, &TodoItem::descriptionChanged);
}

/**
 * @brief 设置待办事项分类
 * @param category 新的待办事项分类
 */
void TodoItem::setCategory(const QString &category) {
    setProperty(m_category, category, &TodoItem::categoryChanged);
}

/**
 * @brief 设置待办事项重要程度
 * @param important 新的重要程度
 */
void TodoItem::setImportant(bool important) {
    setProperty(m_important, important, &TodoItem::importantChanged);
}

/**
 * @brief 设置待办事项截止日期
 * @param deadline 新的截止日期
 */
void TodoItem::setDeadline(const QDateTime &deadline) {
    setProperty(m_deadline, deadline, &TodoItem::deadlineChanged);
}

/**
 * @brief 设置循环间隔
 * @param recurrenceInterval 新的循环间隔
 */
void TodoItem::setRecurrenceInterval(int recurrenceInterval) {
    setProperty(m_recurrenceInterval, recurrenceInterval, &TodoItem::recurrenceIntervalChanged);
}

/**
 * @brief 设置循环次数
 * @param recurrenceCount 新的循环次数
 */
void TodoItem::setRecurrenceCount(int recurrenceCount) {
    setProperty(m_recurrenceCount, recurrenceCount, &TodoItem::recurrenceCountChanged);
}

/**
 * @brief 设置循环开始日期
 * @param recurrenceStartDate 新的循环开始日期
 */
void TodoItem::setRecurrenceStartDate(const QDate &recurrenceStartDate) {
    setProperty(m_recurrenceStartDate, recurrenceStartDate, &TodoItem::recurrenceStartDateChanged);
}

/**
 * @brief 设置是否已完成
 * @param completed 新的完成状态
 */
void TodoItem::setIsCompleted(bool completed) {
    setProperty(m_isCompleted, completed, &TodoItem::isCompletedChanged);
}

/**
 * @brief 设置完成时间
 * @param completedAt 新的完成时间
 */
void TodoItem::setCompletedAt(const QDateTime &completedAt) {
    setProperty(m_completedAt, completedAt, &TodoItem::completedAtChanged);
}

/**
 * @brief 设置是否已删除
 * @param deleted 新的删除状态
 */
void TodoItem::setIsDeleted(bool deleted) {
    setProperty(m_isDeleted, deleted, &TodoItem::isDeletedChanged);
}

/**
 * @brief 设置删除时间
 * @param deletedAt 新的删除时间
 */
void TodoItem::setDeletedAt(const QDateTime &deletedAt) {
    setProperty(m_deletedAt, deletedAt, &TodoItem::deletedAtChanged);
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
 * @param createdAt 新的创建时间
 */
void TodoItem::setCreatedAt(const QDateTime &createdAt) {
    setProperty(m_createdAt, createdAt, &TodoItem::createdAtChanged);
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
 * @param updatedAt 新的更新时间
 */
void TodoItem::setUpdatedAt(const QDateTime &updatedAt) {
    setProperty(m_updatedAt, updatedAt, &TodoItem::updatedAtChanged);
}

/**
 * @brief 设置最后修改时间
 * @param lastModifiedAt 新的最后修改时间
 */
void TodoItem::setLastModifiedAt(const QDateTime &lastModifiedAt) {
    setProperty(m_lastModifiedAt, lastModifiedAt, &TodoItem::lastModifiedAtChanged);
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
 * @param synced 新的同步状态
 */
void TodoItem::setSynced(bool synced) {
    setProperty(m_synced, synced, &TodoItem::syncedChanged);
}

// 便利方法实现

/**
 * @brief 检查待办事项是否已过期
 * @return 如果已过期返回true，否则返回false
 */
bool TodoItem::isOverdue() const noexcept {
    return m_deadline.isValid() && m_deadline < QDateTime::currentDateTime() && !m_isCompleted;
}

/**
 * @brief 检查待办事项是否为循环任务
 * @return 如果是循环任务返回true，否则返回false
 */
constexpr bool TodoItem::isRecurring() const noexcept {
    return m_recurrenceInterval > 0;
}

/**
 * @brief 检查待办事项是否即将到期（24小时内）
 * @return 如果即将到期返回true，否则返回false
 */
bool TodoItem::isDue(const QDateTime &checkTime) const noexcept {
    if (!m_deadline.isValid() || m_isCompleted) {
        return false;
    }
    return m_deadline <= checkTime.addDays(1);
}

/**
 * @brief 计算距离截止日期的天数
 * @param checkTime 检查时间
 * @return 距离截止日期的天数，负数表示已过期
 */
int TodoItem::daysUntilDeadline() const noexcept {
    if (!m_deadline.isValid()) {
        return INT_MAX; // 表示无截止日期
    }
    return QDateTime::currentDateTime().daysTo(m_deadline);
}

/**
 * @brief 比较两个TodoItem是否相等
 * @param other 另一个TodoItem对象
 * @return 如果相等返回true，否则返回false
 */
bool TodoItem::operator==(const TodoItem &other) const {
    return m_id == other.m_id &&                                   // 主键ID
           m_uuid == other.m_uuid &&                               // 唯一标识符
           m_userId == other.m_userId &&                           // 用户ID
           m_title == other.m_title &&                             // 标题
           m_description == other.m_description &&                 // 描述
           m_category == other.m_category &&                       // 类别
           m_important == other.m_important &&                     // 重要性
           m_deadline == other.m_deadline &&                       // 截止日期
           m_recurrenceInterval == other.m_recurrenceInterval &&   // 循环间隔
           m_recurrenceCount == other.m_recurrenceCount &&         // 循环次数
           m_recurrenceStartDate == other.m_recurrenceStartDate && // 循环开始日期
           m_isCompleted == other.m_isCompleted &&                 // 是否已完成
           m_completedAt == other.m_completedAt &&                 // 完成时间
           m_isDeleted == other.m_isDeleted &&                     // 是否已删除
           m_deletedAt == other.m_deletedAt &&                     // 删除时间
           m_createdAt == other.m_createdAt &&                     // 创建时间
           m_updatedAt == other.m_updatedAt &&                     // 最后更新时间
           m_lastModifiedAt == other.m_lastModifiedAt &&           // 最后修改时间
           m_synced == other.m_synced;                             // 同步状态
}

/**
 * @brief 比较两个TodoItem是否不相等
 * @param other 另一个TodoItem对象
 * @return 如果不相等返回true，否则返回false
 */
bool TodoItem::operator!=(const TodoItem &other) const {
    return !(*this == other);
}
