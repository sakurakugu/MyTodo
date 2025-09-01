/**
 * @file todo_item.h
 * @brief TodoItem类的头文件
 *
 * 该文件定义了TodoItem类，用于表示待办事项的数据模型。
 *
 * @author Sakurakugu
 * @date 2025-08-22 12:06:17(UTC+8) 周五
 * @version 2025-08-22 14:55:36(UTC+8) 周五
 */

#pragma once

#include <QDate>
#include <QDateTime>
#include <QObject>
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
class TodoItem : public QObject {
    Q_OBJECT
    Q_PROPERTY(int id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QUuid uuid READ uuid WRITE setUuid NOTIFY uuidChanged)
    Q_PROPERTY(QUuid userUuid READ userUuid WRITE setUserUuid NOTIFY userUuidChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY categoryChanged)
    Q_PROPERTY(bool important READ important WRITE setImportant NOTIFY importantChanged)
    Q_PROPERTY(QDateTime deadline READ deadline WRITE setDeadline NOTIFY deadlineChanged)
    Q_PROPERTY(int recurrenceInterval READ recurrenceInterval WRITE setRecurrenceInterval NOTIFY recurrenceIntervalChanged)
    Q_PROPERTY(int recurrenceCount READ recurrenceCount WRITE setRecurrenceCount NOTIFY recurrenceCountChanged)
    Q_PROPERTY(QDate recurrenceStartDate READ recurrenceStartDate WRITE setRecurrenceStartDate NOTIFY recurrenceStartDateChanged)
    Q_PROPERTY(bool isCompleted READ isCompleted WRITE setIsCompleted NOTIFY isCompletedChanged)
    Q_PROPERTY(QDateTime completedAt READ completedAt WRITE setCompletedAt NOTIFY completedAtChanged)
    Q_PROPERTY(bool isDeleted READ isDeleted WRITE setIsDeleted NOTIFY isDeletedChanged)
    Q_PROPERTY(QDateTime deletedAt READ deletedAt WRITE setDeletedAt NOTIFY deletedAtChanged)
    Q_PROPERTY(QDateTime createdAt READ createdAt WRITE setCreatedAt NOTIFY createdAtChanged)
    Q_PROPERTY(QDateTime updatedAt READ updatedAt WRITE setUpdatedAt NOTIFY updatedAtChanged)
    Q_PROPERTY(QDateTime lastModifiedAt READ lastModifiedAt WRITE setLastModifiedAt NOTIFY lastModifiedAtChanged)
    Q_PROPERTY(bool synced READ synced WRITE setSynced NOTIFY syncedChanged)

  public:
    TodoItem(const TodoItem &) = delete;
    TodoItem &operator=(const TodoItem &) = delete;
    explicit TodoItem(QObject *parent = nullptr);
    TodoItem(int id,                           ///< 待办事项唯一标识符
             const QUuid &uuid,                ///< 唯一标识符
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
             const QDateTime &updatedAt,       ///< 最后更新时间（整个对象最后更新时间，用于同步）
             const QDateTime &lastModifiedAt,  ///< 最后修改时间（标题、描述、分类等）
             bool synced,                      ///< 是否已与服务器同步
             QObject *parent = nullptr);

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

    QDateTime lastModifiedAt() const noexcept { return m_lastModifiedAt; } // 获取最后修改时间
    void setLastModifiedAt(const QDateTime &lastModifiedAt);               // 设置最后修改时间

    bool synced() const noexcept { return m_synced; } // 获取是否已同步
    void setSynced(bool synced);                      // 设置是否已同步

    // 便利方法
    bool isOverdue() const noexcept;                                                         // 检查是否已过期
    constexpr bool isRecurring() const noexcept;                                             // 检查是否为重复任务
    bool isDue(const QDateTime &checkTime = QDateTime::currentDateTime()) const noexcept;    // 检查是否到期
    int daysUntilDeadline() const noexcept;                                                  // 距离截止日期的天数
    bool isInRecurrencePeriod(const QDate &checkDate = QDate::currentDate()) const noexcept; // 检查指定日期是否在重复周期内

    // 比较操作符
    bool operator==(const TodoItem &other) const;
    bool operator!=(const TodoItem &other) const;

  signals:
    void idChanged();                  ///< ID改变信号
    void uuidChanged();                ///< UUID改变信号
    void userUuidChanged();            ///< 用户UUID改变信号
    void titleChanged();               ///< 标题改变信号
    void descriptionChanged();         ///< 描述改变信号
    void categoryChanged();            ///< 分类改变信号
    void importantChanged();           ///< 重要程度改变信号
    void deadlineChanged();            ///< 截止日期改变信号
    void recurrenceIntervalChanged();  ///< 循环间隔改变信号
    void recurrenceCountChanged();     ///< 循环次数改变信号
    void recurrenceStartDateChanged(); ///< 循环开始日期改变信号
    void isCompletedChanged();         ///< 完成状态改变信号
    void completedAtChanged();         ///< 完成时间改变信号
    void isDeletedChanged();           ///< 删除状态改变信号
    void deletedAtChanged();           ///< 删除时间改变信号
    void createdAtChanged();           ///< 创建时间改变信号
    void updatedAtChanged();           ///< 更新时间改变信号
    void lastModifiedAtChanged();      ///< 最后修改时间改变信号
    void syncedChanged();              ///< 同步状态改变信号

  private:
    /**
     * @brief 通用属性设置方法
     * @tparam T 属性类型，必须支持比较和赋值操作
     * @param member 成员变量引用
     * @param value 新值
     * @param signal 信号指针
     */
    template <typename T>
    constexpr void setProperty(T& member, const T& value, void (TodoItem::*signal)()) {
        if (member != value) {
            member = value;
            emit(this->*signal)();
        }
    }

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
    QDateTime m_lastModifiedAt;  // 任务最后修改时间
    bool m_synced;               // 任务是否已同步
};
