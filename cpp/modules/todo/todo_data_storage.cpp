/**
 * @file todo_data_storage.cpp
 * @brief TodoDataStorage类的实现文件
 *
 * 该文件实现了TodoDataStorage类的所有方法，负责待办事项的本地存储和文件导入导出功能。
 *
 * @author Sakurakugu
 * @date 2025-08-25 00:54:11(UTC+8) 周一
 * @change 2025-10-06 02:13:23(UTC+8) 周一
 */

#include "todo_data_storage.h"
#include "todo_item.h"
#include "utility.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QTimeZone>
#include <QUuid>
#include <QVariant>
#include <algorithm>

TodoDataStorage::TodoDataStorage(QObject *parent) //
    : BaseDataStorage("todos", parent)            // 调用基类构造函数
{
    初始化(); // 在子类构造完成后初始化数据表
}

TodoDataStorage::~TodoDataStorage() {}

/**
 * @brief 加载待办事项
 * @param todos 待办事项容器引用
 * @return 加载是否成功
 */
bool TodoDataStorage::加载待办(TodoList &todos) {
    bool success = true;

    try {
        // 清除当前数据
        todos.clear();

        // 从数据库加载数据
        QSqlDatabase db;
        if (!m_database.getDatabase(db))
            return false;

        QSqlQuery query(db);
        const QString queryString =
            "SELECT id, uuid, user_uuid, title, description, category, important, deadline, recurrence_interval, "
            "recurrence_count, recurrence_start_date, is_completed, completed_at, is_trashed, trashed_at, created_at, "
            "updated_at, synced FROM todos ORDER BY id";

        if (!query.exec(queryString)) {
            qCritical() << "加载待办事项查询失败:" << query.lastError().text();
            return false;
        }

        while (query.next()) {
            int id = query.value("id").toInt();
            QUuid uuid = QUuid::fromString(query.value("uuid").toString());
            QUuid userUuid = QUuid::fromString(query.value("user_uuid").toString());
            QString title = query.value("title").toString();
            QString description = query.value("description").toString();
            QString category = query.value("category").toString();
            bool important = query.value("important").toBool();
            QDateTime deadline = Utility::timestampToDateTime(query.value("deadline"));
            int recurrenceInterval = query.value("recurrence_interval").toInt();
            int recurrenceCount = query.value("recurrence_count").toInt();
            QDate recurrenceStartDate = QDate::fromString(query.value("recurrence_start_date").toString(), Qt::ISODate);
            bool isCompleted = query.value("is_completed").toBool();
            QDateTime completedAt = Utility::timestampToDateTime(query.value("completed_at"));
            bool isTrashed = query.value("is_trashed").toBool();
            QDateTime trashedAt = Utility::timestampToDateTime(query.value("trashed_at"));
            QDateTime createdAt = Utility::timestampToDateTime(query.value("created_at"));
            QDateTime updatedAt = Utility::timestampToDateTime(query.value("updated_at"));
            int synced = query.value("synced").toInt();

            auto item = std::make_unique<TodoItem>( //
                id,                                 // 唯一标识符
                uuid,                               // 唯一标识符（UUID）
                userUuid,                           // 用户UUID
                title,                              // 待办事项标题
                description,                        // 待办事项详细描述
                category,                           // 待办事项分类
                important,                          // 重要程度
                deadline,                           // 截止日期
                recurrenceInterval,                 // 重复间隔
                recurrenceCount,                    // 重复次数
                recurrenceStartDate,                // 重复开始日期
                isCompleted,                        // 是否已完成
                completedAt,                        // 完成时间
                isTrashed,                          // 是否已回收
                trashedAt,                          // 回收时间
                createdAt,                          // 创建时间
                updatedAt,                          // 更新时间
                synced                              // 是否已同步
            );
            todos.push_back(std::move(item));
        }

        qDebug() << "成功从数据库加载" << todos.size() << "个待办事项";
    } catch (const std::exception &e) {
        qCritical() << "加载本地存储时发生异常:" << e.what();
        success = false;
    } catch (...) {
        qCritical() << "加载本地存储时发生未知异常";
        success = false;
    }

    return success;
}

/**
 * @brief 添加新的待办事项
 * @param todos 待办事项容器引用
 * @param title 标题
 * @param description 描述
 * @param category 分类
 * @param important 是否重要
 * @param deadline 截止日期
 * @param recurrenceInterval 重复间隔
 * @param recurrenceCount 重复次数
 * @param recurrenceStartDate 重复开始日期
 * @return 添加是否成功
 */
