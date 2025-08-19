/**
 * @file todoitem.h
 * @brief TodoItem类的头文件
 *
 * 该文件定义了TodoItem类，用于表示待办事项的数据模型。
 *
 * @author MyTodo Team
 * @date 2024
 */

#ifndef TODOITEM_H
#define TODOITEM_H

#include <QDateTime>
#include <QObject>
#include <QString>

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
 * @see TodoModel
 */
class TodoItem : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY categoryChanged)
    Q_PROPERTY(bool important READ important WRITE setImportant NOTIFY importantChanged)
    Q_PROPERTY(QString deadline READ deadline WRITE setDeadline NOTIFY deadlineChanged)
    Q_PROPERTY(
        int recurrenceInterval READ recurrenceInterval WRITE setRecurrenceInterval NOTIFY recurrenceIntervalChanged)
    Q_PROPERTY(int recurrenceCount READ recurrenceCount WRITE setRecurrenceCount NOTIFY recurrenceCountChanged)
    Q_PROPERTY(QString recurrenceStartDate READ recurrenceStartDate WRITE setRecurrenceStartDate NOTIFY
                   recurrenceStartDateChanged)
    Q_PROPERTY(QString status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(QDateTime createdAt READ createdAt WRITE setCreatedAt NOTIFY createdAtChanged)
    Q_PROPERTY(QDateTime updatedAt READ updatedAt WRITE setUpdatedAt NOTIFY updatedAtChanged)
    Q_PROPERTY(bool synced READ synced WRITE setSynced NOTIFY syncedChanged)
    Q_PROPERTY(QString uuid READ uuid WRITE setUuid NOTIFY uuidChanged)
    Q_PROPERTY(int userId READ userId WRITE setUserId NOTIFY userIdChanged)
    Q_PROPERTY(bool isCompleted READ isCompleted WRITE setIsCompleted NOTIFY isCompletedChanged)
    Q_PROPERTY(QDateTime completedAt READ completedAt WRITE setCompletedAt NOTIFY completedAtChanged)
    Q_PROPERTY(bool isDeleted READ isDeleted WRITE setIsDeleted NOTIFY isDeletedChanged)
    Q_PROPERTY(QDateTime deletedAt READ deletedAt WRITE setDeletedAt NOTIFY deletedAtChanged)
    Q_PROPERTY(QDateTime lastModifiedAt READ lastModifiedAt WRITE setLastModifiedAt NOTIFY lastModifiedAtChanged)

  public:
    /**
     * @brief 默认构造函数
     * @param parent 父对象指针，用于Qt对象树管理
     */
    explicit TodoItem(QObject *parent = nullptr);

    /**
     * @brief 带参数的构造函数
     *
     * 使用指定参数创建TodoItem对象，通常用于从数据库或网络加载数据。
     *
     * @param id 待办事项唯一标识符
     * @param title 待办事项标题
     * @param description 待办事项详细描述
     * @param category 待办事项分类
     * @param important 重要程度
     * @param status 当前状态
     * @param createdAt 创建时间
     * @param updatedAt 最后更新时间
     * @param synced 是否已与服务器同步
     * @param parent 父对象指针
     */
    TodoItem(const QString &id,                             // 待办事项唯一标识符
             const QString &title,                          // 待办事项标题
             const QString &description,                    //
             const QString &category,                       //
             bool important,                                //
             const QString &status,                         //
             const QString &deadline,                       //
             int recurrenceInterval,                        //
             int recurrenceCount,                           //
             const QString &recurrenceStartDate,            //
             const QDateTime &createdAt,                    //
             const QDateTime &updatedAt,                    //
             bool synced,                                   //
             const QString &uuid = QString(),               //
             int userId = 0,                                //
             bool isCompleted = false,                      //
             const QDateTime &completedAt = QDateTime(),    //
             bool isDeleted = false,                        //
             const QDateTime &deletedAt = QDateTime(),      //
             const QDateTime &lastModifiedAt = QDateTime(), //
             QObject *parent = nullptr);

    QString id() const;            // 获取ID
    void setId(const QString &id); // 设置ID

    QString title() const;               // 获取标题
    void setTitle(const QString &title); // 设置标题

    QString description() const;                     // 获取描述
    void setDescription(const QString &description); // 设置描述

    QString category() const;                  // 获取分类
    void setCategory(const QString &category); // 设置分类

    bool important() const;            // 获取重要程度
    void setImportant(bool important); // 设置重要程度

    QString deadline() const;                  // 获取截止日期
    void setDeadline(const QString &deadline); // 设置截止日期

    int recurrenceInterval() const;                     // 获取循环间隔
    void setRecurrenceInterval(int recurrenceInterval); // 设置循环间隔

    int recurrenceCount() const;                  // 获取循环次数
    void setRecurrenceCount(int recurrenceCount); // 设置循环次数

    QString recurrenceStartDate() const;                             // 获取循环开始日期
    void setRecurrenceStartDate(const QString &recurrenceStartDate); // 设置循环开始日期

    QString status() const;                // 获取状态
    void setStatus(const QString &status); // 设置状态

    QDateTime createdAt() const;                   // 获取创建时间
    void setCreatedAt(const QDateTime &createdAt); // 设置创建时间

    QDateTime updatedAt() const;                   // 获取更新时间
    void setUpdatedAt(const QDateTime &updatedAt); // 设置更新时间

    bool synced() const;         // 获取是否已同步
    void setSynced(bool synced); // 设置是否已同步

    QString uuid() const;              // 获取UUID
    void setUuid(const QString &uuid); // 设置UUID

    int userId() const;         // 获取用户ID
    void setUserId(int userId); // 设置用户ID

    bool isCompleted() const;            // 获取是否已完成
    void setIsCompleted(bool completed); // 设置是否已完成

    QDateTime completedAt() const;                     // 获取完成时间
    void setCompletedAt(const QDateTime &completedAt); // 设置完成时间

    bool isDeleted() const;          // 获取是否已删除
    void setIsDeleted(bool deleted); // 设置是否已删除

    QDateTime deletedAt() const;                   // 获取删除时间
    void setDeletedAt(const QDateTime &deletedAt); // 设置删除时间

    QDateTime lastModifiedAt() const;                        // 获取最后修改时间
    void setLastModifiedAt(const QDateTime &lastModifiedAt); // 设置最后修改时间

  signals:
    void idChanged();                  // ID改变信号
    void titleChanged();               // 标题改变信号
    void descriptionChanged();         // 描述改变信号
    void categoryChanged();            // 分类改变信号
    void importantChanged();           // 重要程度改变信号
    void deadlineChanged();            // 截止日期改变信号
    void recurrenceIntervalChanged();  // 循环间隔改变信号
    void recurrenceCountChanged();     // 循环次数改变信号
    void recurrenceStartDateChanged(); // 循环开始日期改变信号
    void statusChanged();              // 状态改变信号
    void createdAtChanged();           // 创建时间改变信号
    void updatedAtChanged();           // 更新时间改变信号
    void syncedChanged();              // 同步状态改变信号
    void uuidChanged();                // UUID改变信号
    void userIdChanged();              // 用户ID改变信号
    void isCompletedChanged();         // 完成状态改变信号
    void completedAtChanged();         // 完成时间改变信号
    void isDeletedChanged();           // 删除状态改变信号
    void deletedAtChanged();           // 删除时间改变信号
    void lastModifiedAtChanged();      // 最后修改时间改变信号

  private:
    QString m_id;                  // 任务ID
    QString m_title;               // 任务标题
    QString m_description;         // 任务描述
    QString m_category;            // 任务分类
    bool m_important;              // 任务重要程度
    QString m_status;              // 任务状态
    QString m_deadline;            // 任务截止日期
    int m_recurrenceInterval;      // 循环间隔（天）
    int m_recurrenceCount;         // 循环次数
    QString m_recurrenceStartDate; // 循环开始日期
    QDateTime m_createdAt;         // 任务创建时间
    QDateTime m_updatedAt;         // 任务更新时间
    bool m_synced;                 // 任务是否已同步
    QString m_uuid;                // 任务UUID
    int m_userId;                  // 用户ID
    bool m_isCompleted;            // 任务是否已完成
    QDateTime m_completedAt;       // 任务完成时间
    bool m_isDeleted;              // 任务是否已删除
    QDateTime m_deletedAt;         // 任务删除时间
    QDateTime m_lastModifiedAt;    // 任务最后修改时间
};

#endif // TODOITEM_H
