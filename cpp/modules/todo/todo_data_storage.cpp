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
#include <QTimeZone>
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
            uuids::uuid uuid = sqlValueCast<uuids::uuid>(query->value("uuid"));
            uuids::uuid userUuid = sqlValueCast<uuids::uuid>(query->value("user_uuid"));
            std::string title = sqlValueCast<std::string>(query->value("title"));
            std::string description = sqlValueCast<std::string>(query->value("description"));
            std::string category = sqlValueCast<std::string>(query->value("category"));
            bool important = sqlValueCast<bool>(query->value("important"));
            my::DateTime deadline = my::DateTime(sqlValueCast<int64_t>(query->value("deadline")));
            int recurrenceInterval = sqlValueCast<int32_t>(query->value("recurrence_interval"));
            int recurrenceCount = sqlValueCast<int32_t>(query->value("recurrence_count"));
            my::Date recurrenceStartDate = my::Date(sqlValueCast<std::string>(query->value("recurrence_start_date")));
            bool isCompleted = sqlValueCast<bool>(query->value("is_completed"));
            my::DateTime completedAt = my::DateTime(sqlValueCast<int64_t>(query->value("completed_at")));
            bool isTrashed = sqlValueCast<bool>(query->value("is_trashed"));
            my::DateTime trashedAt = my::DateTime(sqlValueCast<int64_t>(query->value("trashed_at")));
            my::DateTime createdAt = my::DateTime(sqlValueCast<int64_t>(query->value("created_at")));
            my::DateTime updatedAt = my::DateTime(sqlValueCast<int64_t>(query->value("updated_at")));
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
bool TodoDataStorage::新增待办(TodoList &todos, const std::string &title, const std::string &description,
                               const std::string &category, bool important, const my::DateTime &deadline,
                               int recurrenceInterval, int recurrenceCount, const my::Date &recurrenceStartDate,
                               uuids::uuid userUuid) {
    my::DateTime now = my::DateTime::now();
    uuids::uuid newUuid = uuids::uuid_system_generator{}();
    my::DateTime nullTime; // 代表空

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
    insertQuery->bindValues(
        newTodo->uuid(),                                                                                       //
        newTodo->userUuid(),                                                                                   //
        newTodo->title(),                                                                                      //
        newTodo->description(),                                                                                //
        newTodo->category(),                                                                                   //
        newTodo->important(),                                                                                  //
        newTodo->deadline().isValid() ? SqlValue(newTodo->deadline().toUnixTimestampMs()) : SqlValue(nullptr), //
        newTodo->recurrenceInterval(),                                                                         //
        newTodo->recurrenceCount(),                                                                            //
        newTodo->recurrenceStartDate().toString(),                                                             //
        newTodo->isCompleted(),
        newTodo->completedAt().isValid() ? SqlValue(newTodo->completedAt().toUnixTimestampMs()) : SqlValue(nullptr), //
        newTodo->isTrashed(),                                                                                        //
        newTodo->trashedAt().isValid() ? SqlValue(newTodo->trashedAt().toUnixTimestampMs()) : SqlValue(nullptr),     //
        newTodo->createdAt().toUnixTimestampMs(),                                                                    //
        newTodo->updatedAt().toUnixTimestampMs(),                                                                    //
        newTodo->synced()                                                                                            //
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
        item->uuid(),                                                                                          //
        item->userUuid(),                                                                                      //
        item->title(),                                                                                         //
        item->description(),                                                                                   //
        item->category(),                                                                                      //
        item->important(),                                                                                     //
        item->deadline().isValid() ? SqlValue(item->deadline().toUnixTimestampMs()) : SqlValue(nullptr),       //
        item->recurrenceInterval(),                                                                            //
        item->recurrenceCount(),                                                                               //
        item->recurrenceStartDate().toString(),                                                                //
        item->isCompleted(),                                                                                   //
        item->completedAt().isValid() ? SqlValue(item->completedAt().toUnixTimestampMs()) : SqlValue(nullptr), //
        item->isTrashed(),                                                                                     //
        item->trashedAt().isValid() ? SqlValue(item->trashedAt().toUnixTimestampMs()) : SqlValue(nullptr),     //
        item->createdAt().toUnixTimestampMs(),                                                                 //
        item->updatedAt().toUnixTimestampMs(),                                                                 //
        item->synced()                                                                                         //
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
bool TodoDataStorage::更新待办(TodoList &todos, const uuids::uuid &uuid, const QVariantMap &todoData) {
    auto it = std::find_if(todos.begin(), todos.end(),
                           [uuid](const std::unique_ptr<TodoItem> &item) { return item->uuid() == uuid; });
    if (it == todos.end()) {
        qWarning() << "未找到待办事项，UUID:" << uuids::to_string(uuid);
        return false;
    }

    // 更新数据库中的待办事项
    auto updateQuery = m_database.createQuery();
    // 记录要更新的字段 [原本类型，字段名]
    std::vector<std::pair<QMetaType::Type, std::string>> fields;
    std::string order = "UPDATE todos SET ";
    auto appendField = [&](const std::string &col, QMetaType::Type t) {
        order += col + " = ?, ";
        fields.emplace_back(t, col);
    };
    if (todoData.contains("title")) {
        appendField("title", QMetaType::QString);
        (*it)->setTitle(todoData["title"].toString().toStdString());
    }
    if (todoData.contains("description")) {
        appendField("description", QMetaType::QString);
        (*it)->setDescription(todoData["description"].toString().toStdString());
    }
    if (todoData.contains("category")) {
        appendField("category", QMetaType::QString);
        (*it)->setCategory(todoData["category"].toString().toStdString());
    }
    if (todoData.contains("important")) {
        appendField("important", QMetaType::Bool);
        (*it)->setImportant(todoData["important"].toBool());
    }
    if (todoData.contains("deadline")) {
        appendField("deadline", QMetaType::QDateTime);
        (*it)->setDeadline(my::DateTime(todoData["deadline"].toString().toStdString()));
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
        (*it)->setRecurrenceStartDate(my::Date(todoData["recurrence_start_date"].toString().toStdString()));
    }
    if (todoData.contains("is_completed")) {
        appendField("is_completed", QMetaType::Bool);
        (*it)->setIsCompleted(todoData["is_completed"].toBool());
    }
    if (todoData.contains("completed_at")) {
        appendField("completed_at", QMetaType::QDateTime);
        (*it)->setCompletedAt(my::DateTime(todoData["completed_at"].toString().toStdString()));
    }
    if (todoData.contains("is_trashed")) {
        appendField("is_trashed", QMetaType::Bool);
        (*it)->setIsTrashed(todoData["is_trashed"].toBool());
    }
    if (todoData.contains("trashed_at")) {
        appendField("trashed_at", QMetaType::QDateTime);
        (*it)->setTrashedAt(my::DateTime(todoData["trashed_at"].toString().toStdString()));
    }
    // 始终更新这两个字段
    order += "updated_at = ?, synced = ? WHERE uuid = ?";
    updateQuery->prepare(order);
    for (const auto &[type, name] : fields) {
        if (type == QMetaType::QDateTime) {
            updateQuery->addBindValue(my::DateTime(todoData[QString::fromStdString(name)].toString().toStdString()));
        } else if (type == QMetaType::QDate) {
            updateQuery->addBindValue(my::Date(todoData[QString::fromStdString(name)].toString().toStdString()));
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
    updateQuery->addBindValue(uuids::to_string(uuid));
    if (!updateQuery->exec()) {
        qCritical() << "更新待办事项到数据库失败:" << updateQuery->lastErrorQt();
        return false;
    }
    if (updateQuery->rowsAffected() == 0) {
        qWarning() << "未找到UUID为" << uuids::to_string(uuid) << "的待办事项";
        return false;
    }
    (*it)->setUpdatedAt(now);
    (*it)->setSynced(2);
    qDebug() << "成功更新待办事项，UUID:" << uuids::to_string(uuid);
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
        item.title(),                                                                                        //
        item.description(),                                                                                  //
        item.category(),                                                                                     //
        item.important(),                                                                                    //
        item.deadline().isValid() ? SqlValue(item.deadline().toUnixTimestampMs()) : SqlValue(nullptr),       //
        item.recurrenceInterval(),                                                                           //
        item.recurrenceCount(),                                                                              //
        item.recurrenceStartDate().toString(),                                                               //
        item.isCompleted(),                                                                                  //
        item.completedAt().isValid() ? SqlValue(item.completedAt().toUnixTimestampMs()) : SqlValue(nullptr), //
        item.isTrashed(),                                                                                    //
        item.trashedAt().isValid() ? SqlValue(item.trashedAt().toUnixTimestampMs()) : SqlValue(nullptr),     //
        item.updatedAt().toUnixTimestampMs(),                                                                //
        item.synced(),                                                                                       //
        item.uuid()                                                                                          //
    );

    if (!updateQuery->exec()) {
        qCritical() << "更新待办事项到数据库失败:" << updateQuery->lastErrorQt();
        return false;
    }
    if (updateQuery->rowsAffected() == 0) {
        qWarning() << "未找到UUID为" << uuids::to_string(item.uuid()) << "的待办事项";
        return false;
    }
    qDebug() << "成功更新待办事项，UUID:" << uuids::to_string(item.uuid());
    return true;
}

/**
 * @brief 回收待办事项
 * @param todos 待办事项容器引用
 * @param uuid 待办事项UUID
 * @return 回收是否成功
 */
bool TodoDataStorage::回收待办(TodoList &todos, const uuids::uuid &uuid) {
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
bool TodoDataStorage::软删除待办([[maybe_unused]] TodoList &todos, const uuids::uuid &uuid) {
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
        qWarning() << "未找到UUID为" << uuids::to_string(uuid) << "的待办事项";
        return false;
    }
    qDebug() << "成功软删除待办事项，UUID:" << uuids::to_string(uuid);
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
bool TodoDataStorage::更新所有待办用户UUID(TodoList &todos, const uuids::uuid &newUserUuid, int synced) {
    bool success = false;
    try {
        // 更新数据库中所有待办事项的用户UUID
        auto updateQuery = m_database.createQuery();
        const std::string updateString = "UPDATE todos SET user_uuid = ?, synced = ?";
        updateQuery->prepare(updateString);
        updateQuery->bindValues(           //
            uuids::to_string(newUserUuid), // user_uuid
            synced                         // synced
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

        qDebug() << "成功更新所有待办事项的用户UUID为" << uuids::to_string(newUserUuid);
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
bool TodoDataStorage::删除待办([[maybe_unused]] TodoList &todos, const uuids::uuid &uuid) {
    bool success = false;
    try {
        // 从数据库中永久删除待办事项
        auto deleteQuery = m_database.createQuery();
        const std::string deleteString = "DELETE FROM todos WHERE uuid = ?";
        deleteQuery->prepare(deleteString);
        deleteQuery->addBindValue(uuids::to_string(uuid));
        if (!deleteQuery->exec()) {
            qCritical() << "永久删除待办事项失败:" << deleteQuery->lastErrorQt();
            return false;
        }
        if (deleteQuery->rowsAffected() == 0) {
            qWarning() << "未找到UUID为" << uuids::to_string(uuid) << "的待办事项，无法删除";
            return false;
        }
        // 要更新qml视图，所以不在这里删除
        // todos.erase(std::remove_if(todos.begin(), todos.end(),
        //                            [&](const std::unique_ptr<TodoItem> &p) { return p && p->uuid() == uuid; }),
        //             todos.end());
        qDebug() << "成功永久删除待办事项，UUID:" << uuids::to_string(uuid);
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
bool TodoDataStorage::导入待办事项从JSON(TodoList &todos, const nlohmann::json &todosArray, //
                                         ImportSource source, 解决冲突方案 resolution) {
    bool success = true;
    // 建立内存索引
    std::unordered_map<uuids::uuid, TodoItem *> uuidIndex;
    for (auto &it : todos) {
        if (it)
            uuidIndex.insert({it->uuid(), it.get()});
    }

    if (!m_database.beginTransaction()) {
        qCritical() << "无法开启事务以导入待办事项:" << m_database.lastError();
        return false;
    }

    int insertCount = 0;
    int updateCount = 0;
    int skipCount = 0;

    try {
        for (const auto &value : todosArray) {
            if (!value.is_object()) {
                qWarning() << "跳过无效待办（非对象）";
                ++skipCount;
                continue;
            }
            nlohmann::json obj = value;
            if (!obj.contains("title") || !obj.contains("user_uuid")) {
                qWarning() << "跳过无效待办（缺字段）";
                ++skipCount;
                continue;
            }
            uuids::uuid userUuid = uuids::uuid::from_string(obj["user_uuid"].get<std::string>()).value();
            if (userUuid.is_nil()) {
                qWarning() << "跳过无效待办（user_uuid 无效）";
                ++skipCount;
                continue;
            }
            uuids::uuid uuid;
            if (obj.contains("uuid")) {
                auto opt = uuids::uuid::from_string(obj["uuid"].get<std::string>());
                uuid = opt.has_value() ? opt.value() : uuids::uuid_system_generator{}();
            } else {
                uuid = uuids::uuid_system_generator{}();
            }
            std::string title = obj["title"].get<std::string>();
            std::string description = obj["description"].get<std::string>();
            std::string category = obj["category"].get<std::string>();
            bool important = obj["important"].get<bool>();
            my::DateTime deadline = my::DateTime::fromISOString(obj["deadline"].get<std::string>());
            int recurrenceInterval = obj["recurrence_interval"].get<int>();
            int recurrenceCount = obj["recurrence_count"].get<int>();
            my::Date recurrenceStartDate =
                my::Date::fromISOString(obj["recurrence_start_date"].get<std::string>());
            bool isCompleted = obj["is_completed"].get<bool>();
            my::DateTime completedAt = my::DateTime::fromISOString(obj["completed_at"].get<std::string>());
            bool isTrashed = obj["is_trashed"].get<bool>();
            my::DateTime trashedAt = my::DateTime::fromISOString(obj["trashed_at"].get<std::string>());
            my::DateTime createdAt = my::DateTime::fromISOString(obj["created_at"].get<std::string>());
            if (!createdAt.isValid())
                createdAt = my::DateTime::now();
            my::DateTime updatedAt = my::DateTime::fromISOString(obj["updated_at"].get<std::string>());
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
            TodoItem *existing = uuidIndex[uuid];

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
                insertQuery->bindValues( //
                    ref.uuid(),          // uuid
                    ref.userUuid(),      // user_uuid
                    ref.title(),         // title
                    ref.description(),   // description
                    ref.category(),      // category
                    ref.important(),     // important
                    ref.deadline().isValid() ? SqlValue(ref.deadline().toUnixTimestampMs()) : SqlValue(nullptr), //
                    ref.recurrenceInterval(),             // recurrence_interval
                    ref.recurrenceCount(),                // recurrence_count
                    ref.recurrenceStartDate().toString(), // recurrence_start_date
                    ref.isCompleted(),                    // is_completed
                    ref.completedAt().isValid() ? SqlValue(ref.completedAt().toUnixTimestampMs())
                                                : SqlValue(nullptr), //
                    ref.isTrashed(),                                 // is_trashed
                    ref.trashedAt().isValid() ? SqlValue(ref.trashedAt().toUnixTimestampMs()) : SqlValue(nullptr), //
                    ref.createdAt().toUnixTimestampMs(), // created_at
                    ref.updatedAt().toUnixTimestampMs(), // updated_at
                    ref.synced()                         // synced
                );
                if (!insertQuery->exec()) {
                    qCritical() << "插入导入待办失败:" << insertQuery->lastErrorQt();
                    success = false;
                    break;
                }
                auto idQuery = m_database.createQuery();
                if (idQuery->exec("SELECT last_insert_rowid()") && idQuery->next())
                    ref.setId(sqlValueCast<int32_t>(idQuery->value(0)));
                uuidIndex[uuid] = newItem.get();
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
                updateQuery->bindValues(                                                             //
                    userUuid,                                                                        // user_uuid
                    title,                                                                           // title
                    description,                                                                     // description
                    category,                                                                        // category
                    important,                                                                       // important
                    deadline.isValid() ? SqlValue(deadline.toUnixTimestampMs()) : SqlValue(nullptr), // deadline
                    recurrenceInterval,                // recurrence_interval
                    recurrenceCount,                   // recurrence_count
                    recurrenceStartDate.toISOString(), // recurrence_start_date
                    isCompleted,                       // is_completed
                    completedAt.isValid() ? SqlValue(completedAt.toUnixTimestampMs())
                                          : SqlValue(nullptr),                                         // completed_at
                    isTrashed,                                                                         // is_trashed
                    trashedAt.isValid() ? SqlValue(trashedAt.toUnixTimestampMs()) : SqlValue(nullptr), // trashed_at
                    createdAt.toUnixTimestampMs(),                                                     // created_at
                    updatedAt.toUnixTimestampMs(),                                                     // updated_at
                    source == ImportSource::Server ? 0 : (existing->synced() == 1 ? 1 : 2),            // synced
                    existing->uuid()                                                                   // uuid
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
std::string TodoDataStorage::构建SQL排序语句(int sortType, bool descending) {
    std::string order;
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
std::vector<int> TodoDataStorage::查询待办ID列表(const QueryOptions &opt) {
    std::vector<int> indexs;
    std::string sql = "SELECT id FROM todos WHERE 1=1";

    // 分类
    if (!opt.category.empty())
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

    if (!opt.searchText.empty())
        // sql += " AND (title LIKE :search OR description LIKE :search OR category LIKE :search)";
        sql += " AND (title LIKE ? OR description LIKE ? OR category LIKE ?)";
    if (opt.dateFilterEnabled) {
        if (opt.dateStart.isValid()) {
            my::DateTime startDt(opt.dateStart, my::Time());
            int64_t startMs = startDt.toUnixTimestamp();
            sql += " AND deadline >= " + std::to_string(startMs);
        }
        if (opt.dateEnd.isValid()) {
            my::Date endDate(opt.dateEnd);
            my::Date nextDay = endDate.addDays(1);
            my::DateTime endDt(nextDay, my::Time()); // 第二天开始的时间
            int64_t endMs = endDt.toUnixTimestamp();
            sql += " AND deadline < " + std::to_string(endMs);
        }
    }

    sql += ' ' + 构建SQL排序语句(opt.sortType, opt.descending);

    // 分页
    if (opt.limit > 0) {
        sql += " LIMIT " + std::to_string(opt.limit);
        if (opt.offset > 0)
            sql += " OFFSET " + std::to_string(opt.offset);
    }

    auto query = m_database.createQuery();
    query->prepare(sql);
    if (!opt.category.empty()) {
        // query->bindValue(":category", opt.category);
        query->addBindValue(opt.category);
    }
    if (!opt.searchText.empty()) {
        // 为 (title LIKE ? OR description LIKE ? OR category LIKE ?) 依次绑定 3 个相同的 LIKE 参数
        const std::string like = '%' + opt.searchText + '%';
        query->addBindValue(like);
        query->addBindValue(like);
        query->addBindValue(like);
    }
    // qInfo() << "查询待办ID列表 SQL:" << QString::fromStdString(sql);
    if (!query->exec()) {
        qCritical() << "查询待办事项ID失败:" << query->lastErrorQt();
        return indexs;
    }
    while (query->next())
        indexs.push_back(sqlValueCast<int32_t>(query->value(0)));
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
bool TodoDataStorage::exportToJson(nlohmann::json &output) {
    auto query = m_database.createQuery();
    const std::string sql = "SELECT uuid, user_uuid, title, description, category, important, deadline, "
                            "recurrence_interval, recurrence_count, recurrence_start_date, is_completed, "
                            "completed_at, is_trashed, trashed_at, created_at, updated_at, synced FROM todos";
    if (!query->exec(sql)) {
        qWarning() << "查询待办数据失败:" << query->lastErrorQt();
        return false;
    }
    nlohmann::json arr;
    while (query->next()) {
        nlohmann::json obj;
        obj["uuid"] = sqlValueCast<std::string>(query->value(1));
        obj["user_uuid"] = sqlValueCast<std::string>(query->value(2));
        obj["title"] = sqlValueCast<std::string>(query->value(3));
        obj["description"] = sqlValueCast<std::string>(query->value(4));
        obj["category"] = sqlValueCast<std::string>(query->value(5));
        obj["important"] = sqlValueCast<int32_t>(query->value(6));
        obj["deadline"] = my::DateTime(sqlValueCast<int64_t>(query->value(7))).toISOString();
        obj["recurrence_interval"] = sqlValueCast<int32_t>(query->value(8));
        obj["recurrence_count"] = sqlValueCast<int32_t>(query->value(9));
        obj["recurrence_start_date"] = sqlValueCast<std::string>(query->value(10));
        obj["is_completed"] = sqlValueCast<int32_t>(query->value(11));
        obj["completed_at"] = my::DateTime(sqlValueCast<int64_t>(query->value(12))).toISOString();
        obj["is_trashed"] = sqlValueCast<int32_t>(query->value(13));
        obj["trashed_at"] = my::DateTime(sqlValueCast<int64_t>(query->value(14))).toISOString();
        obj["created_at"] = my::DateTime(sqlValueCast<int64_t>(query->value(15))).toISOString();
        obj["updated_at"] = my::DateTime(sqlValueCast<int64_t>(query->value(16))).toISOString();
        obj["synced"] = sqlValueCast<int32_t>(query->value(17));
        arr.push_back(obj);
    }
    output["todos"] = arr;
    return true;
}

bool TodoDataStorage::importFromJson(const nlohmann::json_abi_v3_11_3::json &input, bool replaceAll) {
    if (!input.contains("todos") || !input["todos"].is_array())
        return true; // 没有数据
    auto query = m_database.createQuery();
    if (replaceAll) {
        if (!query->exec("DELETE FROM todos")) {
            qWarning() << "清空待办表失败:" << query->lastErrorQt();
            return false;
        }
    }
    nlohmann::json arr = input["todos"];
    for (const auto &v : arr) {
        nlohmann::json o = v;
        query->prepare( //
            "INSERT OR REPLACE INTO todos (uuid, user_uuid, title, description, category, "
            "important, deadline, recurrence_interval, recurrence_count, recurrence_start_date, "
            "is_completed, completed_at, is_trashed, trashed_at, created_at, updated_at, synced) "
            "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
        query->bindValues(                             //
            o["uuid"].get<std::string>(),              //
            o["user_uuid"].get<std::string>(),        //
            o["title"].get<std::string>(),            //
            o["description"].get<std::string>(),      //
            o["category"].get<std::string>(),         //
            o["important"].get<int32_t>(),            //
            my::DateTime::fromISOString(o["deadline"].get<std::string>()).toUnixTimestampMs(),     //
            o["recurrence_interval"].get<int32_t>(),  //
            o["recurrence_count"].get<int32_t>(),     //
            o["recurrence_start_date"].get<std::string>(), //
            o["is_completed"].get<int32_t>(),                 //
            my::DateTime::fromISOString(o["completed_at"].get<std::string>()).toUnixTimestampMs(), //
            o["is_trashed"].get<int32_t>(),                   //
            my::DateTime::fromISOString(o["trashed_at"].get<std::string>()).toUnixTimestampMs(),   //
            my::DateTime::fromISOString(o["created_at"].get<std::string>()).toUnixTimestampMs(),   //
            my::DateTime::fromISOString(o["updated_at"].get<std::string>()).toUnixTimestampMs(),   //
            o["synced"].get<int32_t>()                        //
        );
        if (!query->exec()) {
            qWarning() << "导入待办数据失败:" << query->lastErrorQt();
            return false;
        }
    }
    qInfo() << "成功导入" << arr.size() << "条待办记录";
    return true;
}