bool TodoDataStorage::新增待办(TodoList &todos, const QString &title, const QString &description,
                               const QString &category, bool important, const QDateTime &deadline,
                               int recurrenceInterval, int recurrenceCount, const QDate &recurrenceStartDate,
                               QUuid userUuid) {
    QSqlDatabase db;
    if (!m_database.getDatabase(db))
        return false;
    QDateTime now = QDateTime::currentDateTimeUtc();
    QUuid newUuid = QUuid::createUuid();
    QDateTime nullTime; // 代表空

    // 创建新的待办事项
    auto newTodo = std::make_unique<TodoItem>( //
        -1,                                    // 占位id，插入后再获取
        newUuid,                               // uuid
        userUuid,                              // userUuid
        title,                                 // title
        description,                           // description
        category,                              // category
        important,                             // important
        deadline,                              // deadline
        recurrenceInterval,                    // recurrenceInterval
        recurrenceCount,                       // recurrenceCount
        recurrenceStartDate,                   // recurrenceStartDate
        false,                                 // isCompleted
        nullTime,                              // completedAt
        false,                                 // isTrashed
        nullTime,                              // trashedAt
        now,                                   // createdAt
        now,                                   // updatedAt
        1                                      // synced
    );

    // 插入到数据库
    QSqlQuery insertQuery(db);
    const QString insertString =
        "INSERT INTO todos (uuid, user_uuid, title, description, category, important, deadline, "
        "recurrence_interval, recurrence_count, recurrence_start_date, is_completed, completed_at, "
        "is_trashed, trashed_at, created_at, updated_at, synced) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
    insertQuery.prepare(insertString);
    insertQuery.addBindValue(newTodo->uuid().toString());
    insertQuery.addBindValue(newTodo->userUuid().toString());
    insertQuery.addBindValue(newTodo->title());
    insertQuery.addBindValue(newTodo->description());
    insertQuery.addBindValue(newTodo->category());
    insertQuery.addBindValue(newTodo->important());
    insertQuery.addBindValue(newTodo->deadline().isValid() ? QVariant(newTodo->deadline().toUTC().toMSecsSinceEpoch())
                                                           : QVariant());
    insertQuery.addBindValue(newTodo->recurrenceInterval());
    insertQuery.addBindValue(newTodo->recurrenceCount());
    insertQuery.addBindValue(newTodo->recurrenceStartDate().toString(Qt::ISODate));
    insertQuery.addBindValue(newTodo->isCompleted());
    insertQuery.addBindValue(
        newTodo->completedAt().isValid() ? QVariant(newTodo->completedAt().toUTC().toMSecsSinceEpoch()) : QVariant());
    insertQuery.addBindValue(newTodo->isTrashed());
    insertQuery.addBindValue(newTodo->trashedAt().isValid() ? QVariant(newTodo->trashedAt().toUTC().toMSecsSinceEpoch())
                                                            : QVariant());
    insertQuery.addBindValue(newTodo->createdAt().toUTC().toMSecsSinceEpoch());
    insertQuery.addBindValue(newTodo->updatedAt().toUTC().toMSecsSinceEpoch());
    insertQuery.addBindValue(newTodo->synced());
    if (!insertQuery.exec()) {
        qCritical() << "插入待办事项到数据库失败:" << insertQuery.lastError().text();
        return false;
    }

    // 获取自增ID
    QSqlQuery idQuery(db);
    int newId = -1;
    if (idQuery.exec("SELECT last_insert_rowid()") && idQuery.next())
        newId = idQuery.value(0).toInt();
    if (newId <= 0) {
        qWarning() << "获取自增ID失败，使用临时ID -1";
    }
    newTodo->setId(newId);
    qDebug() << "成功添加待办事项到数据库，ID:" << newId;
    todos.push_back(std::move(newTodo));
    return true;
}

/**
 * @brief 添加新的待办事项
 * @param todos 待办事项容器引用
 * @param item 待办事项智能指针(已经改好的)
 */
bool TodoDataStorage::新增待办(TodoList &todos, std::unique_ptr<TodoItem> item) {
    QSqlDatabase db;
    if (!m_database.getDatabase(db))
        return false;

    // 插入到数据库
    QSqlQuery insertQuery(db);
    const QString insertString =
        "INSERT INTO todos (uuid, user_uuid, title, description, category, important, deadline, "
        "recurrence_interval, recurrence_count, recurrence_start_date, is_completed, completed_at, "
        "is_trashed, trashed_at, created_at, updated_at, synced) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
    insertQuery.prepare(insertString);
    insertQuery.addBindValue(item->uuid().toString());
    insertQuery.addBindValue(item->userUuid().toString());
    insertQuery.addBindValue(item->title());
    insertQuery.addBindValue(item->description());
    insertQuery.addBindValue(item->category());
    insertQuery.addBindValue(item->important());
    insertQuery.addBindValue(item->deadline().isValid() ? QVariant(item->deadline().toUTC().toMSecsSinceEpoch())
                                                        : QVariant());
    insertQuery.addBindValue(item->recurrenceInterval());
    insertQuery.addBindValue(item->recurrenceCount());
    insertQuery.addBindValue(item->recurrenceStartDate().toString(Qt::ISODate));
    insertQuery.addBindValue(item->isCompleted());
    insertQuery.addBindValue(item->completedAt().isValid() ? QVariant(item->completedAt().toUTC().toMSecsSinceEpoch())
                                                           : QVariant());
    insertQuery.addBindValue(item->isTrashed());
    insertQuery.addBindValue(item->trashedAt().isValid() ? QVariant(item->trashedAt().toUTC().toMSecsSinceEpoch())
                                                         : QVariant());
    insertQuery.addBindValue(item->createdAt().toUTC().toMSecsSinceEpoch());
    insertQuery.addBindValue(item->updatedAt().toUTC().toMSecsSinceEpoch());
    insertQuery.addBindValue(item->synced());
    if (!insertQuery.exec()) {
        qCritical() << "插入待办事项到数据库失败:" << insertQuery.lastError().text();
        return false;
    }
    int newId = 获取最后插入行ID(db);
    item->setId(newId);
    todos.push_back(std::move(item));
    qDebug() << "成功添加待办事项到数据库，ID:" << newId;
    return true;
}

/**
 * @brief 更新待办事项
 * @param todos 待办事项容器引用
 * @param id 待办事项ID
 * @param todoData 待办事项数据映射
 * @return 更新是否成功
 */
