/**
 * @file todo_data_storage.cpp
 * @brief TodoDataStorage类的实现文件
 *
 * 该文件实现了TodoDataStorage类的所有方法，负责待办事项的本地存储和文件导入导出功能。
 *
 * @author Sakurakugu
 * @date 2025-08-25 00:54:11(UTC+8) 周一
 * @change 2025-09-02 10:52:13(UTC+8) 周二
 * @version 0.4.0
 */

#include "todo_data_storage.h"
#include "user_auth.h"

#include "foundation/config.h"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariant>
#include <algorithm>
#include <optional>
#include <unordered_map>

TodoDataStorage::TodoDataStorage(QObject *parent)
    : QObject(parent),                    // 父对象
      m_setting(Setting::GetInstance()),  // 设置
      m_database(Database::GetInstance()) // 数据库管理器
{
    // 确保数据库已初始化
    if (!m_database.initializeDatabase()) {
        qCritical() << "TodoDataStorage: 数据库初始化失败";
    }
}

TodoDataStorage::~TodoDataStorage() {}

/**
 * @brief 加载待办事项
 * @param todos 待办事项容器引用
 * @return 加载是否成功
 */
bool TodoDataStorage::加载待办事项(TodoList &todos) {
    bool success = true;

    try {
        // 清除当前数据
        todos.clear();

        // 从数据库加载数据
        QSqlDatabase db = m_database.getDatabase();
        if (!db.isOpen()) {
            qCritical() << "数据库未打开，无法加载待办事项";
            return false;
        }

        QSqlQuery query(db);
        const QString queryString =
            "SELECT id, uuid, user_uuid, title, description, category, important, deadline, recurrence_interval, "
            "recurrence_count, recurrence_start_date, is_completed, completed_at, is_deleted, deleted_at, created_at, "
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
            QDateTime deadline = QDateTime::fromString(query.value("deadline").toString(), Qt::ISODate);
            int recurrenceInterval = query.value("recurrence_interval").toInt();
            int recurrenceCount = query.value("recurrence_count").toInt();
            QDate recurrenceStartDate = QDate::fromString(query.value("recurrence_start_date").toString(), Qt::ISODate);
            bool isCompleted = query.value("is_completed").toBool();
            QDateTime completedAt = QDateTime::fromString(query.value("completed_at").toString(), Qt::ISODate);
            bool isDeleted = query.value("is_deleted").toBool();
            QDateTime deletedAt = QDateTime::fromString(query.value("deleted_at").toString(), Qt::ISODate);
            QDateTime createdAt = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);
            QDateTime updatedAt = QDateTime::fromString(query.value("updated_at").toString(), Qt::ISODate);
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
                isDeleted,                          // 是否已删除
                deletedAt,                          // 删除时间
                createdAt,                          // 创建时间
                updatedAt,                          // 更新时间
                synced,                             // 是否已同步
                this);

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
 * @brief 将待办事项保存到本地存储
 * @param todos 待办事项容器引用
 * @return 保存是否成功
 */
