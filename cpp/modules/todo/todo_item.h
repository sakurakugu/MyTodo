/**
 * @file todo_item.h
 * @brief TodoItem类的头文件
 *
 * 该文件定义了TodoItem类，用于表示待办事项的数据模型。
 *
 * @author Sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @change 2025-09-22 16:33:30(UTC+8) 周一
 */

#pragma once

#include "datetime.h"
#include <string>
#include <uuid.h>

/**
 * @class TodoItem
 * @brief 表示单个待办事项的数据模型类
 *
 * TodoItem类封装了一个待办事项的所有属性，包括：
 * - 基本信息：ID、标题、描述
 * - 分类信息：分类、重要程度
 * - 状态信息：当前状态、创建时间、更新时间
 * - 同步信息：是否已与服务器同步
 *
 * @note 所有属性都有对应的getter、setter方法
 * @see TodoManager
 */
class TodoItem {

  public:
    TodoItem(const TodoItem &) = delete; // 禁止拷贝构造，因为包含唯一标识符uuid
    TodoItem &operator=(const TodoItem &) = delete;
    TodoItem &operator=(TodoItem &&other) noexcept;
    bool operator==(const TodoItem &other) const noexcept;
    bool operator!=(const TodoItem &other) const noexcept;

    explicit TodoItem();
    TodoItem(int id,                                  ///< 唯一标识符
             const uuids::uuid &uuid,                 ///< 唯一标识符（UUID）
             const uuids::uuid &userUuid,             ///< 用户UUID
             const std::string &title,                ///< 待办事项标题
             const std::string &description,          ///< 待办事项详细描述
             const std::string &category,             ///< 待办事项分类
             bool important,                          ///< 重要程度
             const my::DateTime &deadline,            ///< 截止日期、重复结束日期
             int recurrenceInterval,                  ///< 重复间隔
             int recurrenceCount,                     ///< 重复次数
             const my::Date &recurrenceStartDate, ///< 重复开始日期
             bool isCompleted,                        ///< 是否已完成
             const my::DateTime &completedAt,         ///< 完成时间
             bool isTrashed,                          ///< 是否已回收
             const my::DateTime &trashedAt,           ///< 回收时间
             const my::DateTime &createdAt,           ///< 创建时间
             const my::DateTime &updatedAt,           ///< 最后更新时间
             int synced                               ///< 是否已与服务器同步（是否要上传）
                                                      ///< 0表示已同步，1表示待插入，2表示待更新，3表示待删除
    );

    int id() const noexcept { return m_id; } // 获取ID
    void setId(int id);                      // 设置ID

    uuids::uuid uuid() const noexcept { return m_uuid; } // 获取UUID
    void setUuid(const uuids::uuid &uuid);               // 设置UUID

    uuids::uuid userUuid() const noexcept { return m_userUuid; } // 获取用户UUID
    void setUserUuid(const uuids::uuid &userUuid);               // 设置用户UUID

    std::string title() const noexcept { return m_title; } // 获取标题
    void setTitle(const std::string &title);               // 设置标题

    std::string description() const noexcept { return m_description; } // 获取描述
    void setDescription(const std::string &description);               // 设置描述

    std::string category() const noexcept { return m_category; } // 获取分类
    void setCategory(const std::string &category);               // 设置分类

    bool important() const noexcept { return m_important; } // 获取重要程度
    void setImportant(bool important);                      // 设置重要程度

    my::DateTime deadline() const noexcept { return m_deadline; } // 获取截止日期
    void setDeadline(const my::DateTime &deadline);               // 设置截止日期

    int recurrenceInterval() const noexcept { return m_recurrenceInterval; } // 获取循环间隔
    void setRecurrenceInterval(int recurrenceInterval);                      // 设置循环间隔

    int recurrenceCount() const noexcept { return m_recurrenceCount; } // 获取循环次数
    void setRecurrenceCount(int recurrenceCount);                      // 设置循环次数

    my::Date recurrenceStartDate() const noexcept { return m_recurrenceStartDate; } // 获取循环开始日期
    void setRecurrenceStartDate(const my::Date &recurrenceStartDate);               // 设置循环开始日期

    bool isCompleted() const noexcept { return m_isCompleted; } // 获取是否已完成
    void setIsCompleted(bool completed);                        // 设置是否已完成

    my::DateTime completedAt() const noexcept { return m_completedAt; } // 获取完成时间
    void setCompletedAt(const my::DateTime &completedAt);               // 设置完成时间

    bool isTrashed() const noexcept { return m_isTrashed; } // 获取是否已回收
    void setIsTrashed(bool trashed);                        // 设置是否已回收

    my::DateTime trashedAt() const noexcept { return m_trashedAt; } // 获取回收时间
    void setTrashedAt(const my::DateTime &trashedAt);               // 设置回收时间

    my::DateTime createdAt() const noexcept { return m_createdAt; } // 获取创建时间
    void setCreatedAt(const my::DateTime &createdAt);               // 设置创建时间

    my::DateTime updatedAt() const noexcept { return m_updatedAt; } // 获取更新时间
    void setUpdatedAt(const my::DateTime &updatedAt);               // 设置更新时间

    int synced() const noexcept { return m_synced; } // 获取是否已同步
    void setSynced(int synced);                      // 设置是否已同步
    void forceSetSynced(int synced);                 // 强制设置是否已同步

    // 便利方法

    bool isOverdue() const noexcept;                                                  // 检查是否已过期
    constexpr bool isRecurring() const noexcept;                                      // 检查是否为重复任务
    bool isDue(const my::DateTime &checkTime = my::DateTime::today()) const noexcept; // 检查是否到期
    int daysUntilDeadline() const noexcept;                                           // 距离截止日期的天数
    bool isInRecurrencePeriod(
        const my::Date &checkDate = my::Date::today()) const noexcept; // 检查指定日期是否在重复周期内

  private:
    // 成员变量
    int m_id;                       // 任务ID
    uuids::uuid m_uuid;             // 任务UUID
    uuids::uuid m_userUuid;         // 用户UUID
    std::string m_title;            // 任务标题
    std::string m_description;      // 任务描述
    std::string m_category;         // 任务分类
    bool m_important;               // 任务重要程度
    my::DateTime m_deadline;        // 任务截止日期
    int m_recurrenceInterval;       // 循环间隔（天）
    int m_recurrenceCount;          // 循环次数
    my::Date m_recurrenceStartDate; // 循环开始日期
    bool m_isCompleted;             // 任务是否已完成
    my::DateTime m_completedAt;     // 任务完成时间
    bool m_isTrashed;               // 任务是否已回收
    my::DateTime m_trashedAt;       // 任务回收时间
    my::DateTime m_createdAt;       // 任务创建时间
    my::DateTime m_updatedAt;       // 任务更新时间
    int m_synced;                   // 任务是否已同步
};