bool TodoDataStorage::更新待办(TodoList &todos, const QUuid &uuid, const QVariantMap &todoData) {
    QSqlDatabase db;
    if (!m_database.getDatabase(db))
        return false;
    auto it = std::find_if(todos.begin(), todos.end(),
                           [uuid](const std::unique_ptr<TodoItem> &item) { return item->uuid() == uuid; });
    if (it == todos.end()) {
        qWarning() << "未找到待办事项，UUID:" << uuid.toString();
        return false;
    }

    // 更新数据库中的待办事项
    QSqlQuery updateQuery(db);
    // 记录要更新的字段 [原本类型，字段名]
    QList<QPair<QMetaType::Type, QString>> fields;
    QString order = "UPDATE todos SET ";
    auto appendField = [&](const QString &col, QMetaType::Type t) {
        order += col + " = ?, ";
        fields << qMakePair(t, col);
    };
    if (todoData.contains("title")) {
        appendField("title", QMetaType::QString);
        (*it)->setTitle(todoData["title"].toString());
    }
    if (todoData.contains("description")) {
        appendField("description", QMetaType::QString);
        (*it)->setDescription(todoData["description"].toString());
    }
    if (todoData.contains("category")) {
        appendField("category", QMetaType::QString);
        (*it)->setCategory(todoData["category"].toString());
    }
    if (todoData.contains("important")) {
        appendField("important", QMetaType::Bool);
        (*it)->setImportant(todoData["important"].toBool());
    }
    if (todoData.contains("deadline")) {
        appendField("deadline", QMetaType::QDateTime);
        QDateTime dt = QDateTime::fromString(todoData["deadline"].toString(), Qt::ISODate);
        (*it)->setDeadline(dt);
    }
    if (todoData.contains("recurrence_interval")) {
        appendField("recurrence_interval", QMetaType::Int);
        (*it)->setRecurrenceInterval(todoData["recurrence_interval"].toInt());
    }
    if (todoData.contains("recurrence_count")) {
        appendField("recurrence_count", QMetaType::Int);
        (*it)->setRecurrenceCount(todoData["recurrence_count"].toInt());
    }
    if (todoData.contains("recurrence_start_date")) {
        appendField("recurrence_start_date", QMetaType::QDate);
        (*it)->setRecurrenceStartDate(QDate::fromString(todoData["recurrence_start_date"].toString(), Qt::ISODate));
    }
    if (todoData.contains("is_completed")) {
        appendField("is_completed", QMetaType::Bool);
        (*it)->setIsCompleted(todoData["is_completed"].toBool());
    }
    if (todoData.contains("completed_at")) {
        appendField("completed_at", QMetaType::QDateTime);
        QDateTime dt = QDateTime::fromString(todoData["completed_at"].toString(), Qt::ISODate);
        (*it)->setCompletedAt(dt);
    }
    if (todoData.contains("is_trashed")) {
        appendField("is_trashed", QMetaType::Bool);
        (*it)->setIsTrashed(todoData["is_trashed"].toBool());
    }
    if (todoData.contains("trashed_at")) {
        appendField("trashed_at", QMetaType::QDateTime);
        QDateTime dt = QDateTime::fromString(todoData["trashed_at"].toString(), Qt::ISODate);
        (*it)->setTrashedAt(dt);
    }
    // 始终更新这两个字段
    order += "updated_at = ?, synced = ? WHERE uuid = ?";
    updateQuery.prepare(order);
    for (const auto &[type, name] : fields) {
        if (type == QMetaType::QDateTime) {
            QDateTime dt = QDateTime::fromString(todoData[name].toString(), Qt::ISODate);
            updateQuery.addBindValue(dt.isValid() ? QVariant(dt.toUTC().toMSecsSinceEpoch()) : QVariant());
        } else if (type == QMetaType::QDate) {
            QDate d = QDate::fromString(todoData[name].toString(), Qt::ISODate);
            updateQuery.addBindValue(d.isValid() ? d.toString(Qt::ISODate) : QString());
        } else {
            updateQuery.addBindValue(todoData[name]);
        }
    }
    QDateTime now = QDateTime::currentDateTimeUtc();
    updateQuery.addBindValue(now.toMSecsSinceEpoch());
    updateQuery.addBindValue(((*it)->synced() != 1) ? 2 : 1);
    updateQuery.addBindValue(uuid);
    if (!updateQuery.exec()) {
        qCritical() << "更新待办事项到数据库失败:" << updateQuery.lastError().text();
        return false;
    }
    if (updateQuery.numRowsAffected() == 0) {
        qWarning() << "未找到UUID为" << uuid << "的待办事项";
        return false;
    }
    (*it)->setUpdatedAt(now);
    (*it)->setSynced(2);
    qDebug() << "成功更新待办事项，UUID:" << uuid;
    return true;
}

/**
 * @brief 更新待办事项
 * @param todos 待办事项容器引用
 * @param item 待办事项对象引用(已经改好的)
 * @return 更新是否成功
 */
