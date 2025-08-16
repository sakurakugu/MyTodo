#ifndef TODOITEM_H
#define TODOITEM_H

#include <QDateTime>
#include <QObject>
#include <QString>

/**
 * @class TodoItem
 * @brief 表示单个待办事项的类
 * 
 * TodoItem类封装了一个待办事项的所有属性，包括标题、描述、分类、
 * 优先级信息以及同步状态等。该类支持Qt属性系统，可在QML中使用。
 */
class TodoItem : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY categoryChanged)
    Q_PROPERTY(QString urgency READ urgency WRITE setUrgency NOTIFY urgencyChanged)
    Q_PROPERTY(QString importance READ importance WRITE setImportance NOTIFY importanceChanged)
    Q_PROPERTY(QString status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(QDateTime createdAt READ createdAt WRITE setCreatedAt NOTIFY createdAtChanged)
    Q_PROPERTY(QDateTime updatedAt READ updatedAt WRITE setUpdatedAt NOTIFY updatedAtChanged)
    Q_PROPERTY(bool synced READ synced WRITE setSynced NOTIFY syncedChanged)

  public:
    explicit TodoItem(QObject *parent = nullptr);
    TodoItem(const QString &id, const QString &title, const QString &description, const QString &category,
             const QString &urgency, const QString &importance, const QString &status, const QDateTime &createdAt,
             const QDateTime &updatedAt, bool synced, QObject *parent = nullptr);

    QString id() const;            // 获取ID
    void setId(const QString &id); // 设置ID

    QString title() const;               // 获取标题
    void setTitle(const QString &title); // 设置标题

    QString description() const;                     // 获取描述
    void setDescription(const QString &description); // 设置描述

    QString category() const;                  // 获取分类
    void setCategory(const QString &category); // 设置分类

    QString urgency() const;                 // 获取紧急程度
    void setUrgency(const QString &urgency); // 设置紧急程度

    QString importance() const;                    // 获取重要程度
    void setImportance(const QString &importance); // 设置重要程度

    QString status() const;                // 获取状态
    void setStatus(const QString &status); // 设置状态

    QDateTime createdAt() const;                   // 获取创建时间
    void setCreatedAt(const QDateTime &createdAt); // 设置创建时间

    QDateTime updatedAt() const;                   // 获取更新时间
    void setUpdatedAt(const QDateTime &updatedAt); // 设置更新时间

    bool synced() const;         // 获取是否已同步
    void setSynced(bool synced); // 设置是否已同步

  signals:
    void idChanged();            // ID改变信号
    void titleChanged();         // 标题改变信号
    void descriptionChanged();   // 描述改变信号
    void categoryChanged();      // 分类改变信号
    void urgencyChanged();       // 紧急程度改变信号
    void importanceChanged();    // 重要程度改变信号
    void statusChanged();        // 状态改变信号
    void createdAtChanged();     // 创建时间改变信号
    void updatedAtChanged();     // 更新时间改变信号
    void syncedChanged();        // 同步状态改变信号

  private:
    QString m_id;                // 任务ID
    QString m_title;             // 任务标题
    QString m_description;       // 任务描述
    QString m_category;          // 任务分类
    QString m_urgency;           // 任务紧急程度
    QString m_importance;        // 任务重要程度
    QString m_status;            // 任务状态
    QDateTime m_createdAt;       // 任务创建时间
    QDateTime m_updatedAt;       // 任务更新时间
    bool m_synced;               // 任务是否已同步
};

#endif // TODOITEM_H