bool TodoDataStorage::saveToLocalStorage(const TodoList &todos) {
    bool success = true;

    try {
        QSqlDatabase db = m_database.getDatabase();
        if (!db.isOpen()) {
            qCritical() << "数据库未打开，无法保存待办事项";
            return false;
        }

        // 开始事务
        if (!db.transaction()) {
            qCritical() << "无法开始数据库事务:" << db.lastError().text();
            return false;
        }

        // 清除现有数据
        QSqlQuery deleteQuery(db);
        if (!deleteQuery.exec("DELETE FROM todos")) {
            qCritical() << "清除待办事项数据失败:" << deleteQuery.lastError().text();
            db.rollback();
            return false;
        }

        // 插入新数据
        QSqlQuery insertQuery(db);
        const QString insertString =
            "INSERT INTO todos (id, uuid, user_uuid, title, description, category, important, deadline, "
            "recurrence_interval, recurrence_count, recurrence_start_date, is_completed, completed_at, is_deleted, "
            "deleted_at, created_at, updated_at, synced) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
            "?, ?, ?, ?, ?, ?, ?)";
        insertQuery.prepare(insertString);

        for (const auto &item : todos) {
            insertQuery.addBindValue(item->id());
            insertQuery.addBindValue(item->uuid().toString());
            insertQuery.addBindValue(item->userUuid().toString());
            insertQuery.addBindValue(item->title());
            insertQuery.addBindValue(item->description());
            insertQuery.addBindValue(item->category());
            insertQuery.addBindValue(item->important());
            insertQuery.addBindValue(item->deadline().toString(Qt::ISODate));
            insertQuery.addBindValue(item->recurrenceInterval());
            insertQuery.addBindValue(item->recurrenceCount());
            insertQuery.addBindValue(item->recurrenceStartDate().toString(Qt::ISODate));
            insertQuery.addBindValue(item->isCompleted());
            insertQuery.addBindValue(item->completedAt().toString(Qt::ISODate));
            insertQuery.addBindValue(item->isDeleted());
            insertQuery.addBindValue(item->deletedAt().toString(Qt::ISODate));
            insertQuery.addBindValue(item->createdAt().toString(Qt::ISODate));
            insertQuery.addBindValue(item->updatedAt().toString(Qt::ISODate));
            insertQuery.addBindValue(item->synced());

            if (!insertQuery.exec()) {
                qCritical() << "插入待办事项数据失败:" << insertQuery.lastError().text();
                db.rollback();
                return false;
            }
        }

        // 提交事务
        if (!db.commit()) {
            qCritical() << "提交数据库事务失败:" << db.lastError().text();
            return false;
        }

        qDebug() << "已成功保存" << todos.size() << "个待办事项到数据库";
        success = true;
    } catch (const std::exception &e) {
        qCritical() << "保存到本地存储时发生异常:" << e.what();
        success = false;
    } catch (...) {
        qCritical() << "保存到本地存储时发生未知异常";
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
 * @return 新添加的待办事项指针
 */
std::unique_ptr<TodoItem> TodoDataStorage::新增待办(TodoList &todos, const QString &title, const QString &description,
                                                    const QString &category, bool important, const QDateTime &deadline,
                                                    int recurrenceInterval, int recurrenceCount,
                                                    const QDate &recurrenceStartDate, QUuid userUuid) {
    QSqlDatabase db = m_database.getDatabase();
    if (!db.isOpen()) {
        qCritical() << "数据库未打开，无法添加待办事项";
        return nullptr;
    }

    QDateTime now = QDateTime::currentDateTime();
    QUuid newUuid = QUuid::createUuid();
    QDateTime nullTime = QDateTime::fromString("1970-01-01T00:00:00", Qt::ISODate);

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
        false,                                 // isDeleted
        nullTime,                              // deletedAt
        now,                                   // createdAt
        now,                                   // updatedAt
        1,                                     // synced
        this                                   // parent
    );

    // 插入到数据库
    QSqlQuery insertQuery(db);
    const QString insertString =
        "INSERT INTO todos (uuid, user_uuid, title, description, category, important, deadline, "
        "recurrence_interval, recurrence_count, recurrence_start_date, is_completed, completed_at, is_deleted, "
        "deleted_at, created_at, updated_at, synced) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    insertQuery.prepare(insertString);
    insertQuery.addBindValue(newTodo->uuid().toString());
    insertQuery.addBindValue(newTodo->userUuid().toString());
    insertQuery.addBindValue(newTodo->title());
    insertQuery.addBindValue(newTodo->description());
    insertQuery.addBindValue(newTodo->category());
    insertQuery.addBindValue(newTodo->important());
    insertQuery.addBindValue(newTodo->deadline().toString(Qt::ISODate));
    insertQuery.addBindValue(newTodo->recurrenceInterval());
    insertQuery.addBindValue(newTodo->recurrenceCount());
    insertQuery.addBindValue(newTodo->recurrenceStartDate().toString(Qt::ISODate));
    insertQuery.addBindValue(newTodo->isCompleted());
    insertQuery.addBindValue(newTodo->completedAt().toString(Qt::ISODate));
    insertQuery.addBindValue(newTodo->isDeleted());
    insertQuery.addBindValue(newTodo->deletedAt().toString(Qt::ISODate));
    insertQuery.addBindValue(newTodo->createdAt().toString(Qt::ISODate));
    insertQuery.addBindValue(newTodo->updatedAt().toString(Qt::ISODate));
    insertQuery.addBindValue(newTodo->synced());

    if (!insertQuery.exec()) {
        qCritical() << "插入待办事项到数据库失败:" << insertQuery.lastError().text();
        return nullptr;
    }

    // 获取自增ID
    QSqlQuery idQuery(db);
    int newId = -1;
    if (idQuery.exec("SELECT last_insert_rowid()")) {
        if (idQuery.next()) {
            newId = idQuery.value(0).toInt();
        }
    }
    if (newId <= 0) {
        qWarning() << "获取自增ID失败，使用临时ID -1";
    }
    newTodo->setId(newId);

    qDebug() << "成功添加待办事项到数据库，ID:" << newId;
    todos.push_back(std::move(newTodo));
    // 目前调用方未使用返回值，保持接口，返回空unique_ptr（之前实现返回已被移动的指针，同样为空）。
    return {};
}

/**
 * @brief 添加新的待办事项
 * @param todos 待办事项容器引用
 * @param item 待办事项智能指针(已经改好的)
 */
bool TodoDataStorage::新增待办(TodoList &todos, std::unique_ptr<TodoItem> item) {
    QSqlDatabase db = m_database.getDatabase();
    if (!db.isOpen()) {
        qCritical() << "数据库未打开，无法添加待办事项";
        return false;
    }

    // 插入到数据库
    QSqlQuery insertQuery(db);
    const QString insertString =
        "INSERT INTO todos (uuid, user_uuid, title, description, category, important, deadline, "
        "recurrence_interval, recurrence_count, recurrence_start_date, is_completed, completed_at, is_deleted, "
        "deleted_at, created_at, updated_at, synced) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    insertQuery.prepare(insertString);
    insertQuery.addBindValue(item->uuid().toString());
    insertQuery.addBindValue(item->userUuid().toString());
    insertQuery.addBindValue(item->title());
    insertQuery.addBindValue(item->description());
    insertQuery.addBindValue(item->category());
    insertQuery.addBindValue(item->important());
    insertQuery.addBindValue(item->deadline().toString(Qt::ISODate));
    insertQuery.addBindValue(item->recurrenceInterval());
    insertQuery.addBindValue(item->recurrenceCount());
    insertQuery.addBindValue(item->recurrenceStartDate().toString(Qt::ISODate));
    insertQuery.addBindValue(item->isCompleted());
    insertQuery.addBindValue(item->completedAt().toString(Qt::ISODate));
    insertQuery.addBindValue(item->isDeleted());
    insertQuery.addBindValue(item->deletedAt().toString(Qt::ISODate));
    insertQuery.addBindValue(item->createdAt().toString(Qt::ISODate));
    insertQuery.addBindValue(item->updatedAt().toString(Qt::ISODate));
    insertQuery.addBindValue(item->synced());

    if (!insertQuery.exec()) {
        qCritical() << "插入待办事项到数据库失败:" << insertQuery.lastError().text();
        return false;
    }

    // 获取自增ID
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
 * @param title 新标题
 * @param description 新描述
 * @param category 新分类
 * @param important 是否重要
 * @param deadline 新截止日期
 * @param recurrenceInterval 重复间隔
 * @param recurrenceCount 重复次数
 * @param recurrenceStartDate 重复开始日期
 * @return 更新是否成功
 */
bool TodoDataStorage::更新待办(TodoList &todos, int id, const QString &title, const QString &description,
                               const QString &category, bool important, const QDateTime &deadline,
                               int recurrenceInterval, int recurrenceCount, const QDate &recurrenceStartDate) {
    QSqlDatabase db = m_database.getDatabase();
    if (!db.isOpen()) {
        qCritical() << "数据库未打开，无法更新待办事项";
        return false;
    }

    QDateTime now = QDateTime::currentDateTime();

    // 更新数据库中的待办事项
    QSqlQuery updateQuery(db);
    const QString updateString = "UPDATE todos SET title = ?, description = ?, category = ?, important = ?, deadline = "
                                 "?, recurrence_interval = ?, recurrence_count = ?, recurrence_start_date = ?, "
                                 "updated_at = ?, synced = ? WHERE id = ?";
    updateQuery.prepare(updateString);
    updateQuery.addBindValue(title);
    updateQuery.addBindValue(description);
    updateQuery.addBindValue(category);
    updateQuery.addBindValue(important);
    updateQuery.addBindValue(deadline.toString(Qt::ISODate));
    updateQuery.addBindValue(recurrenceInterval);
    updateQuery.addBindValue(recurrenceCount);
    updateQuery.addBindValue(recurrenceStartDate.toString(Qt::ISODate));
    updateQuery.addBindValue(now.toString(Qt::ISODate));
    updateQuery.addBindValue(2); // synced
    updateQuery.addBindValue(id);

    if (!updateQuery.exec()) {
        qCritical() << "更新待办事项到数据库失败:" << updateQuery.lastError().text();
        return false;
    }

    if (updateQuery.numRowsAffected() == 0) {
        qWarning() << "未找到ID为" << id << "的待办事项";
        return false;
    }

    qDebug() << "成功更新待办事项，ID:" << id;
    return true;
}

/**
 * @brief 更新待办事项
 * @param todos 待办事项容器引用
 * @param item 待办事项对象引用(已经改好的)
 * @return 更新是否成功
 */
bool TodoDataStorage::更新待办(TodoList &todos, TodoItem &item) {
    QSqlDatabase db = m_database.getDatabase();
    if (!db.isOpen()) {
        qCritical() << "数据库未打开，无法更新待办事项";
        return false;
    }

    // 更新数据库中的待办事项
    QSqlQuery updateQuery(db);
    const QString updateString =
        "UPDATE todos SET title = ?, description = ?, category = ?, important = ?, deadline = ?, "
        "recurrence_interval = ?, recurrence_count = ?, recurrence_start_date = ?, "
        "is_completed = ?, completed_at = ?, is_deleted = ?, deleted_at = ?, "
        "updated_at = ?, synced = ? WHERE id = ?";

    updateQuery.prepare(updateString);
    updateQuery.addBindValue(item.title());
    updateQuery.addBindValue(item.description());
    updateQuery.addBindValue(item.category());
    updateQuery.addBindValue(item.important());
    updateQuery.addBindValue(item.deadline().toString(Qt::ISODate));
    updateQuery.addBindValue(item.recurrenceInterval());
    updateQuery.addBindValue(item.recurrenceCount());
    updateQuery.addBindValue(item.recurrenceStartDate().toString(Qt::ISODate));
    updateQuery.addBindValue(item.isCompleted());
    updateQuery.addBindValue(item.completedAt().toString(Qt::ISODate));
    updateQuery.addBindValue(item.isDeleted());
    updateQuery.addBindValue(item.deletedAt().toString(Qt::ISODate));
    updateQuery.addBindValue(item.updatedAt().toString(Qt::ISODate));
    updateQuery.addBindValue(item.synced());
    updateQuery.addBindValue(item.id());

    if (!updateQuery.exec()) {
        qCritical() << "更新待办事项到数据库失败:" << updateQuery.lastError().text();
        return false;
    }

    if (updateQuery.numRowsAffected() == 0) {
        qWarning() << "未找到ID为" << item.id() << "的待办事项";
        return false;
    }

    qDebug() << "成功更新待办事项，ID:" << item.id();
    return true;
}

/**
 * @brief 回收待办事项
 * @param todos 待办事项容器引用
 * @param id 待办事项ID
 * @return 回收是否成功
 */
bool TodoDataStorage::回收待办(TodoList &todos, int id) {
    QSqlDatabase db = m_database.getDatabase();
    if (!db.isOpen()) {
        qCritical() << "数据库未打开，无法回收待办事项";
        return false;
    }

    QDateTime now = QDateTime::currentDateTime();

    // 更新数据库中的待办事项（标记为已删除）
    QSqlQuery updateQuery(db);
    const QString updateString = "UPDATE todos SET is_deleted = ?, deleted_at = ?, synced = ? WHERE id = ?";
    updateQuery.prepare(updateString);

    updateQuery.addBindValue(true);
    updateQuery.addBindValue(now.toString(Qt::ISODate));
    updateQuery.addBindValue(2); // synced
    updateQuery.addBindValue(id);

    if (!updateQuery.exec()) {
        qCritical() << "回收待办事项失败:" << updateQuery.lastError().text();
        return false;
    }

    if (updateQuery.numRowsAffected() == 0) {
        qWarning() << "未找到ID为" << id << "的待办事项";
        return false;
    }

    qDebug() << "成功回收待办事项，ID:" << id;
    return true;
}

/**
 * @brief 永久删除待办事项
 * @param todos 待办事项容器引用
 * @param id 待办事项ID
 */
bool TodoDataStorage::删除待办(TodoList &todos, int id) {
    bool success = false;
    try {
        QSqlDatabase db = m_database.getDatabase();
        if (!db.isOpen()) {
            qCritical() << "数据库未打开，无法永久删除待办事项";
            return false;
        }

        // 从数据库中永久删除待办事项
        QSqlQuery deleteQuery(db);
        const QString deleteString = "DELETE FROM todos WHERE id = ?";
        deleteQuery.prepare(deleteString);
        deleteQuery.addBindValue(id);

        if (!deleteQuery.exec()) {
            qCritical() << "永久删除待办事项失败:" << deleteQuery.lastError().text();
            return false;
        }

        if (deleteQuery.numRowsAffected() == 0) {
            qWarning() << "未找到ID为" << id << "的待办事项，无法删除";
            return false;
        }

        qDebug() << "成功永久删除待办事项，ID:" << id;
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
 * @brief 软删除待办事项
 * @param todos 软删除待办容器引用
 * @param id 待办事项ID
 * @return 回收是否成功
 */
bool TodoDataStorage::软删除待办(TodoList &todos, int id) {
    QSqlDatabase db = m_database.getDatabase();
    if (!db.isOpen()) {
        qCritical() << "数据库未打开，无法软删除待办事项";
        return false;
    }

    QDateTime now = QDateTime::currentDateTime();

    // 更新数据库中的待办事项（标记synced为已删除(3)）
    QSqlQuery updateQuery(db);
    const QString updateString = "UPDATE todos SET synced = ? WHERE id = ?";
    updateQuery.prepare(updateString);

    updateQuery.addBindValue(3); // synced
    updateQuery.addBindValue(id);

    if (!updateQuery.exec()) {
        qCritical() << "软删除待办事项失败:" << updateQuery.lastError().text();
        return false;
    }

    if (updateQuery.numRowsAffected() == 0) {
        qWarning() << "未找到ID为" << id << "的待办事项";
        return false;
    }

    qDebug() << "成功软删除待办事项，ID:" << id;
    return true;
}

/**
 * @brief 导入类别从JSON
 * @param categories 类别列表引用
 * @param categoriesArray JSON类别数组
 * @param source 来源（服务器或本地备份）
 * @param resolution 冲突解决策略
 */
bool TodoDataStorage::导入类别从JSON(TodoList &todos, const QJsonArray &todosArray, ImportSource source,
                                     ConflictResolution resolution) {
    bool success = true;

    QSqlDatabase db = m_database.getDatabase();
    if (!db.isOpen()) {
        qCritical() << "数据库未打开，无法导入类别";
        return false;
    }

    // 构建现有类别索引（按 uuid 与 name）
    QHash<QString, TodoItem *> uuidIndex;
    for (auto &item : todos) {
        if (item) {
            uuidIndex.insert(item->uuid().toString(QUuid::WithoutBraces), item.get());
        }
    }

    if (!db.transaction()) {
        qCritical() << "无法开启事务以导入类别:" << db.lastError().text();
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

            QUuid userUuid = QUuid::fromString(obj.value("user_uuid").toString());
            if (userUuid.isNull()) {
                qWarning() << "跳过无效待办（user_uuid 无效）";
                ++skipCount;
                continue;
            }
            QUuid uuid = obj.contains("uuid") ? QUuid::fromString(obj.value("uuid").toString()) : QUuid::createUuid();
            if (uuid.isNull())
                uuid = QUuid::createUuid();
            QString title = obj.value("title").toString();
            QString description = obj.value("description").toString();
            QString category = obj.value("category").toString();
            bool important = obj.value("important").toBool();
            QDateTime deadline = QDateTime::fromString(obj["deadline"].toString(), Qt::ISODate);
            int recurrenceInterval = obj.value("recurrenceInterval").toInt();
            int recurrenceCount = obj.value("recurrenceCount").toInt();
            QDate recurrenceStartDate = QDate::fromString(obj["recurrenceStartDate"].toString(), Qt::ISODate);
            bool isCompleted = obj.value("isCompleted").toBool();
            QDateTime completedAt = QDateTime::fromString(obj["completed_at"].toString(), Qt::ISODate);
            bool isDeleted = obj.value("isDeleted").toBool();
            QDateTime deletedAt = QDateTime::fromString(obj["deleted_at"].toString(), Qt::ISODate);
            QDateTime createdAt = obj.contains("created_at")
                                      ? QDateTime::fromString(obj.value("created_at").toString(), Qt::ISODate)
                                      : QDateTime::currentDateTime();
            if (!createdAt.isValid())
                createdAt = QDateTime::currentDateTime();
            QDateTime updatedAt = obj.contains("updated_at")
                                      ? QDateTime::fromString(obj.value("updated_at").toString(), Qt::ISODate)
                                      : createdAt;
            if (!updatedAt.isValid())
                updatedAt = createdAt;

            // 构造临时 incoming 对象（不立即放入列表）
            TodoItem incoming(                          //
                -1,                                     //
                uuid,                                   //
                userUuid,                               //
                title,                                  //
                description,                            //
                category,                               //
                important,                              //
                deadline,                               //
                recurrenceInterval,                     //
                recurrenceCount,                        //
                recurrenceStartDate,                    //
                isCompleted,                            //
                completedAt,                            //
                isDeleted,                              //
                deletedAt,                              //
                createdAt,                              //
                updatedAt,                              //
                source == ImportSource::Server ? 0 : 1, //
                this                                    //
            );

            // 查找现有
            TodoItem *existing = uuidIndex.value(uuid.toString(QUuid::WithoutBraces), nullptr);

            ConflictResolution action = 评估冲突(existing, incoming, resolution);
            if (action == ConflictResolution::Skip) {
                ++skipCount;
                continue;
            }

            if (action == ConflictResolution::Insert) {
                auto newItem = std::make_unique<TodoItem>(
                    incoming.id(),
                    incoming.uuid(),
                    incoming.userUuid(),
                    incoming.title(),
                    incoming.description(),
                    incoming.category(),
                    incoming.important(),
                    incoming.deadline(),
                    incoming.recurrenceInterval(),
                    incoming.recurrenceCount(),
                    incoming.recurrenceStartDate(),
                    incoming.isCompleted(),
                    incoming.completedAt(),
                    incoming.isDeleted(),
                    incoming.deletedAt(),
                    incoming.createdAt(),
                    incoming.updatedAt(),
                    incoming.synced(),
                    this
                );
                TodoItem* itemPtr = newItem.get();
                bool success = 新增待办(todos, std::move(newItem));
                    // 更新索引
                    uuidIndex.insert(uuid.toString(QUuid::WithoutBraces), itemPtr);
                    ++insertCount;

            } else if (action == ConflictResolution::Overwrite && existing) {
                QSqlQuery updateQuery(db);
                updateQuery.prepare( //
                    "UPDATE todos SET user_uuid = ?, title = ?, description = ?,category = ?, important = ?, "
                    "deadline = ?, recurrence_interval = ?, recurrence_count = ?, recurrence_start_date = ?, "
                    "is_completed = ?, completed_at = ?, is_deleted = ?, deleted_at = ?, created_at = ?, "
                    "updated_at = ?, synced = ? WHERE uuid = ?");
                updateQuery.addBindValue(userUuid.toString());
                updateQuery.addBindValue(title);
                updateQuery.addBindValue(description);
                updateQuery.addBindValue(category);
                updateQuery.addBindValue(important);
                updateQuery.addBindValue(deadline.toString(Qt::ISODate));
                updateQuery.addBindValue(recurrenceInterval);
                updateQuery.addBindValue(recurrenceCount);
                updateQuery.addBindValue(recurrenceStartDate.toString(Qt::ISODate));
                updateQuery.addBindValue(isCompleted);
                updateQuery.addBindValue(completedAt.toString(Qt::ISODate));
                updateQuery.addBindValue(isDeleted);
                updateQuery.addBindValue(deletedAt.toString(Qt::ISODate));
                updateQuery.addBindValue(createdAt.toString(Qt::ISODate));
                updateQuery.addBindValue(updatedAt.toString(Qt::ISODate));
                updateQuery.addBindValue(source == ImportSource::Server ? 0 : (existing->synced() == 1 ? 1 : 2));
                updateQuery.addBindValue(existing->uuid().toString());
                if (!updateQuery.exec()) {
                    qCritical() << "更新待办失败(uuid=" << existing->uuid() << "):" << updateQuery.lastError().text();
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
                existing->setIsDeleted(isDeleted);
                existing->setDeletedAt(deletedAt);
                existing->setCreatedAt(createdAt);
                existing->setUpdatedAt(updatedAt);
                existing->setSynced(source == ImportSource::Server ? 0 : (existing->synced() == 1 ? 1 : 2));
                ++updateCount;
            }
        }

        if (success) {
            if (!db.commit()) {
                qCritical() << "提交事务失败:" << db.lastError().text();
                success = false;
                db.rollback();
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
 * @brief 评估冲突并决定动作
 * @param existing 现有类别指针（若无则为nullptr）
 * @param incoming 新导入的类别引用
 * @param resolution 冲突解决策略
 * @return 决定的冲突动作
 */
TodoDataStorage::ConflictResolution TodoDataStorage::评估冲突( //
    const TodoItem *existing,                                  //
    const TodoItem &incoming,                                  //
    ConflictResolution resolution) const                       //
{
    // 无冲突，直接插入
    if (!existing) {
        return ConflictResolution::Insert;
    }

    switch (resolution) {
    case Skip:
        return ConflictResolution::Skip; // 直接跳过
    case Overwrite:
        return ConflictResolution::Overwrite; // 强制覆盖
    case Merge: {
        // Merge: 选择更新时间新的那条；若相等则保留旧
        if (incoming.updatedAt() > existing->updatedAt()) {
            return ConflictResolution::Overwrite;
        } else {
            return ConflictResolution::Skip;
        }
    }
    case Insert:
        return ConflictResolution::Insert;
    default:
        return ConflictResolution::Skip;
    }
}

/**
 * @brief 获取最后插入行ID
 * @param db 数据库连接引用
 * @return 最后插入行ID，失败返回-1
 */
int TodoDataStorage::获取最后插入行ID(QSqlDatabase &db) const {
    if (!db.isOpen())
        return -1;
    QSqlQuery idQuery(db);
    if (!idQuery.exec("SELECT last_insert_rowid()")) {
        qWarning() << "执行 last_insert_rowid 查询失败:" << idQuery.lastError().text();
        return -1;
    }
    if (idQuery.next()) {
        return idQuery.value(0).toInt();
    }
    return -1;
}

/**
 * @brief 根据排序类型和顺序生成SQL的ORDER BY子句
 * @param sortType 排序类型（0-创建时间，1-截止日期，2-重要性，3-标题）
 * @param descending 是否降序
 * @return SQL的ORDER BY子句
 */
QString TodoDataStorage::构建SQL排序语句(int sortType, bool descending) {
    QString order;
    switch (sortType) {
    case 1: // 截止日期
        // NULL 截止日期 放最后
        order = "ORDER BY (deadline IS NULL) ASC, deadline";
        break;
    case 2:                                                 // 重要性
        order = "ORDER BY important DESC, created_at DESC"; // 重要性 固定优先降序
        if (descending) {
            // 如果用户希望 overall descending，则在 importance 模式下可以理解为反向：再追加一个标识
            // 这里保持简单：用户 descending=true 时调整主列方向
            order = "ORDER BY important ASC, created_at DESC";
        }
        return order; // 已返回
    case 3:           // 标题
        order = "ORDER BY title COLLATE NOCASE";
        break;
    case 0: // 创建时间(默认)
    default:
        order = "ORDER BY created_at";
        break;
    }
    if (descending) {
        order += " DESC";
    } else {
        order += " ASC";
    }
    return order;
}

/**
 * @brief 查询待办ID列表
 * @param opt 查询选项
 * @return 符合条件的待办ID列表
 */
QList<int> TodoDataStorage::查询待办ID列表(const QueryOptions &opt) {
    QList<int> indexs;
    QSqlDatabase db = m_database.getDatabase();
    if (!db.isOpen()) {
        qCritical() << "数据库未打开，无法查询待办ID列表";
        return indexs;
    }

    QString sql = "SELECT id FROM todos WHERE 1=1";

    // 分类
    if (!opt.category.isEmpty() && opt.category != "全部" && opt.category != "all") {
        sql += " AND category = :category";
    }

    // 状态与删除逻辑
    // recycle: is_deleted=1
    // 其它：is_deleted=0
    if (opt.statusFilter == "recycle") {
        sql += " AND is_deleted = 1";
    } else {
        sql += " AND is_deleted = 0";
        if (opt.statusFilter == "todo") {
            sql += " AND is_completed = 0";
        } else if (opt.statusFilter == "done") {
            sql += " AND is_completed = 1";
        } else if (opt.statusFilter == "all") {
            // is_deleted=0 已限制，全部不过滤完成状态
        }
    }

    // 日期范围 (deadline)
    if (opt.dateFilterEnabled) {
        sql += " AND deadline IS NOT NULL";
        if (opt.dateStart.isValid()) {
            sql += " AND date(deadline) >= :dateStart";
        }
        if (opt.dateEnd.isValid()) {
            sql += " AND date(deadline) <= :dateEnd";
        }
    }

    // 搜索 (LIKE)
    bool doSearch = !opt.searchText.isEmpty();
    if (doSearch) {
        sql += " AND (title LIKE :kw OR description LIKE :kw OR category LIKE :kw)";
    }

    // 排序
    sql += ' ' + 构建SQL排序语句(opt.sortType, opt.descending);

    // 分页
    if (opt.limit > 0) {
        sql += " LIMIT :limit";
        if (opt.offset > 0) {
            sql += " OFFSET :offset";
        }
    }

    QSqlQuery query(db);
    if (!query.prepare(sql)) {
        qCritical() << "准备查询失败:" << query.lastError().text() << sql;
        return indexs;
    }

    if (!opt.category.isEmpty() && opt.category != "全部" && opt.category != "all") {
        query.bindValue(":category", opt.category);
    }
    if (opt.dateFilterEnabled) {
        if (opt.dateStart.isValid()) {
            query.bindValue(":dateStart", opt.dateStart.toString(Qt::ISODate));
        }
        if (opt.dateEnd.isValid()) {
            query.bindValue(":dateEnd", opt.dateEnd.toString(Qt::ISODate));
        }
    }
    if (doSearch) {
        QString pattern = '%' + opt.searchText + '%';
        query.bindValue(":kw", pattern);
    }
    if (opt.limit > 0) {
        query.bindValue(":limit", opt.limit);
        if (opt.offset > 0) {
            query.bindValue(":offset", opt.offset);
        }
    }

    if (!query.exec()) {
        qCritical() << "执行查询失败:" << query.lastError().text() << sql;
        return indexs;
    }

    while (query.next()) {
        indexs << query.value(0).toInt();
    }
    return indexs;
}
