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
#include <QUuid>
#include <algorithm>
#include <optional>
#include <unordered_map>

TodoDataStorage::TodoDataStorage(QObject *parent)
    : QObject(parent),                  // 父对象
      m_setting(Setting::GetInstance()) // 设置
{
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

        // 从设置中加载数据
        int count = m_setting.get("todos/size", 0).toInt();

        // 获取当前最大的id，用于为没有id的项目分配新id
        int maxId = 0;
        for (int i = 0; i < count; ++i) {
            QString prefix = QString("todos/%1/").arg(i);
            int currentId = m_setting.get(prefix + "id").toInt();
            if (currentId > maxId) {
                maxId = currentId;
            }
        }

        for (int i = 0; i < count; ++i) {
            QString prefix = QString("todos/%1/").arg(i);

            // 验证必要字段
            if (!m_setting.contains(prefix + "uuid")) {
                QUuid uuid = QUuid::createUuid();
                m_setting.save(prefix + "uuid", uuid.toString());
            }

            // 获取id，如果没有或为0，则分配一个新的唯一id
            int itemId = m_setting.get(prefix + "id").toInt();
            if (itemId == 0) {
                itemId = ++maxId;
                // 保存新分配的id到设置中
                m_setting.save(prefix + "id", itemId);
            }

            auto item = std::make_unique<TodoItem>(
                itemId,                                                           // 唯一标识符
                QUuid::fromString(m_setting.get(prefix + "uuid").toString()),     // 唯一标识符（UUID）
                QUuid::fromString(m_setting.get(prefix + "userUuid").toString()), // 用户UUID
                m_setting.get(prefix + "title").toString(),                       // 待办事项标题
                m_setting.get(prefix + "description").toString(),                 // 待办事项详细描述
                m_setting.get(prefix + "category").toString(),                    // 待办事项分类
                m_setting.get(prefix + "important").toBool(),                     // 重要程度
                QDateTime::fromString(m_setting.get(prefix + "deadline").toString(),
                                      Qt::ISODate),                      // 截止日期、重复结束日期
                m_setting.get(prefix + "recurrenceInterval", 0).toInt(), // 重复间隔
                m_setting.get(prefix + "recurrenceCount", -1).toInt(),   // 重复次数
                QDate::fromString(m_setting.get(prefix + "recurrenceStartDate").toString(),
                                  Qt::ISODate),                        // 重复开始日期
                m_setting.get(prefix + "isCompleted", false).toBool(), // 是否已完成
                m_setting.get(prefix + "completedAt").toDateTime(),    // 完成时间
                m_setting.get(prefix + "isDeleted", false).toBool(),   // 是否已删除
                m_setting.get(prefix + "deletedAt").toDateTime(),      // 删除时间
                m_setting.get(prefix + "createdAt").toDateTime(),      // 创建时间
                m_setting.get(prefix + "updatedAt").toDateTime(),      // 更新时间
                m_setting.get(prefix + "lastModifiedAt").toDateTime(), // 最后修改时间
                m_setting.get(prefix + "synced").toBool(),             // 是否已同步
                this);

            todos.push_back(std::move(item));
        }

        qDebug() << "成功从本地存储加载" << todos.size() << "个待办事项";
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
        // 如果数量小于原大小，先删除所有现有的待办事项条目
        const size_t currentSize = m_setting.get("todos/size", 0).toInt();
        if (todos.size() < currentSize) {
            // 获取当前存储的待办事项数量

            // 删除所有现有的待办事项条目
            for (size_t i = 0; i < currentSize; ++i) {
                QString prefix = QString("todos/%1").arg(i);

                // 删除该待办事项的所有属性
                m_setting.remove(prefix + "/id");
                m_setting.remove(prefix + "/uuid");
                m_setting.remove(prefix + "/userUuid");
                m_setting.remove(prefix + "/title");
                m_setting.remove(prefix + "/description");
                m_setting.remove(prefix + "/category");
                m_setting.remove(prefix + "/important");
                m_setting.remove(prefix + "/createdAt");
                m_setting.remove(prefix + "/updatedAt");
                m_setting.remove(prefix + "/synced");
                m_setting.remove(prefix + "/deadline");
                m_setting.remove(prefix + "/recurrenceInterval");
                m_setting.remove(prefix + "/recurrenceCount");
                m_setting.remove(prefix + "/recurrenceStartDate");
                m_setting.remove(prefix + "/isCompleted");
                m_setting.remove(prefix + "/completedAt");
                m_setting.remove(prefix + "/isDeleted");
                m_setting.remove(prefix + "/deletedAt");
                m_setting.remove(prefix + "/lastModifiedAt");
                // 删除整个待办事项条目 (todos/x)
                m_setting.remove(prefix);
            }
        }

        // 保存待办事项数量
        m_setting.save("todos/size", static_cast<int>(todos.size()));

        // 保存每个待办事项
        for (size_t i = 0; i < todos.size(); ++i) {
            const TodoItem *item = todos.at(i).get();
            QString prefix = QString("todos/%1/").arg(i);

            m_setting.save(prefix + "id", item->id());
            m_setting.save(prefix + "uuid", item->uuid());
            m_setting.save(prefix + "userUuid", item->userUuid());
            m_setting.save(prefix + "title", item->title());
            m_setting.save(prefix + "description", item->description());
            m_setting.save(prefix + "category", item->category());
            m_setting.save(prefix + "important", item->important());
            m_setting.save(prefix + "createdAt", item->createdAt());
            m_setting.save(prefix + "updatedAt", item->updatedAt());
            m_setting.save(prefix + "synced", item->synced());
            m_setting.save(prefix + "deadline", item->deadline());
            m_setting.save(prefix + "recurrenceInterval", item->recurrenceInterval());
            m_setting.save(prefix + "recurrenceCount", item->recurrenceCount());
            m_setting.save(prefix + "recurrenceStartDate", item->recurrenceStartDate());
            m_setting.save(prefix + "isCompleted", item->isCompleted());
            m_setting.save(prefix + "completedAt", item->completedAt());
            m_setting.save(prefix + "isDeleted", item->isDeleted());
            m_setting.save(prefix + "deletedAt", item->deletedAt());
            m_setting.save(prefix + "lastModifiedAt", item->lastModifiedAt());
        }

        qDebug() << "已成功保存" << todos.size() << "个待办事项到本地存储";
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
