/**
 * @file todo_item.cpp
 * @brief TodoItem类的实现文件
 *
 * 该文件实现了TodoItem类，用于表示待办事项的数据模型。
 * TodoItem包含了待办事项的所有属性，如标题、描述、分类、优先级等，
 * 并提供了相应的getter和setter方法，支持Qt的属性系统和信号槽机制。
 *
 * @author Sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @change 2025-09-22 22:55:11(UTC+8) 周一
 */

#include "todo_item.h"
#include "holiday_manager.h"

/**
 * @brief 默认构造函数
 *
 * 创建一个空的TodoItem对象，所有字符串属性为空，
 * 时间属性为无效时间，同步状态为false。
 *
 * @param parent 父对象指针，用于Qt的对象树管理
 */
TodoItem::TodoItem()
    : m_id(0),                            // 初始化待办事项ID为0
      m_uuid(uuids::uuid()),              // 初始化UUID
      m_userUuid(uuids::uuid()),          // 初始化用户UUID为空字符串
      m_important(false),                 // 初始化重要程度为false
      m_deadline(QDateTime()),            // 初始化截止日期为空
      m_recurrenceInterval(0),            // 初始化循环间隔为0
      m_recurrenceCount(0),               // 初始化循环次数为0
      m_recurrenceStartDate(QDate()),     // 初始化重复开始日期为空
      m_isCompleted(false),               // 初始化是否已完成为false
      m_isTrashed(false),                 // 初始化是否已回收为false
      m_createdAt(my::DateTime::today()), // 初始化创建时间为当前时间
      m_updatedAt(my::DateTime::today()), // 初始化更新时间为当前时间
      m_synced(1)                         // 初始化是否已同步为false
{}

/**
 * @brief 带参数的构造函数
 *
 * 使用指定的参数创建TodoItem对象。这个构造函数通常用于
 * 从数据库或网络加载已存在的待办事项数据。
 */
TodoItem::TodoItem(int id,                              ///< 待办事项唯一标识符
                   const uuids::uuid &uuid,             ///< 唯一标识符
                   const uuids::uuid &userUuid,         ///< 用户UUID
                   const std::string &title,            ///< 待办事项标题
                   const std::string &description,      ///< 待办事项详细描述
                   const std::string &category,         ///< 待办事项分类
                   bool important,                      ///< 重要程度
                   const my::DateTime &deadline,        ///< 截止日期、重复结束日期
                   int recurrenceInterval,              ///< 重复间隔
                   int recurrenceCount,                 ///< 重复次数
                   const my::Date &recurrenceStartDate, ///< 重复开始日期
                   bool isCompleted,                    ///< 是否已完成
                   const my::DateTime &completedAt,     ///< 完成时间
                   bool isTrashed,                      ///< 是否已回收
                   const my::DateTime &trashedAt,       ///< 回收时间
                   const my::DateTime &createdAt,       ///< 创建时间
                   const my::DateTime &updatedAt,       ///< 最后更新时间
                   int synced)                          ///< 是否已与服务器同步
    : m_id(id),                                         ///< 初始化待办事项ID
      m_uuid(uuid),                                     ///< 初始化待办事项UUID
      m_userUuid(userUuid),                             ///< 初始化用户UUID
      m_title(title),                                   ///< 初始化待办事项标题
      m_description(description),                       ///< 初始化待办事项描述
      m_category(category),                             ///< 初始化待办事项分类
      m_important(important),                           ///< 初始化待办事项重要程度
      m_deadline(deadline),                             ///< 初始化待办事项截止日期
      m_recurrenceInterval(recurrenceInterval),         ///< 初始化循环间隔
      m_recurrenceCount(recurrenceCount),               ///< 初始化循环次数
      m_recurrenceStartDate(recurrenceStartDate),       ///< 初始化循环开始日期
      m_isCompleted(isCompleted),                       ///< 初始化完成状态
      m_completedAt(completedAt),                       ///< 初始化完成时间
      m_isTrashed(isTrashed),                           ///< 初始化回收状态
      m_trashedAt(trashedAt),                           ///< 初始化回收时间
      m_createdAt(createdAt),                           ///< 初始化待办事项创建时间
      m_updatedAt(updatedAt),                           ///< 初始化待办事项更新时间
      m_synced(synced)                                  ///< 初始化待办事项同步状态
{}

/**
 * @brief 设置待办事项的唯一标识符
 * @param id 新的待办事项ID
 */
void TodoItem::setId(int id) {
    if (m_id != id) {
        m_id = id;
    }
}

