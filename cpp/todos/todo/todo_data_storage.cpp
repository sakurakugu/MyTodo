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

TodoDataStorage::~TodoDataStorage() {
}

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
            "updated_at, last_modified_at, synced FROM todos ORDER BY id";

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
            QDateTime lastModifiedAt = QDateTime::fromString(query.value("last_modified_at").toString(), Qt::ISODate);
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
                lastModifiedAt,                     // 最后修改时间
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
            "deleted_at, created_at, updated_at, last_modified_at, synced) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
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
            insertQuery.addBindValue(item->lastModifiedAt().toString(Qt::ISODate));
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
                                                    const QDate &recurrenceStartDate,QUuid userUuid) {
    QSqlDatabase db = m_database.getDatabase();
    if (!db.isOpen()) {
        qCritical() << "数据库未打开，无法添加待办事项";
        return nullptr;
    }

    // 获取当前最大ID
    QSqlQuery maxIdQuery(db);
    int maxId = 0;
    if (maxIdQuery.exec("SELECT MAX(id) FROM todos")) {
        if (maxIdQuery.next()) {
            maxId = maxIdQuery.value(0).toInt();
        }
    }

    int newId = maxId + 1;
    QDateTime now = QDateTime::currentDateTime();
    QUuid newUuid = QUuid::createUuid();
    QDateTime nullTime = QDateTime::fromString("1970-01-01T00:00:00", Qt::ISODate);

    // 创建新的待办事项
    auto newTodo = std::make_unique<TodoItem>( //
        newId,                                 // id
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
        now,                                   // lastModifiedAt
        1,                                     // synced
        this                                   // parent
    );

    // 插入到数据库
    QSqlQuery insertQuery(db);
    const QString insertString =
        "INSERT INTO todos (id, uuid, user_uuid, title, description, category, important, deadline, "
        "recurrence_interval, recurrence_count, recurrence_start_date, is_completed, completed_at, is_deleted, "
        "deleted_at, created_at, updated_at, last_modified_at, synced) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
        "?, ?, ?, ?, ?, ?)";
    insertQuery.prepare(insertString);

    insertQuery.addBindValue(newTodo->id());
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
    insertQuery.addBindValue(newTodo->lastModifiedAt().toString(Qt::ISODate));
    insertQuery.addBindValue(newTodo->synced());

    if (!insertQuery.exec()) {
        qCritical() << "插入待办事项到数据库失败:" << insertQuery.lastError().text();
        return nullptr;
    }

    todos.push_back(std::move(newTodo));

    qDebug() << "成功添加待办事项到数据库，ID:" << newId;
    return newTodo;
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
bool TodoDataStorage::更新待办(TodoList &todos, int id, const QString &title, const QString &description, const QString &category,
                               bool important, const QDateTime &deadline, int recurrenceInterval, int recurrenceCount,
                               const QDate &recurrenceStartDate) {
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
                                 "updated_at = ?, last_modified_at = ?, synced = ? WHERE id = ?";
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
 * @brief 回收待办事项
 * @param todos 待办事项容器引用
 * @param id 待办事项ID
 * @return 回收是否成功
 */
bool TodoDataStorage::回收待办(TodoList &todos, int id) {
    QSqlDatabase db = m_database.getDatabase();
    if (!db.isOpen()) {
        qCritical() << "数据库未打开，无法删除待办事项";
        return false;
    }

    QDateTime now = QDateTime::currentDateTime();

    // 更新数据库中的待办事项（标记为已删除）
    QSqlQuery updateQuery(db);
    const QString updateString =
        "UPDATE todos SET is_deleted = ?, deleted_at = ?, last_modified_at = ?, synced = ? WHERE id = ?";
    updateQuery.prepare(updateString);

    updateQuery.addBindValue(true);
    updateQuery.addBindValue(now.toString(Qt::ISODate));
    updateQuery.addBindValue(now.toString(Qt::ISODate));
    updateQuery.addBindValue(2); // synced
    updateQuery.addBindValue(id);

    if (!updateQuery.exec()) {
        qCritical() << "删除待办事项失败:" << updateQuery.lastError().text();
        return false;
    }

    if (updateQuery.numRowsAffected() == 0) {
        qWarning() << "未找到ID为" << id << "的待办事项";
        return false;
    }

    qDebug() << "成功删除待办事项，ID:" << id;
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
 * @brief 获取下一个可用ID
 * @param categories 类别列表
 * @return 下一个可用ID
 */
int TodoDataStorage::获取下一个可用ID(const TodoList &todos) const {
    int maxId = 0;
    for (const auto &todo : todos) {
        if (todo && todo->id() > maxId) {
            maxId = todo->id();
        }
    }
    return maxId + 1;
}