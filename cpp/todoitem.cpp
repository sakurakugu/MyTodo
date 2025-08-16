#include "todoItem.h"

TodoItem::TodoItem(QObject *parent)
    : QObject(parent)
    , m_synced(false) {
}

TodoItem::TodoItem(const QString &id, const QString &title, const QString &description,
                 const QString &category, const QString &urgency, const QString &importance,
                 const QString &status, const QDateTime &createdAt,
                 const QDateTime &updatedAt, bool synced, QObject *parent)
    : QObject(parent)
    , m_id(id)
    , m_title(title)
    , m_description(description)
    , m_category(category)
    , m_urgency(urgency)
    , m_importance(importance)
    , m_status(status)
    , m_createdAt(createdAt)
    , m_updatedAt(updatedAt)
    , m_synced(synced) {
}

QString TodoItem::id() const {
    return m_id;
}

void TodoItem::setId(const QString &id) {
    if (m_id == id)
        return;

    m_id = id;
    emit idChanged();
}

QString TodoItem::title() const {
    return m_title;
}

void TodoItem::setTitle(const QString &title) {
    if (m_title == title)
        return;

    m_title = title;
    emit titleChanged();
}

QString TodoItem::description() const {
    return m_description;
}

void TodoItem::setDescription(const QString &description) {
    if (m_description == description)
        return;

    m_description = description;
    emit descriptionChanged();
}

QString TodoItem::category() const {
    return m_category;
}

void TodoItem::setCategory(const QString &category) {
    if (m_category == category)
        return;

    m_category = category;
    emit categoryChanged();
}

QString TodoItem::urgency() const {
    return m_urgency;
}

void TodoItem::setUrgency(const QString &urgency) {
    if (m_urgency == urgency)
        return;

    m_urgency = urgency;
    emit urgencyChanged();
}

QString TodoItem::importance() const {
    return m_importance;
}

void TodoItem::setImportance(const QString &importance) {
    if (m_importance == importance)
        return;

    m_importance = importance;
    emit importanceChanged();
}

QString TodoItem::status() const {
    return m_status;
}

void TodoItem::setStatus(const QString &status) {
    if (m_status == status)
        return;

    m_status = status;
    emit statusChanged();
}

QDateTime TodoItem::createdAt() const {
    return m_createdAt;
}

void TodoItem::setCreatedAt(const QDateTime &createdAt) {
    if (m_createdAt == createdAt)
        return;

    m_createdAt = createdAt;
    emit createdAtChanged();
}

QDateTime TodoItem::updatedAt() const {
    return m_updatedAt;
}

void TodoItem::setUpdatedAt(const QDateTime &updatedAt) {
    if (m_updatedAt == updatedAt)
        return;

    m_updatedAt = updatedAt;
    emit updatedAtChanged();
}

bool TodoItem::synced() const {
    return m_synced;
}

void TodoItem::setSynced(bool synced) {
    if (m_synced == synced)
        return;

    m_synced = synced;
    emit syncedChanged();
}