bool TodoDataStorage::更新待办([[maybe_unused]] TodoList &todos, TodoItem &item) {
    QSqlDatabase db;
    if (!m_database.getDatabase(db))
        return false;
    // 更新数据库中的待办事项
    QSqlQuery updateQuery(db);
    const QString updateString =
        "UPDATE todos SET title = ?, description = ?, category = ?, important = ?, deadline = ?, "
        "recurrence_interval = ?, recurrence_count = ?, recurrence_start_date = ?, is_completed = ?, "
        "completed_at = ?, is_trashed = ?, trashed_at = ?, updated_at = ?, synced = ? WHERE uuid = ?";
    updateQuery.prepare(updateString);
    updateQuery.addBindValue(item.title());
    updateQuery.addBindValue(item.description());
    updateQuery.addBindValue(item.category());
    updateQuery.addBindValue(item.important());
    updateQuery.addBindValue(item.deadline().isValid() ? QVariant(item.deadline().toUTC().toMSecsSinceEpoch())
                                                       : QVariant());
    updateQuery.addBindValue(item.recurrenceInterval());
    updateQuery.addBindValue(item.recurrenceCount());
    updateQuery.addBindValue(item.recurrenceStartDate().toString(Qt::ISODate));
    updateQuery.addBindValue(item.isCompleted());
    updateQuery.addBindValue(item.completedAt().isValid() ? QVariant(item.completedAt().toUTC().toMSecsSinceEpoch())
                                                          : QVariant());
    updateQuery.addBindValue(item.isTrashed());
    updateQuery.addBindValue(item.trashedAt().isValid() ? QVariant(item.trashedAt().toUTC().toMSecsSinceEpoch())
                                                        : QVariant());
    updateQuery.addBindValue(item.updatedAt().toUTC().toMSecsSinceEpoch());
    updateQuery.addBindValue(item.synced());
    updateQuery.addBindValue(item.uuid());
    if (!updateQuery.exec()) {
        qCritical() << "更新待办事项到数据库失败:" << updateQuery.lastError().text();
        return false;
    }
    if (updateQuery.numRowsAffected() == 0) {
        qWarning() << "未找到UUID为" << item.uuid() << "的待办事项";
        return false;
    }
    qDebug() << "成功更新待办事项，UUID:" << item.uuid();
    return true;
}

/**
 * @brief 回收待办事项
 * @param todos 待办事项容器引用
 * @param uuid 待办事项UUID
 * @return 回收是否成功
 */
bool TodoDataStorage::回收待办(TodoList &todos, const QUuid &uuid) {
    QVariantMap todoData;
    todoData["is_trashed"] = true;
    todoData["trashed_at"] = QDateTime::currentDateTimeUtc();
    return 更新待办(todos, uuid, todoData);
}

/**
 * @brief 软删除待办事项
 * @param todos 软删除待办容器引用
 * @param uuid 待办事项UUID
 * @return 回收是否成功
 */
bool TodoDataStorage::软删除待办([[maybe_unused]] TodoList &todos, const QUuid &uuid) {
    QSqlDatabase db;
    if (!m_database.getDatabase(db))
        return false;
    QSqlQuery updateQuery(db);
    const QString updateString = "UPDATE todos SET synced = ? WHERE uuid = ?";
    updateQuery.prepare(updateString);
    updateQuery.addBindValue(3); // 已删除
    updateQuery.addBindValue(uuid);
    if (!updateQuery.exec()) {
        qCritical() << "软删除待办事项失败:" << updateQuery.lastError().text();
        return false;
    }
    if (updateQuery.numRowsAffected() == 0) {
        qWarning() << "未找到UUID为" << uuid << "的待办事项";
        return false;
    }
    qDebug() << "成功软删除待办事项，UUID:" << uuid;
    return true;
}

/**
 * @brief 永久删除所有待办事项
 * @param todos 待办事项容器引用
 */
bool TodoDataStorage::删除所有待办(TodoList &todos) {
    bool success = false;
    try {
        QSqlDatabase db;
        if (!m_database.getDatabase(db))
            return false;

        // 从数据库中永久删除所有待办事项
        QSqlQuery deleteQuery(db);
        const QString deleteString = "DELETE FROM todos";
        deleteQuery.prepare(deleteString);

        if (!deleteQuery.exec()) {
            qCritical() << "永久删除所有待办事项失败:" << deleteQuery.lastError().text();
            return false;
        }

        todos.clear(); // 清空内存中的待办事项列表

        qDebug() << "成功永久删除所有待办事项";
        return true;
    } catch (const std::exception &e) {
        qCritical() << "删除所有待办事项时发生异常:" << e.what();
        success = false;
    } catch (...) {
        qCritical() << "删除所有待办事项时发生未知异常";
        success = false;
    }

    return success;
}

/**
 * @brief 更新所有待办事项的用户UUID
 * @param todos 待办事项容器引用
 * @param newUserUuid 新的用户UUID
 */
bool TodoDataStorage::更新所有待办用户UUID(TodoList &todos, const QUuid &newUserUuid, int synced) {
    bool success = false;
    try {
        QSqlDatabase db;
        if (!m_database.getDatabase(db))
            return false;

        // 更新数据库中所有待办事项的用户UUID
        QSqlQuery updateQuery(db);
        const QString updateString = "UPDATE todos SET user_uuid = ?, synced = ?";
        updateQuery.prepare(updateString);
        updateQuery.addBindValue(newUserUuid.toString());
        updateQuery.addBindValue(synced); // synced

        if (!updateQuery.exec()) {
            qCritical() << "更新待办事项的用户UUID失败:" << updateQuery.lastError().text();
            return false;
        }

        // 更新内存中的待办事项列表
        for (auto &item : todos) {
            if (item) {
                item->setUserUuid(newUserUuid);
                item->setSynced(synced);
            }
        }

        qDebug() << "成功更新所有待办事项的用户UUID为" << newUserUuid.toString();
        return true;
    } catch (const std::exception &e) {
        qCritical() << "更新待办事项的用户UUID时发生异常:" << e.what();
        success = false;
    } catch (...) {
        qCritical() << "更新待办事项的用户UUID时发生未知异常";
        success = false;
    }
    return success;
}

/**
 * @brief 永久删除待办事项
 * @param todos 待办事项容器引用
 * @param id 待办事项ID
 */
