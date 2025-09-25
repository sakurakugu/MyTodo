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

#include <QDateTime>
#include <QString>
#include <QUuid>

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
 * 该类继承自QObject，支持Qt的属性系统和信号槽机制，
 * 可以直接在QML中使用，实现数据绑定和自动更新UI。
 *
 * @note 所有属性都有对应的getter、setter方法和change信号
 * @see TodoManager
 */
class TodoItem {

  public:
    TodoItem(const TodoItem &) = delete;
    TodoItem &operator=(const TodoItem &) = delete;
    TodoItem &operator=(TodoItem &&other) noexcept;
    bool operator==(const TodoItem &other) const noexcept;
    bool operator!=(const TodoItem &other) const noexcept;

    explicit TodoItem();
    TodoItem(int id,                           ///< 唯一标识符
             const QUuid &uuid,                ///< 唯一标识符（UUID）
             const QUuid &userUuid,            ///< 用户UUID
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
             int synced                        ///< 是否已与服务器同步（是否要上传）
    );

    int id() const noexcept { return m_id; } // 获取ID
    void setId(int id);                      // 设置ID

    QUuid uuid() const noexcept { return m_uuid; } // 获取UUID
    void setUuid(const QUuid &uuid);               // 设置UUID

    QUuid userUuid() const noexcept { return m_userUuid; } // 获取用户UUID
    void setUserUuid(const QUuid &userUuid);               // 设置用户UUID

    QString title() const noexcept { return m_title; } // 获取标题
    void setTitle(const QString &title);               // 设置标题

    QString description() const noexcept { return m_description; } // 获取描述
    void setDescription(const QString &description);               // 设置描述

    QString category() const noexcept { return m_category; } // 获取分类
    void setCategory(const QString &category);               // 设置分类

    bool important() const noexcept { return m_important; } // 获取重要程度
    void setImportant(bool important);                      // 设置重要程度

    QDateTime deadline() const noexcept { return m_deadline; } // 获取截止日期
    void setDeadline(const QDateTime &deadline);               // 设置截止日期

    int recurrenceInterval() const noexcept { return m_recurrenceInterval; } // 获取循环间隔
    void setRecurrenceInterval(int recurrenceInterval);                      // 设置循环间隔

    int recurrenceCount() const noexcept { return m_recurrenceCount; } // 获取循环次数
    void setRecurrenceCount(int recurrenceCount);                      // 设置循环次数

    QDate recurrenceStartDate() const noexcept { return m_recurrenceStartDate; } // 获取循环开始日期
    void setRecurrenceStartDate(const QDate &recurrenceStartDate);               // 设置循环开始日期

    bool isCompleted() const noexcept { return m_isCompleted; } // 获取是否已完成
    void setIsCompleted(bool completed);                        // 设置是否已完成

    QDateTime completedAt() const noexcept { return m_completedAt; } // 获取完成时间
    void setCompletedAt(const QDateTime &completedAt);               // 设置完成时间

    bool isDeleted() const noexcept { return m_isDeleted; } // 获取是否已删除
    void setIsDeleted(bool deleted);                        // 设置是否已删除

    QDateTime deletedAt() const noexcept { return m_deletedAt; } // 获取删除时间
    void setDeletedAt(const QDateTime &deletedAt);               // 设置删除时间

    QDateTime createdAt() const noexcept { return m_createdAt; } // 获取创建时间
    void setCreatedAt(const QDateTime &createdAt);               // 设置创建时间

    QDateTime updatedAt() const noexcept { return m_updatedAt; } // 获取更新时间
    void setUpdatedAt(const QDateTime &updatedAt);               // 设置更新时间

    int synced() const noexcept { return m_synced; } // 获取是否已同步
    void setSynced(int synced);                      // 设置是否已同步

    // 便利方法
    bool isOverdue() const noexcept;                                                      // 检查是否已过期
    constexpr bool isRecurring() const noexcept;                                          // 检查是否为重复任务
    bool isDue(const QDateTime &checkTime = QDateTime::currentDateTime()) const noexcept; // 检查是否到期
    int daysUntilDeadline() const noexcept;                                               // 距离截止日期的天数
    bool isInRecurrencePeriod(
        const QDate &checkDate = QDate::currentDate()) const noexcept; // 检查指定日期是否在重复周期内



  private:
    // 成员变量
    int m_id;                    // 任务ID
    QUuid m_uuid;                // 任务UUID
    QUuid m_userUuid;            // 用户UUID
    QString m_title;             // 任务标题
    QString m_description;       // 任务描述
    QString m_category;          // 任务分类
    bool m_important;            // 任务重要程度
    QDateTime m_deadline;        // 任务截止日期
    int m_recurrenceInterval;    // 循环间隔（天）
    int m_recurrenceCount;       // 循环次数
    QDate m_recurrenceStartDate; // 循环开始日期
    bool m_isCompleted;          // 任务是否已完成
    QDateTime m_completedAt;     // 任务完成时间
    bool m_isDeleted;            // 任务是否已删除
    QDateTime m_deletedAt;       // 任务删除时间
    QDateTime m_createdAt;       // 任务创建时间
    QDateTime m_updatedAt;       // 任务更新时间
    int m_synced;                // 任务是否已同步
};
