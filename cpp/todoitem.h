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
    Q_PROPERTY(QString importance READ importance WRITE setImportance NOTIFY importanceChanged)
    Q_PROPERTY(QString status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(QDateTime createdAt READ createdAt WRITE setCreatedAt NOTIFY createdAtChanged)
    Q_PROPERTY(QDateTime updatedAt READ updatedAt WRITE setUpdatedAt NOTIFY updatedAtChanged)
    Q_PROPERTY(bool synced READ synced WRITE setSynced NOTIFY syncedChanged)

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
     * @param importance 重要程度
     * @param status 当前状态
     * @param createdAt 创建时间
     * @param updatedAt 最后更新时间
     * @param synced 是否已与服务器同步
     * @param parent 父对象指针
     */
    TodoItem(const QString &id, const QString &title, const QString &description, const QString &category, const QString &importance, const QString &status, const QDateTime &createdAt,
             const QDateTime &updatedAt, bool synced, QObject *parent = nullptr);

    QString id() const;             // 获取ID
    void setId(const QString &id);  // 设置ID

    QString title() const;                // 获取标题
    void setTitle(const QString &title);  // 设置标题

    QString description() const;                      // 获取描述
    void setDescription(const QString &description);  // 设置描述

    QString category() const;                   // 获取分类
    void setCategory(const QString &category);  // 设置分类

    QString importance() const;                     // 获取重要程度
    void setImportance(const QString &importance);  // 设置重要程度

    QString status() const;                 // 获取状态
    void setStatus(const QString &status);  // 设置状态

    QDateTime createdAt() const;                    // 获取创建时间
    void setCreatedAt(const QDateTime &createdAt);  // 设置创建时间

    QDateTime updatedAt() const;                    // 获取更新时间
    void setUpdatedAt(const QDateTime &updatedAt);  // 设置更新时间

    bool synced() const;          // 获取是否已同步
    void setSynced(bool synced);  // 设置是否已同步

  signals:
    void idChanged();           // ID改变信号
    void titleChanged();        // 标题改变信号
    void descriptionChanged();  // 描述改变信号
    void categoryChanged();     // 分类改变信号
    void importanceChanged();   // 重要程度改变信号
    void statusChanged();       // 状态改变信号
    void createdAtChanged();    // 创建时间改变信号
    void updatedAtChanged();    // 更新时间改变信号
    void syncedChanged();       // 同步状态改变信号

  private:
    QString m_id;           // 任务ID
    QString m_title;        // 任务标题
    QString m_description;  // 任务描述
    QString m_category;     // 任务分类
    QString m_importance;   // 任务重要程度
    QString m_status;       // 任务状态
    QDateTime m_createdAt;  // 任务创建时间
    QDateTime m_updatedAt;  // 任务更新时间
    bool m_synced;          // 任务是否已同步
};

#endif  // TODOITEM_H
