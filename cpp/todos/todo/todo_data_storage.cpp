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

#include "foundation/config.h"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QUuid>
#include <algorithm>
#include <optional>
#include <unordered_map>

TodoDataStorage::TodoDataStorage(QObject *parent)
    : QObject(parent),                                    // 父对象
      m_setting(Setting::GetInstance()),                 // 设置
      m_database(Database::GetInstance())  // 数据库管理器
{
    // 确保数据库已初始化
    if (!m_database.initializeDatabase()) {
        qCritical() << "TodoDataStorage: 数据库初始化失败";
    }
}

TodoDataStorage::~TodoDataStorage() {
}

/**
 * @brief 从本地存储加载待办事项
 * @param todos 待办事项容器引用
 * @return 加载是否成功
 */
bool TodoDataStorage::loadFromLocalStorage(std::vector<std::unique_ptr<TodoItem>> &todos) {
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
        const QString queryString = "SELECT id, uuid, user_uuid, title, description, category, important, deadline, recurrence_interval, recurrence_count, recurrence_start_date, is_completed, completed_at, is_deleted, deleted_at, created_at, updated_at, last_modified_at, synced FROM todos ORDER BY id";
        
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
            bool synced = query.value("synced").toBool();

            auto item = std::make_unique<TodoItem>(
                id,                   // 唯一标识符
                uuid,                 // 唯一标识符（UUID）
                userUuid,             // 用户UUID
                title,                // 待办事项标题
                description,          // 待办事项详细描述
                category,             // 待办事项分类
                important,            // 重要程度
                deadline,             // 截止日期
                recurrenceInterval,   // 重复间隔
                recurrenceCount,      // 重复次数
                recurrenceStartDate,  // 重复开始日期
                isCompleted,          // 是否已完成
                completedAt,          // 完成时间
                isDeleted,            // 是否已删除
                deletedAt,            // 删除时间
                createdAt,            // 创建时间
                updatedAt,            // 更新时间
                lastModifiedAt,       // 最后修改时间
                synced,               // 是否已同步
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
bool TodoDataStorage::saveToLocalStorage(const std::vector<std::unique_ptr<TodoItem>> &todos) {
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
        const QString insertString = "INSERT INTO todos (id, uuid, user_uuid, title, description, category, important, deadline, recurrence_interval, recurrence_count, recurrence_start_date, is_completed, completed_at, is_deleted, deleted_at, created_at, updated_at, last_modified_at, synced) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
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
 * @brief 从TOML表创建新的TodoItem
 * @param todoTable TOML表引用
 * @param newId 新分配的ID
 * @return 创建的TodoItem智能指针，失败时返回nullptr
 */
std::unique_ptr<TodoItem> TodoDataStorage::createTodoItemFromToml(const toml::table &todoTable, int newId) {
    try {
        // 解析必要字段
        auto uuidNode = todoTable["uuid"];
        if (!uuidNode || !uuidNode.is_string()) {
            return nullptr;
        }

        QString uuidStr = QString::fromStdString(uuidNode.value_or(""));
        QUuid itemUuid = QUuid::fromString(uuidStr);
        if (itemUuid.isNull()) {
            return nullptr;
        }

        // 解析其他字段，提供默认值
        auto getStringValue = [&todoTable](const char *key, const QString &defaultValue = QString()) -> QString {
            auto node = todoTable[key];
            return node && node.is_string() ? QString::fromStdString(node.value_or("")) : defaultValue;
        };

        auto getBoolValue = [&todoTable](const char *key, bool defaultValue = false) -> bool {
            auto node = todoTable[key];
            return node && node.is_boolean() ? node.value_or(defaultValue) : defaultValue;
        };

        auto getIntValue = [&todoTable](const char *key, int defaultValue = 0) -> int {
            auto node = todoTable[key];
            if (node && node.is_integer()) {
                return static_cast<int>(node.value_or(defaultValue));
            }
            return defaultValue;
        };

        auto getDateTimeValue = [&todoTable](const char *key, const QDateTime &defaultValue = QDateTime::fromString(
                                                                  "1970-01-01T00:00:00", Qt::ISODate)) -> QDateTime {
            auto node = todoTable[key];
            if (node && node.is_string()) {
                QString dateStr = QString::fromStdString(node.value_or("1970-01-01T00:00:00"));
                QDateTime result = QDateTime::fromString(dateStr, Qt::ISODate);
                return result.isValid() ? result : defaultValue;
            }
            return defaultValue;
        };

        auto getDateValue = [&todoTable](const char *key, const QDate &defaultValue =
                                                              QDate::fromString("1970-01-01", Qt::ISODate)) -> QDate {
            auto node = todoTable[key];
            if (node && node.is_string()) {
                QString dateStr = QString::fromStdString(node.value_or("1970-01-01"));
                QDate result = QDate::fromString(dateStr, Qt::ISODate);
                return result.isValid() ? result : defaultValue;
            }
            return defaultValue;
        };

        // 创建TodoItem
        auto item = std::make_unique<TodoItem>(
            newId,                                                            // id
            itemUuid,                                                         // uuid
            QUuid::fromString(getStringValue("userUuid")),                    // userUuid
            getStringValue("title"),                                          // title
            getStringValue("description"),                                    // description
            getStringValue("category", "未分类"),                             // category
            getBoolValue("important"),                                        // important
            getDateTimeValue("deadline"),                                     // deadline
            getIntValue("recurrenceInterval"),                                // recurrenceInterval
            getIntValue("recurrenceCount", -1),                               // recurrenceCount
            getDateValue("recurrenceStartDate"),                              // recurrenceStartDate
            getBoolValue("isCompleted"),                                      // isCompleted
            getDateTimeValue("completedAt"),                                  // completedAt
            getBoolValue("isDeleted"),                                        // isDeleted
            getDateTimeValue("deletedAt"),                                    // deletedAt
            getDateTimeValue("createdAt", QDateTime::currentDateTime()),      // createdAt
            getDateTimeValue("updatedAt", QDateTime::currentDateTime()),      // updatedAt
            getDateTimeValue("lastModifiedAt", QDateTime::currentDateTime()), // lastModifiedAt
            getBoolValue("synced"),                                           // synced
            this                                                              // parent
        );

        return item;
    } catch (const std::exception &e) {
        qWarning() << "创建TodoItem时发生异常:" << e.what();
        return nullptr;
    }
}

/**
 * @brief 从TOML表更新现有的TodoItem
 * @param item 要更新的TodoItem指针
 * @param todoTable TOML表引用
 */
void TodoDataStorage::updateTodoItemFromToml(TodoItem *item, const toml::table &todoTable) {
    if (!item) {
        return;
    }

    try {
        // 辅助函数
        auto getStringValue = [&todoTable](const char *key) -> std::optional<QString> {
            auto node = todoTable[key];
            if (node && node.is_string()) {
                return QString::fromStdString(node.value_or(""));
            }
            return std::nullopt;
        };

        auto getBoolValue = [&todoTable](const char *key) -> std::optional<bool> {
            auto node = todoTable[key];
            if (node && node.is_boolean()) {
                return node.value_or(false);
            }
            return std::nullopt;
        };

        auto getIntValue = [&todoTable](const char *key) -> std::optional<int> {
            auto node = todoTable[key];
            if (node && node.is_integer()) {
                return static_cast<int>(node.value_or(0));
            }
            return std::nullopt;
        };

        auto getDateTimeValue = [&todoTable](const char *key) -> std::optional<QDateTime> {
            auto node = todoTable[key];
            if (node && node.is_date_time()) {
                QDateTime result = QDateTime::fromString(node.value_or("1970-01-01T00:00:00"), Qt::ISODate);
                if (result.isValid()) {
                    return result;
                }
            }
            return std::nullopt;
        };

        auto getDateValue = [&todoTable](const char *key) -> std::optional<QDate> {
            auto node = todoTable[key];
            if (node && node.is_date()) {
                QDate result = QDate::fromString(node.value_or("1970-01-01"), "yyyy-MM-dd");
                if (result.isValid()) {
                    return result;
                }
            }
            return std::nullopt;
        };

        // 更新字段（只更新存在的字段）
        if (auto value = getStringValue("title")) {
            item->setTitle(*value);
        }
        if (auto value = getStringValue("description")) {
            item->setDescription(*value);
        }
        if (auto value = getStringValue("category")) {
            item->setCategory(*value);
        }
        if (auto value = getBoolValue("important")) {
            item->setImportant(*value);
        }
        if (auto value = getDateTimeValue("deadline")) {
            item->setDeadline(*value);
        }
        if (auto value = getIntValue("recurrenceInterval")) {
            item->setRecurrenceInterval(*value);
        }
        if (auto value = getIntValue("recurrenceCount")) {
            item->setRecurrenceCount(*value);
        }
        if (auto value = getDateValue("recurrenceStartDate")) {
            item->setRecurrenceStartDate(*value);
        }
        if (auto value = getBoolValue("isCompleted")) {
            item->setIsCompleted(*value);
        }
        if (auto value = getDateTimeValue("completedAt")) {
            item->setCompletedAt(*value);
        }
        if (auto value = getBoolValue("isDeleted")) {
            item->setIsDeleted(*value);
        }
        if (auto value = getDateTimeValue("deletedAt")) {
            item->setDeletedAt(*value);
        }
        if (auto value = getDateTimeValue("createdAt")) {
            item->setCreatedAt(*value);
        }
        if (auto value = getDateTimeValue("updatedAt")) {
            item->setUpdatedAt(*value);
        }
        if (auto value = getDateTimeValue("lastModifiedAt")) {
            item->setLastModifiedAt(*value);
        }
        if (auto value = getBoolValue("synced")) {
            item->setSynced(*value);
        }
        if (auto value = getStringValue("userUuid")) {
            QUuid userUuid = QUuid::fromString(*value);
            if (!userUuid.isNull()) {
                item->setUserUuid(userUuid);
            }
        }

    } catch (const std::exception &e) {
        qWarning() << "更新TodoItem时发生异常:" << e.what();
    }
}


/**
 * @brief 添加新的待办事项
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
std::unique_ptr<TodoItem> TodoDataStorage::addTodo(const QString &title, const QString &description, const QString &category, bool important, const QDateTime &deadline, int recurrenceInterval, int recurrenceCount, const QDate &recurrenceStartDate) {
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

    QDateTime now = QDateTime::currentDateTime();
    QUuid newUuid = QUuid::createUuid();
    QUuid userUuid = QUuid::createUuid(); // 这里应该设置为当前用户的UUID
    int newId = maxId + 1;

    // 创建新的待办事项
    auto newTodo = std::make_unique<TodoItem>(
        newId,                                                            // id
        newUuid,                                                          // uuid
        userUuid,                                                         // userUuid
        title,                                                            // title
        description,                                                      // description
        category,                                                         // category
        important,                                                        // important
        deadline,                                                         // deadline
        recurrenceInterval,                                               // recurrenceInterval
        recurrenceCount,                                                  // recurrenceCount
        recurrenceStartDate,                                              // recurrenceStartDate
        false,                                                            // isCompleted
        QDateTime::fromString("1970-01-01T00:00:00", Qt::ISODate),       // completedAt
        false,                                                            // isDeleted
        QDateTime::fromString("1970-01-01T00:00:00", Qt::ISODate),       // deletedAt
        now,                                                              // createdAt
        now,                                                              // updatedAt
        now,                                                              // lastModifiedAt
        false,                                                            // synced
        this                                                              // parent
    );

    // 插入到数据库
    QSqlQuery insertQuery(db);
    const QString insertString = "INSERT INTO todos (id, uuid, user_uuid, title, description, category, important, deadline, recurrence_interval, recurrence_count, recurrence_start_date, is_completed, completed_at, is_deleted, deleted_at, created_at, updated_at, last_modified_at, synced) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
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

    qDebug() << "成功添加待办事项到数据库，ID:" << newId;
    return newTodo;
}

/**
 * @brief 更新待办事项
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
bool TodoDataStorage::updateTodo(int id, const QString &title, const QString &description, const QString &category, bool important, const QDateTime &deadline, int recurrenceInterval, int recurrenceCount, const QDate &recurrenceStartDate) {
    QSqlDatabase db = m_database.getDatabase();
    if (!db.isOpen()) {
        qCritical() << "数据库未打开，无法更新待办事项";
        return false;
    }

    QDateTime now = QDateTime::currentDateTime();

    // 更新数据库中的待办事项
    QSqlQuery updateQuery(db);
    const QString updateString = "UPDATE todos SET title = ?, description = ?, category = ?, important = ?, deadline = ?, recurrence_interval = ?, recurrence_count = ?, recurrence_start_date = ?, updated_at = ?, last_modified_at = ?, synced = ? WHERE id = ?";
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
    updateQuery.addBindValue(false); // synced
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
 * @brief 删除待办事项
 * @param id 待办事项ID
 * @return 删除是否成功
 */
bool TodoDataStorage::deleteTodo(int id) {
    QSqlDatabase db = m_database.getDatabase();
    if (!db.isOpen()) {
        qCritical() << "数据库未打开，无法删除待办事项";
        return false;
    }

    QDateTime now = QDateTime::currentDateTime();

    // 更新数据库中的待办事项（标记为已删除）
    QSqlQuery updateQuery(db);
    const QString updateString = "UPDATE todos SET is_deleted = ?, deleted_at = ?, last_modified_at = ?, synced = ? WHERE id = ?";
    updateQuery.prepare(updateString);
    
    updateQuery.addBindValue(true);
    updateQuery.addBindValue(now.toString(Qt::ISODate));
    updateQuery.addBindValue(now.toString(Qt::ISODate));
    updateQuery.addBindValue(false); // synced
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
 * @brief 从TOML表导入待办事项（指定冲突解决策略）
 * @param table TOML表引用
 * @param todos 待办事项容器引用
 * @param resolution 冲突解决策略
 * @return 导入是否成功
 */
bool TodoDataStorage::importFromToml(const toml::table &table, std::vector<std::unique_ptr<TodoItem>> &todos,
                                     ConflictResolution resolution) {
    bool success = true;
    int importedCount = 0;
    int skippedCount = 0;
    int conflictCount = 0;

    try {
        // 检查TOML表中是否包含todos节点
        auto todosNode = table["todos"];
        if (!todosNode) {
            qWarning() << "TOML文件中未找到todos节点";
            return false;
        }

        // 获取待办事项数量
        auto sizeNode = todosNode["size"];
        if (!sizeNode || !sizeNode.is_integer()) {
            qWarning() << "TOML文件中todos.size字段无效";
            return false;
        }

        int todoCount = sizeNode.value_or(0);
        if (todoCount <= 0) {
            qDebug() << "TOML文件中没有待办事项需要导入";
            emit importCompleted(0, 0, 0);
            return true;
        }

        // 获取当前最大ID，用于分配新ID
        int maxId = 0;
        for (const auto &item : todos) {
            if (item->id() > maxId) {
                maxId = item->id();
            }
        }

        // 创建UUID到现有项目的映射，用于冲突检测
        std::unordered_map<QString, TodoItem *> existingUuids;
        for (const auto &item : todos) {
            existingUuids[item->uuid().toString()] = item.get();
        }

        // 逐个导入待办事项
        for (int i = 0; i < todoCount; ++i) {
            auto todoNode = todosNode[std::to_string(i)];
            if (!todoNode || !todoNode.is_table()) {
                qWarning() << "跳过无效的待办事项记录（索引" << i << "）：不是有效的表格";
                skippedCount++;
                continue;
            }

            auto todoTable = *todoNode.as_table();

            // 验证必要字段
            auto uuidNode = todoTable["uuid"];
            if (!uuidNode || !uuidNode.is_string()) {
                qWarning() << "跳过无效的待办事项记录（索引" << i << "）：缺少有效的uuid字段";
                skippedCount++;
                continue;
            }

            QString uuidStr = QString::fromStdString(uuidNode.value_or(""));
            QUuid itemUuid = QUuid::fromString(uuidStr);
            if (itemUuid.isNull()) {
                qWarning() << "跳过无效的待办事项记录（索引" << i << "）：uuid格式无效";
                skippedCount++;
                continue;
            }

            // 检查冲突
            auto existingItem = existingUuids.find(uuidStr);
            if (existingItem != existingUuids.end()) {
                conflictCount++;

                switch (resolution) {
                case ConflictResolution::Skip:
                    qDebug() << "跳过冲突的待办事项（UUID:" << uuidStr << "）";
                    skippedCount++;
                    continue;

                case ConflictResolution::Overwrite: {
                    // 更新现有项目的数据
                    TodoItem *existing = existingItem->second;
                    updateTodoItemFromToml(existing, todoTable);
                    qDebug() << "覆盖现有待办事项（UUID:" << uuidStr << "）";
                    importedCount++;
                    continue;
                }

                case ConflictResolution::Merge: {
                    // 比较时间戳，保留较新的版本
                    auto updatedAtNode = todoTable["updatedAt"];
                    if (updatedAtNode && updatedAtNode.is_string()) {
                        QDateTime importUpdatedAt =
                            QDateTime::fromString(QString::fromStdString(updatedAtNode.value_or("")), Qt::ISODate);

                        TodoItem *existing = existingItem->second;
                        if (importUpdatedAt > existing->updatedAt()) {
                            updateTodoItemFromToml(existing, todoTable);
                            qDebug() << "合并更新待办事项（UUID:" << uuidStr << "）";
                            importedCount++;
                        } else {
                            qDebug() << "保留现有待办事项（UUID:" << uuidStr << "）";
                            skippedCount++;
                        }
                    } else {
                        skippedCount++;
                    }
                    continue;
                }
                }
            }

            // 创建新的待办事项
            try {
                auto newItem = createTodoItemFromToml(todoTable, ++maxId);
                if (newItem) {
                    existingUuids[uuidStr] = newItem.get();
                    todos.push_back(std::move(newItem));
                    importedCount++;
                } else {
                    skippedCount++;
                }
            } catch (const std::exception &e) {
                qWarning() << "创建待办事项时发生异常（索引" << i << "）:" << e.what();
                skippedCount++;
            }
        }

        qDebug() << "TOML导入完成 - 导入:" << importedCount << "跳过:" << skippedCount << "冲突:" << conflictCount;
        emit importCompleted(importedCount, skippedCount, conflictCount);

    } catch (const std::exception &e) {
        qCritical() << "从TOML导入时发生异常:" << e.what();
        success = false;
    } catch (...) {
        qCritical() << "从TOML导入时发生未知异常";
        success = false;
    }

    return success;
}