bool TodoDataStorage::删除待办([[maybe_unused]] TodoList &todos, const QUuid &uuid) {
    bool success = false;
    try {
        QSqlDatabase db;
        if (!m_database.getDatabase(db))
            return false;

        // 从数据库中永久删除待办事项
        QSqlQuery deleteQuery(db);
        const QString deleteString = "DELETE FROM todos WHERE uuid = ?";
        deleteQuery.prepare(deleteString);
        deleteQuery.addBindValue(uuid);
        if (!deleteQuery.exec()) {
            qCritical() << "永久删除待办事项失败:" << deleteQuery.lastError().text();
            return false;
        }
        if (deleteQuery.numRowsAffected() == 0) {
            qWarning() << "未找到UUID为" << uuid << "的待办事项，无法删除";
            return false;
        }
        // 要更新qml视图，所以不在这里删除
        // todos.erase(std::remove_if(todos.begin(), todos.end(),
        //                            [&](const std::unique_ptr<TodoItem> &p) { return p && p->uuid() == uuid; }),
        //             todos.end());
        qDebug() << "成功永久删除待办事项，UUID:" << uuid;
        return true;
    } catch (const std::exception &e) {
        qCritical() << "删除待办事项时发生异常:" << e.what();
        success = false;
    } catch (...) {
        qCritical() << "删除待办事项时发生未知异常";
        success = false;
    }
    return success;
}

/**
 * @brief 导入待办事项从JSON
 * @param todos 待办事项容器引用
 * @param todosArray JSON待办事项数组
 * @param source 来源（服务器或本地备份）
 * @param resolution 冲突解决策略
 */
