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
        auto query = m_database.createQuery();
        query->prepare(
            "SELECT id, uuid, user_uuid, title, description, category, important, deadline, recurrence_interval, "
            "recurrence_count, recurrence_start_date, is_completed, completed_at, is_trashed, trashed_at, created_at, "
            "updated_at, synced FROM todos ORDER BY id");

        if (!query->exec()) {
            qCritical() << "加载待办事项查询失败:" << query->lastErrorQt();
            return false;
        }

        while (query->next()) {
            int id = sqlValueCast<int32_t>(query->value("id"));
            QUuid uuid = QUuid::fromString(sqlValueCast<std::string>(query->value("uuid")));
            QUuid userUuid = QUuid::fromString(sqlValueCast<std::string>(query->value("user_uuid")));
            QString title = QString::fromStdString(sqlValueCast<std::string>(query->value("title")));
            QString description = QString::fromStdString(sqlValueCast<std::string>(query->value("description")));
            QString category = QString::fromStdString(sqlValueCast<std::string>(query->value("category")));
            bool important = sqlValueCast<bool>(query->value("important"));
            QDateTime deadline = Utility::timestampToDateTime(sqlValueCast<int64_t>(query->value("deadline")));
            int recurrenceInterval = sqlValueCast<int32_t>(query->value("recurrence_interval"));
            int recurrenceCount = sqlValueCast<int32_t>(query->value("recurrence_count"));
            QDate recurrenceStartDate = QDate::fromString(
                QString::fromStdString(sqlValueCast<std::string>(query->value("recurrence_start_date"))), Qt::ISODate);
            bool isCompleted = sqlValueCast<bool>(query->value("is_completed"));
            QDateTime completedAt = Utility::timestampToDateTime(sqlValueCast<int64_t>(query->value("completed_at")));
            bool isTrashed = sqlValueCast<bool>(query->value("is_trashed"));
            QDateTime trashedAt = Utility::timestampToDateTime(sqlValueCast<int64_t>(query->value("trashed_at")));
            QDateTime createdAt = Utility::timestampToDateTime(sqlValueCast<int64_t>(query->value("created_at")));
            QDateTime updatedAt = Utility::timestampToDateTime(sqlValueCast<int64_t>(query->value("updated_at")));
            int synced = sqlValueCast<int32_t>(query->value("synced"));

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
    auto insertQuery = m_database.createQuery();
    const std::string insertString =
        "INSERT INTO todos (uuid, user_uuid, title, description, category, important, deadline, "
        "recurrence_interval, recurrence_count, recurrence_start_date, is_completed, completed_at, "
        "is_trashed, trashed_at, created_at, updated_at, synced) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
    insertQuery->prepare(insertString);
    insertQuery->bindValues(newTodo->uuid().toString(),     //
                            newTodo->userUuid().toString(), //
                            newTodo->title(),               //
                            newTodo->description(),         //
                            newTodo->category(),            //
                            newTodo->important(),           //
                            newTodo->deadline().isValid() ? SqlValue(newTodo->deadline().toUTC().toMSecsSinceEpoch())
                                                          : SqlValue(nullptr),    //
                            newTodo->recurrenceInterval(),                        //
                            newTodo->recurrenceCount(),                           //
                            newTodo->recurrenceStartDate().toString(Qt::ISODate), //
                            newTodo->isCompleted(),
                            newTodo->completedAt().isValid()
                                ? SqlValue(newTodo->completedAt().toUTC().toMSecsSinceEpoch())
                                : SqlValue(nullptr), //
                            newTodo->isTrashed(),    //
                            newTodo->trashedAt().isValid() ? SqlValue(newTodo->trashedAt().toUTC().toMSecsSinceEpoch())
                                                           : SqlValue(nullptr), //
                            newTodo->createdAt().toUTC().toMSecsSinceEpoch(),   //
                            newTodo->updatedAt().toUTC().toMSecsSinceEpoch(),   //
                            newTodo->synced()                                   //
    );
    if (!insertQuery->exec()) {
        qCritical() << "插入待办事项到数据库失败:" << insertQuery->lastErrorQt();
        return false;
    }

    // 获取自增ID
    int newId = insertQuery->lastInsertRowId();
    if (newId == 0) {
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
    // 插入到数据库
    auto insertQuery = m_database.createQuery();
    const std::string insertString =
        "INSERT INTO todos (uuid, user_uuid, title, description, category, important, deadline, "
        "recurrence_interval, recurrence_count, recurrence_start_date, is_completed, completed_at, "
        "is_trashed, trashed_at, created_at, updated_at, synced) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
    insertQuery->prepare(insertString);
    insertQuery->bindValues(
        item->uuid().toString(),                                                                                 //
        item->userUuid().toString(),                                                                             //
        item->title(),                                                                                           //
        item->description(),                                                                                     //
        item->category(),                                                                                        //
        item->important(),                                                                                       //
        item->deadline().isValid() ? SqlValue(item->deadline().toUTC().toMSecsSinceEpoch()) : SqlValue(nullptr), //
        item->recurrenceInterval(),                                                                              //
        item->recurrenceCount(),                                                                                 //
        item->recurrenceStartDate().toString(Qt::ISODate),                                                       //
        item->isCompleted(),
        item->completedAt().isValid() ? SqlValue(item->completedAt().toUTC().toMSecsSinceEpoch())
                                      : SqlValue(nullptr),                                                         //
        item->isTrashed(),                                                                                         //
        item->trashedAt().isValid() ? SqlValue(item->trashedAt().toUTC().toMSecsSinceEpoch()) : SqlValue(nullptr), //
        item->createdAt().toUTC().toMSecsSinceEpoch(),                                                             //
        item->updatedAt().toUTC().toMSecsSinceEpoch(),                                                             //
        item->synced()                                                                                             //
    );
    if (!insertQuery->exec()) {
        qCritical() << "插入待办事项到数据库失败:" << insertQuery->lastErrorQt();
        return false;
    }
    int newId = insertQuery->lastInsertRowId();
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
    auto it = std::find_if(todos.begin(), todos.end(),
                           [uuid](const std::unique_ptr<TodoItem> &item) { return item->uuid() == uuid; });
    if (it == todos.end()) {
        qWarning() << "未找到待办事项，UUID:" << uuid.toString();
        return false;
    }

    // 更新数据库中的待办事项
    auto updateQuery = m_database.createQuery();
    // 记录要更新的字段 [原本类型，字段名]
    QList<QPair<QMetaType::Type, std::string>> fields;
    std::string order = "UPDATE todos SET ";
    auto appendField = [&](const std::string &col, QMetaType::Type t) {
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
    updateQuery->prepare(order);
    for (const auto &[type, name] : fields) {
        if (type == QMetaType::QDateTime) {
            QDateTime dt = QDateTime::fromString(todoData[QString::fromStdString(name)].toString(), Qt::ISODate);
            updateQuery->addBindValue(dt.isValid() ? SqlValue(dt.toUTC().toMSecsSinceEpoch()) : SqlValue(nullptr));
        } else if (type == QMetaType::QDate) {
            QDate d = QDate::fromString(todoData[QString::fromStdString(name)].toString(), Qt::ISODate);
            updateQuery->addBindValue(d.isValid() ? SqlValue(d.toString(Qt::ISODate)) : SqlValue(nullptr));
        } else if (type == QMetaType::Bool) {
            updateQuery->addBindValue(todoData[QString::fromStdString(name)].toBool());
        } else if (type == QMetaType::Int) {
            updateQuery->addBindValue(todoData[QString::fromStdString(name)].toInt());
        } else {
            updateQuery->addBindValue(todoData[QString::fromStdString(name)].toString());
        }
    }
    QDateTime now = QDateTime::currentDateTimeUtc();
    updateQuery->addBindValue(now.toMSecsSinceEpoch());
    updateQuery->addBindValue(((*it)->synced() != 1) ? 2 : 1);
    updateQuery->addBindValue(uuid);
    if (!updateQuery->exec()) {
        qCritical() << "更新待办事项到数据库失败:" << updateQuery->lastErrorQt();
        return false;
    }
    if (updateQuery->rowsAffected() == 0) {
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
    // 更新数据库中的待办事项
    auto updateQuery = m_database.createQuery();
    const std::string updateString =
        "UPDATE todos SET title = ?, description = ?, category = ?, important = ?, deadline = ?, "
        "recurrence_interval = ?, recurrence_count = ?, recurrence_start_date = ?, is_completed = ?, "
        "completed_at = ?, is_trashed = ?, trashed_at = ?, updated_at = ?, synced = ? WHERE uuid = ?";
    updateQuery->prepare(updateString);
    updateQuery->bindValues(
        item.title().toStdString(),                                                                                  //
        item.description().toStdString(),                                                                            //
        item.category().toStdString(),                                                                               //
        item.important(),                                                                                            //
        item.deadline().isValid() ? SqlValue(item.deadline().toUTC().toMSecsSinceEpoch()) : SqlValue(nullptr),       //
        item.recurrenceInterval(),                                                                                   //
        item.recurrenceCount(),                                                                                      //
        item.recurrenceStartDate().toString(Qt::ISODate),                                                            //
        item.isCompleted(),                                                                                          //
        item.completedAt().isValid() ? SqlValue(item.completedAt().toUTC().toMSecsSinceEpoch()) : SqlValue(nullptr), //
        item.isTrashed(),                                                                                            //
        item.trashedAt().isValid() ? SqlValue(item.trashedAt().toUTC().toMSecsSinceEpoch()) : SqlValue(nullptr),     //
        item.updatedAt().toUTC().toMSecsSinceEpoch(),                                                                //
        item.synced(),                                                                                               //
        item.uuid()                                                                                                  //
    );

    if (!updateQuery->exec()) {
        qCritical() << "更新待办事项到数据库失败:" << updateQuery->lastErrorQt();
        return false;
    }
    if (updateQuery->rowsAffected() == 0) {
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
    auto updateQuery = m_database.createQuery();
    const std::string updateString = "UPDATE todos SET synced = ? WHERE uuid = ?";
    updateQuery->prepare(updateString);
    updateQuery->bindValues( //
        3,                   // 已删除
        uuid                 //
    );
    if (!updateQuery->exec()) {
        qCritical() << "软删除待办事项失败:" << updateQuery->lastErrorQt();
        return false;
    }
    if (updateQuery->rowsAffected() == 0) {
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
        // 从数据库中永久删除所有待办事项
        auto deleteQuery = m_database.createQuery();
        const std::string deleteString = "DELETE FROM todos";
        deleteQuery->prepare(deleteString);

        if (!deleteQuery->exec()) {
            qCritical() << "永久删除所有待办事项失败:" << deleteQuery->lastErrorQt();
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
        // 更新数据库中所有待办事项的用户UUID
        auto updateQuery = m_database.createQuery();
        const std::string updateString = "UPDATE todos SET user_uuid = ?, synced = ?";
        updateQuery->prepare(updateString);
        updateQuery->bindValues(    //
            newUserUuid.toString(), // user_uuid
            synced                  // synced
        );
        if (!updateQuery->exec()) {
            qCritical() << "更新待办事项的用户UUID失败:" << updateQuery->lastErrorQt();
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
        // 从数据库中永久删除待办事项
        auto deleteQuery = m_database.createQuery();
        const std::string deleteString = "DELETE FROM todos WHERE uuid = ?";
        deleteQuery->prepare(deleteString);
        deleteQuery->addBindValue(uuid.toString());
        if (!deleteQuery->exec()) {
            qCritical() << "永久删除待办事项失败:" << deleteQuery->lastErrorQt();
            return false;
        }
        if (deleteQuery->rowsAffected() == 0) {
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
    // 建立内存索引
    QHash<QString, TodoItem *> uuidIndex;
    for (auto &it : todos) {
        if (it)
            uuidIndex.insert(it->uuid().toString(QUuid::WithoutBraces), it.get());
    }

    if (!m_database.beginTransaction()) {
        qCritical() << "无法开启事务以导入待办事项:" << m_database.lastError();
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
                auto insertQuery = m_database.createQuery();
                insertQuery->prepare( //
                    "INSERT INTO todos (uuid, user_uuid, title, description, category, important, deadline, "
                    "recurrence_interval, recurrence_count, recurrence_start_date, is_completed, completed_at, "
                    "is_trashed, trashed_at, created_at, updated_at, synced) VALUES "
                    "(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
                insertQuery->bindValues(       //
                    ref.uuid().toString(),     // uuid
                    ref.userUuid().toString(), // user_uuid
                    ref.title(),               // title
                    ref.description(),         // description
                    ref.category(),            // category
                    ref.important(),           // important
                    ref.deadline().isValid() ? SqlValue(ref.deadline().toUTC().toMSecsSinceEpoch())
                                             : SqlValue(nullptr),    // deadline
                    ref.recurrenceInterval(),                        // recurrence_interval
                    ref.recurrenceCount(),                           // recurrence_count
                    ref.recurrenceStartDate().toString(Qt::ISODate), // recurrence_start_date
                    ref.isCompleted(),                               // is_completed
                    ref.completedAt().isValid() ? SqlValue(ref.completedAt().toUTC().toMSecsSinceEpoch())
                                                : SqlValue(nullptr), // completed_at
                    ref.isTrashed(),                                 // is_trashed
                    ref.trashedAt().isValid() ? SqlValue(ref.trashedAt().toUTC().toMSecsSinceEpoch())
                                              : SqlValue(nullptr), // trashed_at
                    ref.createdAt().toUTC().toMSecsSinceEpoch(),   // created_at
                    ref.updatedAt().toUTC().toMSecsSinceEpoch(),   // updated_at
                    ref.synced()                                   // synced
                );
                if (!insertQuery->exec()) {
                    qCritical() << "插入导入待办失败:" << insertQuery->lastErrorQt();
                    success = false;
                    break;
                }
                auto idQuery = m_database.createQuery();
                if (idQuery->exec("SELECT last_insert_rowid()") && idQuery->next())
                    ref.setId(sqlValueCast<int32_t>(idQuery->value(0)));
                uuidIndex.insert(uuid.toString(QUuid::WithoutBraces), newItem.get());
                todos.push_back(std::move(newItem));
                ++insertCount;
                continue;
            }
            if (action == 解决冲突方案::Overwrite && existing) {
                auto updateQuery = m_database.createQuery();
                updateQuery->prepare( //
                    "UPDATE todos SET user_uuid=?, title=?, description=?, category=?, important=?, deadline=?, "
                    "recurrence_interval=?, recurrence_count=?, recurrence_start_date=?, is_completed=?, "
                    "completed_at=?, is_trashed=?, trashed_at=?, created_at=?, updated_at=?, synced=? WHERE uuid=?");
                updateQuery->bindValues( //
                    userUuid,            // user_uuid
                    title,               // title
                    description,         // description
                    category,            // category
                    important,           // important
                    deadline.isValid() ? SqlValue(deadline.toUTC().toMSecsSinceEpoch()) : SqlValue(nullptr), // deadline
                    recurrenceInterval,                        // recurrence_interval
                    recurrenceCount,                           // recurrence_count
                    recurrenceStartDate.toString(Qt::ISODate), // recurrence_start_date
                    isCompleted,                               // is_completed
                    completedAt.isValid() ? SqlValue(completedAt.toUTC().toMSecsSinceEpoch())
                                          : SqlValue(nullptr), // completed_at
                    isTrashed,                                 // is_trashed
                    trashedAt.isValid() ? SqlValue(trashedAt.toUTC().toMSecsSinceEpoch())
                                        : SqlValue(nullptr),                                // trashed_at
                    createdAt.toUTC().toMSecsSinceEpoch(),                                  // created_at
                    updatedAt.toUTC().toMSecsSinceEpoch(),                                  // updated_at
                    source == ImportSource::Server ? 0 : (existing->synced() == 1 ? 1 : 2), // synced
                    existing->uuid().toString()                                             // uuid
                );
                if (!updateQuery->exec()) {
                    qCritical() << "更新导入待办失败:" << updateQuery->lastErrorQt();
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
            if (!m_database.commitTransaction()) {
                qCritical() << "提交事务失败:" << m_database.lastError();
                m_database.rollbackTransaction();
                success = false;
            } else {
                qDebug() << "导入完成 - 新增:" << insertCount << ", 更新:" << updateCount << ", 跳过:" << skipCount;
            }
        } else {
            m_database.rollbackTransaction();
        }
    } catch (const std::exception &e) {
        qCritical() << "导入待办时发生异常:" << e.what();
        m_database.rollbackTransaction();
        success = false;
    } catch (...) {
        qCritical() << "导入待办时发生未知异常";
        m_database.rollbackTransaction();
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
    std::string sql = "SELECT id FROM todos WHERE 1=1";

    // 分类
    if (!opt.category.isEmpty())
        // sql += " AND category = :category";
        sql += " AND category = ?";
    if (opt.statusFilter == "todo")
        sql += " AND is_trashed = 0 AND is_completed = 0";
    else if (opt.statusFilter == "done")
        sql += " AND is_trashed = 0 AND is_completed = 1";
    else if (opt.statusFilter == "recycle")
        sql += " AND is_trashed = 1";
    else if (opt.statusFilter == "all") {
        // 与内存侧语义一致：all = 显示所有未回收任务
        sql += " AND is_trashed = 0";
    } else
        sql += " AND is_trashed = 0"; // 默认排除已回收

    if (!opt.searchText.isEmpty())
        // sql += " AND (title LIKE :search OR description LIKE :search OR category LIKE :search)";
        sql += " AND (title LIKE ? OR description LIKE ? OR category LIKE ?)";
    if (opt.dateFilterEnabled) {
        if (opt.dateStart.isValid()) {
            QDateTime startDt(opt.dateStart, QTime(0, 0), QTimeZone::UTC);
            qint64 startMs = startDt.toMSecsSinceEpoch();
            sql += " AND deadline >= " + std::to_string(startMs);
        }
        if (opt.dateEnd.isValid()) {
            QDate nextDay = opt.dateEnd.addDays(1);
            QDateTime endDt(nextDay, QTime(0, 0), QTimeZone::UTC); // 第二天开始的时间
            qint64 endMs = endDt.toMSecsSinceEpoch();
            sql += " AND deadline < " + std::to_string(endMs);
        }
    }

    sql += ' ' + 构建SQL排序语句(opt.sortType, opt.descending).toStdString();

    // 分页
    if (opt.limit > 0) {
        sql += QString(" LIMIT %1").arg(opt.limit).toStdString();
        if (opt.offset > 0)
            sql += QString(" OFFSET %1").arg(opt.offset).toStdString();
    }

    auto query = m_database.createQuery();
    query->prepare(sql);
    if (!opt.category.isEmpty()) {
        // query->bindValue(":category", opt.category);
        query->addBindValue(opt.category);
    }
    if (!opt.searchText.isEmpty()) {
        // 为 (title LIKE ? OR description LIKE ? OR category LIKE ?) 依次绑定 3 个相同的 LIKE 参数
        const QString like = '%' + opt.searchText + '%';
        query->addBindValue(like);
        query->addBindValue(like);
        query->addBindValue(like);
    }
    qInfo() << "查询待办ID列表 SQL:" << QString::fromStdString(sql);
    if (!query->exec()) {
        qCritical() << "查询待办事项ID失败:" << query->lastErrorQt();
        return indexs;
    }
    while (query->next())
        indexs.append(sqlValueCast<int32_t>(query->value(0)));
    return indexs;
}

/**
 * @brief 初始化TODO表
 * @return 初始化是否成功
 */
bool TodoDataStorage::初始化数据表() {
    return 创建数据表();
}

bool TodoDataStorage::创建数据表() {
    const std::string createTableQuery = R"(
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

    const std::vector<std::string> indexes = //
        {"CREATE INDEX IF NOT EXISTS idx_todos_uuid ON todos(uuid)",
         "CREATE INDEX IF NOT EXISTS idx_todos_user_uuid ON todos(user_uuid)",
         "CREATE INDEX IF NOT EXISTS idx_todos_category ON todos(category)",
         "CREATE INDEX IF NOT EXISTS idx_todos_deadline ON todos(deadline)",
         "CREATE INDEX IF NOT EXISTS idx_todos_completed ON todos(is_completed)",
         "CREATE INDEX IF NOT EXISTS idx_todos_trashed ON todos(is_trashed)",
         "CREATE INDEX IF NOT EXISTS idx_todos_synced ON todos(synced)"};
    for (const std::string &idx : indexes) {
        执行SQL查询(idx);
    }
    qDebug() << "todos表初始化成功";
    return true;
}

/**
 * @brief 导出待办数据到JSON对象
 * @param output 输出的JSON对象引用
 * @return 导出是否成功
 */
bool TodoDataStorage::exportToJson(QJsonObject &output) {
    auto query = m_database.createQuery();
    const std::string sql = "SELECT id, uuid, user_uuid, title, description, category, important, deadline, "
                            "recurrence_interval, recurrence_count, recurrence_start_date, is_completed, "
                            "completed_at, is_trashed, trashed_at, created_at, updated_at, synced FROM todos";
    if (!query->exec(sql)) {
        qWarning() << "查询待办数据失败:" << query->lastErrorQt();
        return false;
    }
    QJsonArray arr;
    while (query->next()) {
        QJsonObject obj;
        // TODO:id 是自增主键，不应该导出
        obj["id"] = sqlValueCast<int32_t>(query->value(0));
        obj["uuid"] = QString::fromStdString(sqlValueCast<std::string>(query->value(1)));
        obj["user_uuid"] = QString::fromStdString(sqlValueCast<std::string>(query->value(2)));
        obj["title"] = QString::fromStdString(sqlValueCast<std::string>(query->value(3)));
        QVariant desc = QString::fromStdString(sqlValueCast<std::string>(query->value(4)));
        obj["description"] = desc.isNull() ? QJsonValue() : desc.toString();
        obj["category"] = QString::fromStdString(sqlValueCast<std::string>(query->value(5)));
        obj["important"] = sqlValueCast<int32_t>(query->value(6));
        obj["deadline"] = Utility::timestampToIsoJson(sqlValueCast<int64_t>(query->value(7)));
        obj["recurrence_interval"] = sqlValueCast<int32_t>(query->value(8));
        obj["recurrence_count"] = sqlValueCast<int32_t>(query->value(9));
        QVariant rsd = QString::fromStdString(sqlValueCast<std::string>(query->value(10)));
        obj["recurrence_start_date"] = rsd.isNull() ? QJsonValue() : rsd.toString();
        obj["is_completed"] = sqlValueCast<int32_t>(query->value(11));
        obj["completed_at"] = Utility::timestampToIsoJson(sqlValueCast<int64_t>(query->value(12)));
        obj["is_trashed"] = sqlValueCast<int32_t>(query->value(13));
        obj["trashed_at"] = Utility::timestampToIsoJson(sqlValueCast<int64_t>(query->value(14)));
        obj["created_at"] = Utility::timestampToIsoJson(sqlValueCast<int64_t>(query->value(15)));
        obj["updated_at"] = Utility::timestampToIsoJson(sqlValueCast<int64_t>(query->value(16)));
        obj["synced"] = sqlValueCast<int32_t>(query->value(17));
        arr.append(obj);
    }
    output["todos"] = arr;
    return true;
}

bool TodoDataStorage::importFromJson(const QJsonObject &input, bool replaceAll) {
    if (!input.contains("todos") || !input["todos"].isArray())
        return true; // 没有数据
    auto query = m_database.createQuery();
    if (replaceAll) {
        if (!query->exec("DELETE FROM todos")) {
            qWarning() << "清空待办表失败:" << query->lastErrorQt();
            return false;
        }
    }
    QJsonArray arr = input["todos"].toArray();
    // TODO:id 是自增主键，不应该从JSON导入
    for (const auto &v : arr) {
        QJsonObject o = v.toObject();
        query->prepare( //
            "INSERT OR REPLACE INTO todos (id, uuid, user_uuid, title, description, category, "
            "important, deadline, recurrence_interval, recurrence_count, recurrence_start_date, "
            "is_completed, completed_at, is_trashed, trashed_at, created_at, updated_at, synced) "
            "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
        query->bindValues(                             //
            o["id"].toInt(),                           //
            o["uuid"].toString(),                      //
            o["user_uuid"].toString(),                 //
            o["title"].toString(),                     //
            o["description"].toString(),               //
            o["category"].toString(),                  //
            o["important"].toInt(),                    //
            Utility::fromJsonValue(o["deadline"]),     //
            o["recurrence_interval"].toInt(),          //
            o["recurrence_count"].toInt(),             //
            o["recurrence_start_date"].toString(),     //
            o["is_completed"].toInt(),                 //
            Utility::fromJsonValue(o["completed_at"]), //
            o["is_trashed"].toInt(),                   //
            Utility::fromJsonValue(o["trashed_at"]),   //
            Utility::fromJsonValue(o["created_at"]),   //
            Utility::fromJsonValue(o["updated_at"]),   //
            o["synced"].toInt()                        //
        );
        if (!query->exec()) {
            qWarning() << "导入待办数据失败:" << query->lastErrorQt();
            return false;
        }
    }
    qInfo() << "成功导入" << arr.size() << "条待办记录";
    return true;
}