/**
 * @brief 设置待办事项的UUID
 * @param uuid 新的UUID
 */
void TodoItem::setUuid(const uuids::uuid &uuid) {
    if (m_uuid != uuid) {
        m_uuid = uuid;
    }
}

/**
 * @brief 设置用户UUID
 * @param userUuid 新的用户UUID
 */
void TodoItem::setUserUuid(const uuids::uuid &userUuid) {
    if (m_userUuid != userUuid) {
        m_userUuid = userUuid;
    }
}

/**
 * @brief 设置待办事项标题
 * @param title 新的待办事项标题
 */
void TodoItem::setTitle(const std::string &title) {
    std::string title_;
    if (title.length() > 255) {
        title_ = title.substr(0, 240) + "......";
    } else [[likely]] {
        title_ = title;
    }
    if (m_title != title_) {
        m_title = title_;
    }
}

/**
 * @brief 设置待办事项描述
 * @param description 新的待办事项描述
 */
void TodoItem::setDescription(const std::string &description) {
    if (m_description != description) {
        m_description = description;
    }
}

/**
 * @brief 设置待办事项分类
 * @param category 新的待办事项分类
 */
void TodoItem::setCategory(const std::string &category) {
    std::string category_;
    if (category.length() > 50) {
        category_ = category.substr(0, 40) + "......";
    } else [[likely]] {
        category_ = category;
    }
    if (m_category != category_) {
        m_category = category_;
    }
}

/**
 * @brief 设置待办事项重要程度
 * @param important 新的重要程度
 */
void TodoItem::setImportant(bool important) {
    if (m_important != important) {
        m_important = important;
    }
}

/**
 * @brief 设置待办事项截止日期
 * @param deadline 新的截止日期
 */
void TodoItem::setDeadline(const my::DateTime &deadline) {
    if (m_deadline != deadline) {
        m_deadline = deadline;
    }
}

/**
 * @brief 设置循环间隔
 * @param recurrenceInterval 新的循环间隔
 */
void TodoItem::setRecurrenceInterval(int recurrenceInterval) {
    int interval_ = recurrenceInterval;
    if (interval_ < 0) {
        switch (interval_) {
        case -1:
        case -2:
        case -3:
        case -5:
        case -7:
        case -30:
        case -365:
        case -999:
            break;
        default:
            interval_ = 0;
            break;
        }
    }
    if (m_recurrenceInterval != interval_) {
        m_recurrenceInterval = interval_;
    }
}

/**
 * @brief 设置循环次数
 * @param recurrenceCount 新的循环次数
 */
void TodoItem::setRecurrenceCount(int recurrenceCount) {
    if (m_recurrenceCount != recurrenceCount) {
        m_recurrenceCount = recurrenceCount;
    }
}

/**
 * @brief 设置循环开始日期
 * @param recurrenceStartDate 新的循环开始日期
 */
void TodoItem::setRecurrenceStartDate(const my::Date &recurrenceStartDate) {
    if (m_recurrenceStartDate != recurrenceStartDate) {
        m_recurrenceStartDate = recurrenceStartDate;
    }
}

/**
 * @brief 设置是否已完成
 * @param completed 新的完成状态
 */
void TodoItem::setIsCompleted(bool completed) {
    if (m_isCompleted != completed) {
        m_isCompleted = completed;
    }
}

/**
 * @brief 设置完成时间
 * @param completedAt 新的完成时间
 */
void TodoItem::setCompletedAt(const my::DateTime &completedAt) {
    if (m_completedAt != completedAt) {
        m_completedAt = completedAt;
    }
}

/**
 * @brief 设置是否已回收
 * @param trashed 新的回收状态
 */
void TodoItem::setIsTrashed(bool trashed) {
    if (m_isTrashed != trashed) {
        m_isTrashed = trashed;
    }
}

/**
 * @brief 设置回收时间
 * @param trashedAt 新的回收时间
 */
void TodoItem::setTrashedAt(const my::DateTime &trashedAt) {
    if (m_trashedAt != trashedAt) {
        m_trashedAt = trashedAt;
    }
}

/**
 * @brief 设置待办事项创建时间
 * @param createdAt 新的创建时间
 */
void TodoItem::setCreatedAt(const my::DateTime &createdAt) {
    if (m_createdAt != createdAt) {
        m_createdAt = createdAt;
    }
}

/**
 * @brief 设置待办事项最后更新时间
 * @param updatedAt 新的更新时间
 */