bool TodoDataStorage::导入待办事项从JSON(TodoList &todos, const QJsonArray &todosArray, //
                                         ImportSource source, 解决冲突方案 resolution) {
    bool success = true;
    QSqlDatabase db;
    if (!m_database.getDatabase(db))
        return false;
    // 建立内存索引
    QHash<QString, TodoItem *> uuidIndex;
    for (auto &it : todos) {
        if (it)
            uuidIndex.insert(it->uuid().toString(QUuid::WithoutBraces), it.get());
    }

    if (!db.transaction()) {
        qCritical() << "无法开启事务以导入待办事项:" << db.lastError().text();
        return false;
    }

    int insertCount = 0;
    int updateCount = 0;
    int skipCount = 0;

    try {
        for (const QJsonValue &value : todosArray) {
            if (!value.isObject()) {
                qWarning() << "跳过无效待办（非对象）";
                ++skipCount;
                continue;
            }
            QJsonObject obj = value.toObject();
            if (!obj.contains("title") || !obj.contains("user_uuid")) {
                qWarning() << "跳过无效待办（缺字段）";
                ++skipCount;
                continue;
            }
            QUuid userUuid = QUuid::fromString(obj["user_uuid"].toString());
            if (userUuid.isNull()) {
                qWarning() << "跳过无效待办（user_uuid 无效）";
                ++skipCount;
                continue;
            }
            QUuid uuid = obj.contains("uuid") ? QUuid::fromString(obj["uuid"].toString()) : QUuid::createUuid();
            if (uuid.isNull())
                uuid = QUuid::createUuid();
            QString title = obj["title"].toString();
            QString description = obj["description"].toString();
            QString category = obj["category"].toString();
            bool important = obj["important"].toBool();
            QDateTime deadline = Utility::fromIsoString(obj["deadline"].toString());
            int recurrenceInterval = obj["recurrence_interval"].toInt();
            int recurrenceCount = obj["recurrence_count"].toInt();
            QDate recurrenceStartDate = QDate::fromString(obj["recurrence_start_date"].toString(), Qt::ISODate);
            bool isCompleted = obj["is_completed"].toBool();
            QDateTime completedAt = Utility::fromIsoString(obj["completed_at"].toString());
            bool isTrashed = obj["is_trashed"].toBool();
            QDateTime trashedAt = Utility::fromIsoString(obj["trashed_at"].toString());
            QDateTime createdAt = Utility::fromIsoString(obj["created_at"].toString());
            if (!createdAt.isValid())
                createdAt = QDateTime::currentDateTimeUtc();
            QDateTime updatedAt = Utility::fromIsoString(obj["updated_at"].toString());
            if (!updatedAt.isValid())
                updatedAt = createdAt;

            // 构造临时 incoming 对象（不立即放入列表）
            TodoItem incoming(                         //
                -1,                                    //
                uuid,                                  //
                userUuid,                              //
                title,                                 //
                description,                           //
                category,                              //
                important,                             //
                deadline,                              //
                recurrenceInterval,                    //
                recurrenceCount,                       //
                recurrenceStartDate,                   //
                isCompleted,                           //
                completedAt,                           //
                isTrashed,                             //
                trashedAt,                             //
                createdAt,                             //
                updatedAt,                             //
                source == ImportSource::Server ? 0 : 1 //
            );

            // 查找现有
            TodoItem *existing = uuidIndex.value(uuid.toString(QUuid::WithoutBraces), nullptr);

            解决冲突方案 action = 评估冲突(existing, incoming, resolution);
            if (action == 解决冲突方案::Skip) {
                ++skipCount;
                continue;
            }

            if (action == 解决冲突方案::Insert) {
                // 不能直接复制（TodoItem禁止拷贝），显式构造
                auto newItem = std::make_unique<TodoItem>( //
                    incoming.id(),                         //
                    incoming.uuid(),                       //
                    incoming.userUuid(),                   //
                    incoming.title(),                      //
                    incoming.description(),                //
                    incoming.category(),                   //
                    incoming.important(),                  //
                    incoming.deadline(),                   //
                    incoming.recurrenceInterval(),         //
                    incoming.recurrenceCount(),            //
                    incoming.recurrenceStartDate(),        //
                    incoming.isCompleted(),                //
                    incoming.completedAt(),                //
                    incoming.isTrashed(),                  //
                    incoming.trashedAt(),                  //
                    incoming.createdAt(),                  //
                    incoming.updatedAt(),                  //
                    incoming.synced()                      //
                );
                auto &ref = *newItem;
                QSqlQuery ins(db);
                ins.prepare("INSERT INTO todos (uuid, user_uuid, title, description, category, important, deadline, "
                            "recurrence_interval, recurrence_count, recurrence_start_date, is_completed, completed_at, "
                            "is_trashed, trashed_at, created_at, updated_at, synced) VALUES "
                            "(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
                ins.addBindValue(ref.uuid().toString());
                ins.addBindValue(ref.userUuid().toString());
                ins.addBindValue(ref.title());
                ins.addBindValue(ref.description());
                ins.addBindValue(ref.category());
                ins.addBindValue(ref.important());
                ins.addBindValue(ref.deadline().isValid() ? QVariant(ref.deadline().toUTC().toMSecsSinceEpoch())
                                                          : QVariant());
                ins.addBindValue(ref.recurrenceInterval());
                ins.addBindValue(ref.recurrenceCount());
                ins.addBindValue(ref.recurrenceStartDate().toString(Qt::ISODate));
                ins.addBindValue(ref.isCompleted());
                ins.addBindValue(ref.completedAt().isValid() ? QVariant(ref.completedAt().toUTC().toMSecsSinceEpoch())
                                                             : QVariant());
                ins.addBindValue(ref.isTrashed());
                ins.addBindValue(ref.trashedAt().isValid() ? QVariant(ref.trashedAt().toUTC().toMSecsSinceEpoch())
                                                           : QVariant());
                ins.addBindValue(ref.createdAt().toUTC().toMSecsSinceEpoch());
                ins.addBindValue(ref.updatedAt().toUTC().toMSecsSinceEpoch());
                ins.addBindValue(ref.synced());
                if (!ins.exec()) {
                    qCritical() << "插入导入待办失败:" << ins.lastError().text();
                    success = false;
                    break;
                }
                QSqlQuery idQ(db);
                if (idQ.exec("SELECT last_insert_rowid()") && idQ.next())
                    ref.setId(idQ.value(0).toInt());
                uuidIndex.insert(uuid.toString(QUuid::WithoutBraces), newItem.get());
                todos.push_back(std::move(newItem));
                ++insertCount;
                continue;
            }
            if (action == 解决冲突方案::Overwrite && existing) {
                QSqlQuery up(db);
                up.prepare(
                    "UPDATE todos SET user_uuid=?, title=?, description=?, category=?, important=?, deadline=?, "
                    "recurrence_interval=?, recurrence_count=?, recurrence_start_date=?, is_completed=?, "
                    "completed_at=?, is_trashed=?, trashed_at=?, created_at=?, updated_at=?, synced=? WHERE uuid=?");
                up.addBindValue(userUuid.toString());
                up.addBindValue(title);
                up.addBindValue(description);
                up.addBindValue(category);
                up.addBindValue(important);
                up.addBindValue(deadline.isValid() ? QVariant(deadline.toUTC().toMSecsSinceEpoch()) : QVariant());
                up.addBindValue(recurrenceInterval);
                up.addBindValue(recurrenceCount);
                up.addBindValue(recurrenceStartDate.toString(Qt::ISODate));
                up.addBindValue(isCompleted);
                up.addBindValue(completedAt.isValid() ? QVariant(completedAt.toUTC().toMSecsSinceEpoch()) : QVariant());
                up.addBindValue(isTrashed);
                up.addBindValue(trashedAt.isValid() ? QVariant(trashedAt.toUTC().toMSecsSinceEpoch()) : QVariant());
                up.addBindValue(createdAt.toUTC().toMSecsSinceEpoch());
                up.addBindValue(updatedAt.toUTC().toMSecsSinceEpoch());
                up.addBindValue(source == ImportSource::Server ? 0 : (existing->synced() == 1 ? 1 : 2));
                up.addBindValue(existing->uuid().toString());
                if (!up.exec()) {
                    qCritical() << "更新导入待办失败:" << up.lastError().text();
                    success = false;
                    break;
                }
                // 更新内存
                existing->setTitle(title);
                existing->setUserUuid(userUuid);
                existing->setDescription(description);
                existing->setCategory(category);
                existing->setImportant(important);
                existing->setDeadline(deadline);
                existing->setRecurrenceInterval(recurrenceInterval);
                existing->setRecurrenceCount(recurrenceCount);
                existing->setRecurrenceStartDate(recurrenceStartDate);
                existing->setIsCompleted(isCompleted);
                existing->setCompletedAt(completedAt);
                existing->setIsTrashed(isTrashed);
                existing->setTrashedAt(trashedAt);
                existing->setCreatedAt(createdAt);
                existing->setUpdatedAt(updatedAt);
                existing->setSynced(source == ImportSource::Server ? 0 : 2);
                ++updateCount;
                continue;
            }
        }
        if (success) {
            if (!db.commit()) {
                qCritical() << "提交事务失败:" << db.lastError().text();
                db.rollback();
                success = false;
            } else {
                qDebug() << "导入完成 - 新增:" << insertCount << ", 更新:" << updateCount << ", 跳过:" << skipCount;
            }
        } else {
            db.rollback();
        }
    } catch (const std::exception &e) {
        qCritical() << "导入待办时发生异常:" << e.what();
        db.rollback();
        success = false;
    } catch (...) {
        qCritical() << "导入待办时发生未知异常";
        db.rollback();
        success = false;
    }
    return success;
}

// 私有辅助方法实现

/**
 * @brief 根据排序类型和顺序生成SQL的ORDER BY子句
 * @param sortType 排序类型（0-创建时间，1-截止日期，2-重要性，3-标题，4-修改时间，5-完成时间）
 * @param descending 是否降序
 * @return SQL的ORDER BY子句
 */
QString TodoDataStorage::构建SQL排序语句(int sortType, bool descending) {
    QString order;
    switch (sortType) {
    case 0: // 创建时间
        order = "ORDER BY created_at";
        break;
    case 1: // 截止日期
        // NULL 截止日期 放最后
        order = "ORDER BY (deadline IS NULL) ASC, deadline";
        break;
    case 2: // 重要度 + 创建时间
        order = "ORDER BY important DESC, created_at DESC";
        if (descending)
            order = "ORDER BY important ASC, created_at DESC"; // 特殊翻转
        return order;
    case 3: // 标题
        order = "ORDER BY title COLLATE NOCASE";
        break;
    case 5: // 完成时间
        // NULL 完成时间（未完成的任务）放最后
        order = "ORDER BY (completed_at IS NULL) ASC, completed_at";
        break;
    case 4: // 更新时间（默认）
    default:
        order = "ORDER BY updated_at";
        break;
    }
    order += descending ? " DESC" : " ASC";
    return order;
}

/**
 * @brief 查询待办ID列表
 * @param opt 查询选项
 * @return 符合条件的待办ID列表
 */
QList<int> TodoDataStorage::查询待办ID列表(const QueryOptions &opt) {
    QList<int> indexs;
    QSqlDatabase db;
    if (!m_database.getDatabase(db))
        return indexs;
    QString sql = "SELECT id FROM todos WHERE 1=1";

    // 分类
    if (!opt.category.isEmpty())
        sql += " AND category = :category";
    if (opt.statusFilter == "todo")
        sql += " AND is_trashed = 0 AND is_completed = 0";
    else if (opt.statusFilter == "done")
        sql += " AND is_trashed = 0 AND is_completed = 1";
    else if (opt.statusFilter == "recycle")
        sql += " AND is_trashed = 1";
    else if (opt.statusFilter == "all") {
        // 空
    } else
        sql += " AND is_trashed = 0"; // 默认排除已回收

    if (!opt.searchText.isEmpty())
        sql += " AND (title LIKE :search OR description LIKE :search OR category LIKE :search)";
    if (opt.dateFilterEnabled) {
        if (opt.dateStart.isValid()) {
            QDateTime startDt(opt.dateStart, QTime(0, 0), QTimeZone::UTC);
            qint64 startMs = startDt.toMSecsSinceEpoch();
            sql += " AND deadline >= " + QString::number(startMs);
        }
        if (opt.dateEnd.isValid()) {
            QDate nextDay = opt.dateEnd.addDays(1);
            QDateTime endDt(nextDay, QTime(0, 0), QTimeZone::UTC); // 第二天开始的时间
            qint64 endMs = endDt.toMSecsSinceEpoch();
            sql += " AND deadline < " + QString::number(endMs);
        }
    }

    sql += ' ' + 构建SQL排序语句(opt.sortType, opt.descending);

    // 分页
    if (opt.limit > 0) {
        sql += QString(" LIMIT %1").arg(opt.limit);
        if (opt.offset > 0)
            sql += QString(" OFFSET %1").arg(opt.offset);
    }

    QSqlQuery query(db);
    query.prepare(sql);
    if (!opt.category.isEmpty())
        query.bindValue(":category", opt.category);
    if (!opt.searchText.isEmpty())
        query.bindValue(":search", '%' + opt.searchText + '%');
    if (!query.exec()) {
        qCritical() << "查询待办事项ID失败:" << query.lastError().text();
        return indexs;
    }
    while (query.next())
        indexs.append(query.value(0).toInt());
    return indexs;
}

/**
 * @brief 初始化TODO表
 * @return 初始化是否成功
 */
bool TodoDataStorage::初始化数据表() {
    QSqlDatabase db;
    if (!m_database.getDatabase(db))
        return false;

    return 创建数据表();
}

bool TodoDataStorage::创建数据表() {
    const QString createTableQuery = R"(
        CREATE TABLE IF NOT EXISTS todos (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            uuid TEXT UNIQUE NOT NULL,
            user_uuid TEXT NOT NULL,
            title TEXT NOT NULL,
            description TEXT,
            category TEXT NOT NULL DEFAULT '未分类',
            important INTEGER NOT NULL DEFAULT 0,
            deadline INTEGER,
            recurrence_interval INTEGER NOT NULL DEFAULT 0,
            recurrence_count INTEGER NOT NULL DEFAULT 0,
            recurrence_start_date TEXT,
            is_completed INTEGER NOT NULL DEFAULT 0,
            completed_at INTEGER,
            is_trashed INTEGER NOT NULL DEFAULT 0,
            trashed_at INTEGER,
            created_at INTEGER NOT NULL,
            updated_at INTEGER NOT NULL,
            synced INTEGER NOT NULL DEFAULT 1
        )
    )";
    执行SQL查询(createTableQuery);

    const QStringList indexes = {"CREATE INDEX IF NOT EXISTS idx_todos_uuid ON todos(uuid)",
                                 "CREATE INDEX IF NOT EXISTS idx_todos_user_uuid ON todos(user_uuid)",
                                 "CREATE INDEX IF NOT EXISTS idx_todos_category ON todos(category)",
                                 "CREATE INDEX IF NOT EXISTS idx_todos_deadline ON todos(deadline)",
                                 "CREATE INDEX IF NOT EXISTS idx_todos_completed ON todos(is_completed)",
                                 "CREATE INDEX IF NOT EXISTS idx_todos_trashed ON todos(is_trashed)",
                                 "CREATE INDEX IF NOT EXISTS idx_todos_synced ON todos(synced)"};
    for (const QString &ix : indexes) {
        执行SQL查询(ix);
    }
    qDebug() << "todos表初始化成功";
    return true;
}

/**
 * @brief 导出待办数据到JSON对象
 * @param output 输出的JSON对象引用
 * @return 导出是否成功
 */
bool TodoDataStorage::导出到JSON(QJsonObject &output) {
    QSqlDatabase db;
    if (!m_database.getDatabase(db))
        return false;
    QSqlQuery query(db);
    const QString sql = "SELECT id, uuid, user_uuid, title, description, category, important, deadline, "
                        "recurrence_interval, recurrence_count, recurrence_start_date, is_completed, "
                        "completed_at, is_trashed, trashed_at, created_at, updated_at, synced FROM todos";
    if (!query.exec(sql)) {
        qWarning() << "查询待办数据失败:" << query.lastError().text();
        return false;
    }
    QJsonArray arr;
    while (query.next()) {
        QJsonObject obj;
        // TODO:id 是自增主键，不应该导出
        obj["id"] = query.value("id").toInt();
        obj["uuid"] = query.value("uuid").toString();
        obj["user_uuid"] = query.value("user_uuid").toString();
        obj["title"] = query.value("title").toString();
        QVariant desc = query.value("description");
        obj["description"] = desc.isNull() ? QJsonValue() : desc.toString();
        obj["category"] = query.value("category").toString();
        obj["important"] = query.value("important").toInt();
        obj["deadline"] = Utility::timestampToIsoJson(query.value("deadline"));
        obj["recurrence_interval"] = query.value("recurrence_interval").toInt();
        obj["recurrence_count"] = query.value("recurrence_count").toInt();
        QVariant rsd = query.value("recurrence_start_date");
        obj["recurrence_start_date"] = rsd.isNull() ? QJsonValue() : rsd.toString();
        obj["is_completed"] = query.value("is_completed").toInt();
        obj["completed_at"] = Utility::timestampToIsoJson(query.value("completed_at"));
        obj["is_trashed"] = query.value("is_trashed").toInt();
        obj["trashed_at"] = Utility::timestampToIsoJson(query.value("trashed_at"));
        obj["created_at"] = Utility::timestampToIsoJson(query.value("created_at"));
        obj["updated_at"] = Utility::timestampToIsoJson(query.value("updated_at"));
        obj["synced"] = query.value("synced").toInt();
        arr.append(obj);
    }
    output["todos"] = arr;
    return true;
}