void TodoItem::setUpdatedAt(const my::DateTime &updatedAt) {
    if (m_updatedAt != updatedAt) {
        m_updatedAt = updatedAt;
    }
}

/**
 * @brief 设置待办事项同步状态
 * @param synced 新的同步状态
 */
void TodoItem::setSynced(int synced) {
    if (m_synced == synced)
        return;

    // 如果之前是新建(1)，现在要改为更新(2)，保持不变
    if (m_synced == 1 && synced == 2)
        return;

    m_synced = synced;
}

/**
 * @brief 强制设置待办事项同步状态
 * @param synced 新的同步状态
 */
void TodoItem::forceSetSynced(int synced) {
    if (m_synced == synced)
        return;

    m_synced = synced;
}

// 便利方法实现

/**
 * @brief 检查待办事项是否已过期
 * @return 如果已过期返回true，否则返回false
 */
bool TodoItem::isOverdue() const noexcept {
    return m_deadline.isValid() && m_deadline < my::DateTime::today() && !m_isCompleted;
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
bool TodoItem::isDue(const my::DateTime &checkTime) const noexcept {
    if (!m_deadline.isValid() || m_isCompleted) {
        return false;
    }
    auto time = checkTime;
    return m_deadline <= time.addDays(1);
}

/**
 * @brief 计算距离截止日期的天数
 * @param checkTime 检查时间，无截止日期时默认当前时间
 * @return 距离截止日期的天数，负数表示已过期
 */
int TodoItem::daysUntilDeadline() const noexcept {
    if (!m_deadline.isValid()) {
        return INT_MAX; // 表示无截止日期
    }
    return my::DateTime::today().daysTo(m_deadline);
}

/**
 * @brief 检查指定日期是否在重复周期内
 * @param checkDate 要检查的日期，默认为当前日期
 * @return 如果在重复周期内返回true，否则返回false
 */
bool TodoItem::isInRecurrencePeriod(const my::Date &checkDate) const noexcept {
    // 如果不是重复任务，返回false
    if (!isRecurring()) {
        return false;
    }

    // 如果重复开始日期无效，返回false
    if (!m_recurrenceStartDate.isValid()) {
        return false;
    }

    // 检查日期是否在重复开始日期之前
    if (checkDate < m_recurrenceStartDate) {
        return false;
    }

    // 如果有截止日期，检查是否超过截止日期
    if (m_deadline.isValid() && checkDate > m_deadline.date()) {
        return false;
    }

    // 计算从开始日期到检查日期的天数
    int daysSinceStart = m_recurrenceStartDate.daysTo(checkDate);

    bool isValidInterval = false; // 检查是否符合循环间隔
    int occurrenceNumber = 0;     // 循环次数，从1开始计算

    if (m_recurrenceInterval > 0) {
        // 正数：按照天数间隔进行正常循环判断
        if (daysSinceStart % m_recurrenceInterval == 0) {
            isValidInterval = true;
            occurrenceNumber = daysSinceStart / m_recurrenceInterval + 1;
        }
    } else if (m_recurrenceInterval < 0) {
        // 负数：按照特定时间单位进行循环判断
        HolidayManager &holidayManager = HolidayManager::GetInstance();
        switch (m_recurrenceInterval) {
        case -1:
            // 每天
            isValidInterval = true;
            occurrenceNumber = daysSinceStart + 1;
            break;
        case -7: // 每周（每7天）
            if (daysSinceStart % 7 == 0) {
                isValidInterval = true;
                occurrenceNumber = daysSinceStart / 7 + 1;
            }
            break;
        case -30: { // 每月（检查是否为同一天）
            const my::Date &startDate = m_recurrenceStartDate;
            if (checkDate.day() == startDate.day()) {
                int monthsDiff = (checkDate.year() - startDate.year()) * 12 + (checkDate.month() - startDate.month());
                if (monthsDiff >= 0) {
                    isValidInterval = true;
                    occurrenceNumber = monthsDiff + 1;
                }
            }
            break;
        }
        case -365: { // 每年（检查是否为同一月同一天）
            const my::Date &startDate = m_recurrenceStartDate;
            if (checkDate.month() == startDate.month() && checkDate.day() == startDate.day()) {
                int yearsDiff = checkDate.year() - startDate.year();
                if (yearsDiff >= 0) {
                    isValidInterval = true;
                    occurrenceNumber = yearsDiff + 1;
                }
            }
            break;
        }
        case -5: { // 每工作日
            auto dateType = holidayManager.getDateType(checkDate);
            if (dateType == HolidayManager::DateType::WorkDay) {
                isValidInterval = true;
                // 计算从开始日期到当前日期的工作日数量
                int workDayCount = 0;
                my::Date currentDate = m_recurrenceStartDate;
                while (currentDate <= checkDate) {
                    if (holidayManager.getDateType(currentDate) == HolidayManager::DateType::WorkDay) {
                        workDayCount++;
                    }
                    currentDate = currentDate.addDays(1);
                }
                occurrenceNumber = workDayCount;
            }
            break;
        }
        case -3: { // 每节假日
            auto dateType = holidayManager.getDateType(checkDate);
            if (dateType == HolidayManager::DateType::Holiday) {
                isValidInterval = true;
                // 计算从开始日期到当前日期的节假日数量
                int holidayCount = 0;
                my::Date currentDate = m_recurrenceStartDate;
                while (currentDate <= checkDate) {
                    if (holidayManager.getDateType(currentDate) == HolidayManager::DateType::Holiday) {
                        holidayCount++;
                    }
                    currentDate = currentDate.addDays(1);
                }
                occurrenceNumber = holidayCount;
            }
            break;
        }
        case -2: { // 每周末
            auto dateType = holidayManager.getDateType(checkDate);
            if (dateType == HolidayManager::DateType::Weekend) {
                isValidInterval = true;
                // 计算从开始日期到当前日期的周末数量
                int weekendCount = 0;
                my::Date currentDate = my::Date(m_recurrenceStartDate);
                while (currentDate <= checkDate) {
                    if (holidayManager.getDateType(currentDate) == HolidayManager::DateType::Weekend) {
                        weekendCount++;
                    }
                    currentDate = currentDate.addDays(1);
                }
                occurrenceNumber = weekendCount;
            }
            break;
        }
        case -999: { // 无限循环
            isValidInterval = true;
            occurrenceNumber = daysSinceStart + 1;
            break;
        }
        default:
            break;
        }
    }

    if (!isValidInterval) {
        return false;
    }

    // 如果设置了重复次数，检查是否超过重复次数
    if (m_recurrenceCount > 0 && occurrenceNumber > m_recurrenceCount) {
        return false;
    }

    return true;
}

/**
 * @brief 移动赋值运算符
 * @param other 要移动的TodoItem对象
 * @return 当前对象的引用
 */
TodoItem &TodoItem::operator=(TodoItem &&other) noexcept {
    if (this != &other) {
        // 移动所有成员变量
        m_id = std::move(other.m_id);
        m_uuid = std::move(other.m_uuid);
        m_userUuid = std::move(other.m_userUuid);
        m_title = std::move(other.m_title);
        m_description = std::move(other.m_description);
        m_category = std::move(other.m_category);
        m_important = std::move(other.m_important);
        m_deadline = std::move(other.m_deadline);
        m_recurrenceInterval = std::move(other.m_recurrenceInterval);
        m_recurrenceCount = std::move(other.m_recurrenceCount);
        m_recurrenceStartDate = std::move(other.m_recurrenceStartDate);
        m_isCompleted = std::move(other.m_isCompleted);
        m_completedAt = std::move(other.m_completedAt);
        m_isTrashed = std::move(other.m_isTrashed);
        m_trashedAt = std::move(other.m_trashedAt);
        m_createdAt = std::move(other.m_createdAt);
        m_updatedAt = std::move(other.m_updatedAt);
        m_synced = std::move(other.m_synced);
    }
    return *this;
}

/**
 * @brief 比较两个TodoItem是否相等
 * @param other 另一个TodoItem对象
 * @return 如果相等返回true，否则返回false
 */
bool TodoItem::operator==(const TodoItem &other) const noexcept {
    return m_id == other.m_id &&                                   // 主键ID
           m_uuid == other.m_uuid &&                               // 唯一标识符
           m_userUuid == other.m_userUuid &&                       // 用户ID
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
           m_isTrashed == other.m_isTrashed &&                     // 是否已删除
           m_trashedAt == other.m_trashedAt &&                     // 删除时间
           m_createdAt == other.m_createdAt &&                     // 创建时间
           m_updatedAt == other.m_updatedAt &&                     // 最后更新时间
           m_synced == other.m_synced;                             // 同步状态
}

/**
 * @brief 比较两个TodoItem是否不相等
 * @param other 另一个TodoItem对象
 * @return 如果不相等返回true，否则返回false
 */
bool TodoItem::operator!=(const TodoItem &other) const noexcept {
    return !(*this == other);
}