bool TodoDataStorage::导入从JSON(const QJsonObject &input, bool replaceAll) {
    QSqlDatabase db;
    if (!m_database.getDatabase(db))
        return false;
    if (!input.contains("todos") || !input["todos"].isArray())
        return true; // 没有数据
    QSqlQuery q(db);
    if (replaceAll) {
        if (!q.exec("DELETE FROM todos")) {
            qWarning() << "清空待办表失败:" << q.lastError().text();
            return false;
        }
    }
    QJsonArray arr = input["todos"].toArray();
    auto bindIso = [&](QSqlQuery &qq, const QJsonObject &o, const char *k) {
        if (o.contains(k) && !o.value(k).isNull()) {
            QDateTime dt = QDateTime::fromString(o.value(k).toString(), Qt::ISODate);
            qq.addBindValue(dt.isValid() ? QVariant(dt.toUTC().toMSecsSinceEpoch()) : QVariant());
        } else {
            qq.addBindValue(QVariant());
        }
    };
    // TODO:id 是自增主键，不应该从JSON导入
    for (const auto &v : arr) {
        QJsonObject o = v.toObject();
        q.prepare("INSERT OR REPLACE INTO todos (id, uuid, user_uuid, title, description, category, "
                  "important, deadline, recurrence_interval, recurrence_count, recurrence_start_date, "
                  "is_completed, completed_at, is_trashed, trashed_at, created_at, updated_at, synced) "
                  "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
        q.addBindValue(o["id"].toVariant());
        q.addBindValue(o["uuid"].toString());
        q.addBindValue(o["user_uuid"].toString());
        q.addBindValue(o["title"].toString());
        q.addBindValue(o["description"].toVariant());
        q.addBindValue(o["category"].toString());
        q.addBindValue(o["important"].toInt());
        bindIso(q, o, "deadline");
        q.addBindValue(o["recurrence_interval"].toInt());
        q.addBindValue(o["recurrence_count"].toInt());
        q.addBindValue(o["recurrence_start_date"].toVariant());
        q.addBindValue(o["is_completed"].toInt());
        bindIso(q, o, "completed_at");
        q.addBindValue(o["is_trashed"].toInt());
        bindIso(q, o, "trashed_at");
        bindIso(q, o, "created_at");
        bindIso(q, o, "updated_at");
        q.addBindValue(o["synced"].toInt());
        if (!q.exec()) {
            qWarning() << "导入待办数据失败:" << q.lastError().text();
            return false;
        }
    }
    qInfo() << "成功导入" << arr.size() << "条待办记录";
    return true;
}
