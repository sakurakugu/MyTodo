/**
 * @file todo_data_storage.cpp
 * @brief TodoDataStorage类的实现文件
 *
 * 该文件实现了TodoDataStorage类的所有方法，负责待办事项的本地存储和文件导入导出功能。
 *
 * @author Sakurakugu
 * @date 2025-01-25
 * @version 1.0.0
 */

#include "todo_data_storage.h"

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

TodoDataStorage::TodoDataStorage(Setting &setting, QObject *parent) : QObject(parent), m_setting(setting) {
    qDebug() << "TodoDataStorage 初始化完成";
}

TodoDataStorage::~TodoDataStorage() {
    qDebug() << "TodoDataStorage 已销毁";
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
        int count = m_setting.get(QStringLiteral("todos/size"), 0).toInt();
        qDebug() << "从本地存储加载" << count << "个待办事项";

        for (int i = 0; i < count; ++i) {
            QString prefix = QString("todos/%1/").arg(i);

            // 验证必要字段
            if (!m_setting.contains(prefix + "id") || !m_setting.contains(prefix + "title")) {
                qWarning() << "跳过无效的待办事项记录（索引" << i << "）：缺少必要字段";
                continue;
            }

            auto item = std::make_unique<TodoItem>(
                m_setting.get(prefix + "id").toInt(),                                              // id
                QUuid::fromString(m_setting.get(prefix + "uuid").toString()),                      // uuid
                QUuid::fromString(m_setting.get(prefix + "userUuid").toString()),                  // userUuid
                m_setting.get(prefix + "title").toString(),                                        // title
                m_setting.get(prefix + "description").toString(),                                  // description
                m_setting.get(prefix + "category").toString(),                                     // category
                m_setting.get(prefix + "important").toBool(),                                      // important
                QDateTime::fromString(m_setting.get(prefix + "deadline").toString(), Qt::ISODate), // deadline
                m_setting.get(prefix + "recurrenceInterval", 0).toInt(),                           // recurrenceInterval
                m_setting.get(prefix + "recurrenceCount", -1).toInt(),                             // recurrenceCount
                QDate::fromString(m_setting.get(prefix + "recurrenceStartDate").toString(),
                                  Qt::ISODate),                        // recurrenceStartDate
                m_setting.get(prefix + "isCompleted", false).toBool(), // isCompleted
                m_setting.get(prefix + "completedAt").toDateTime(),    // completedAt
                m_setting.get(prefix + "isDeleted", false).toBool(),   // isDeleted
                m_setting.get(prefix + "deletedAt").toDateTime(),      // deletedAt
                m_setting.get(prefix + "createdAt").toDateTime(),      // createdAt
                m_setting.get(prefix + "updatedAt").toDateTime(),      // updatedAt
                m_setting.get(prefix + "lastModifiedAt").toDateTime(), // lastModifiedAt
                m_setting.get(prefix + "synced").toBool(),             // synced
                this);

            todos.push_back(std::move(item));
        }

        qDebug() << "成功从本地存储加载" << todos.size() << "个待办事项";
        emit dataOperationCompleted(true, QString("成功加载 %1 个待办事项").arg(todos.size()));
    } catch (const std::exception &e) {
        qCritical() << "加载本地存储时发生异常:" << e.what();
        success = false;
        emit dataOperationCompleted(false, QString("加载失败: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "加载本地存储时发生未知异常";
        success = false;
        emit dataOperationCompleted(false, "加载失败: 未知异常");
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
        // 如果是空列表，先删除所有现有的待办事项条目
        if (todos.empty()) {
            // 获取当前存储的待办事项数量
            int currentSize = m_setting.get(QStringLiteral("todos/size"), 0).toInt();

            // 删除所有现有的待办事项条目
            for (int i = 0; i < currentSize; ++i) {
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

                // 删除整个待办事项条目
                m_setting.remove(prefix);
            }
        }

        // 保存待办事项数量
        m_setting.save(QStringLiteral("todos/size"), static_cast<int>(todos.size()));

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
        emit dataOperationCompleted(true, QString("成功保存 %1 个待办事项").arg(todos.size()));
        success = true;
    } catch (const std::exception &e) {
        qCritical() << "保存到本地存储时发生异常:" << e.what();
        success = false;
        emit dataOperationCompleted(false, QString("保存失败: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "保存到本地存储时发生未知异常";
        success = false;
        emit dataOperationCompleted(false, "保存失败: 未知异常");
    }
    return success;
}

/**
 * @brief 导出待办事项到文件
 * @param todos 待办事项容器引用
 * @param filePath 导出文件路径
 * @return 导出是否成功
 */
bool TodoDataStorage::exportTodos(const std::vector<std::unique_ptr<TodoItem>> &todos, const QString &filePath) {
    QJsonArray todosArray;

    // 将所有待办事项转换为JSON格式
    for (const auto &todo : todos) {
        QJsonObject todoObj = todoToJson(todo.get());
        todosArray.append(todoObj);
    }

    // 创建根JSON对象
    QJsonObject rootObj;
    rootObj["version"] = "1.0";
    rootObj["exportDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    rootObj["todos"] = todosArray;

    // 写入文件
    QJsonDocument doc(rootObj);
    QFile file(filePath);

    // 确保目录存在
    QFileInfo fileInfo(filePath);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "无法打开文件进行写入:" << filePath;
        emit dataOperationCompleted(false, QString("无法打开文件: %1").arg(filePath));
        return false;
    }

    file.write(doc.toJson());
    file.close();

    qDebug() << "成功导出" << todos.size() << "个待办事项到" << filePath;
    emit dataOperationCompleted(true, QString("成功导出 %1 个待办事项").arg(todos.size()));
    return true;
}

/**
 * @brief 简单导入待办事项（跳过冲突项目）
 * @param todos 待办事项容器引用
 * @param filePath 导入文件路径
 * @return 导入是否成功
 */
bool TodoDataStorage::importTodos(std::vector<std::unique_ptr<TodoItem>> &todos, const QString &filePath) {
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件进行读取:" << filePath;
        emit dataOperationCompleted(false, QString("无法打开文件: %1").arg(filePath));
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON解析错误:" << error.errorString();
        emit dataOperationCompleted(false, QString("JSON解析错误: %1").arg(error.errorString()));
        return false;
    }

    QJsonObject rootObj = doc.object();

    if (!validateJsonFormat(rootObj)) {
        emit dataOperationCompleted(false, "文件格式无效");
        return false;
    }

    QJsonArray todosArray = rootObj["todos"].toArray();
    int importedCount = 0;
    int skippedCount = 0;

    for (const QJsonValue &value : todosArray) {
        QJsonObject todoObj = value.toObject();
        int id = todoObj["id"].toInt();

        // 检查是否已存在相同ID的待办事项
        if (findTodoById(todos, id) != nullptr) {
            skippedCount++;
            continue;
        }

        // 创建新的待办事项
        auto newTodo = createTodoFromJson(todoObj, this);
        todos.push_back(std::move(newTodo));
        importedCount++;
    }

    qDebug() << "导入完成 - 新增:" << importedCount << "个，跳过:" << skippedCount << "个";
    emit dataOperationCompleted(true,
                                QString("导入完成 - 新增: %1 个，跳过: %2 个").arg(importedCount).arg(skippedCount));
    return true;
}

/**
 * @brief 检查导入冲突
 * @param todos 待办事项容器引用
 * @param filePath 导入文件路径
 * @return 冲突信息列表
 */
QVariantList TodoDataStorage::checkImportConflicts(const std::vector<std::unique_ptr<TodoItem>> &todos,
                                                   const QString &filePath) {
    QVariantList conflicts;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件进行读取:" << filePath;
        emit dataOperationCompleted(false, QString("无法打开文件: %1").arg(filePath));
        return conflicts;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON解析错误:" << error.errorString();
        emit dataOperationCompleted(false, QString("JSON解析错误: %1").arg(error.errorString()));
        return conflicts;
    }

    QJsonObject rootObj = doc.object();

    if (!validateJsonFormat(rootObj)) {
        emit dataOperationCompleted(false, "文件格式无效");
        return conflicts;
    }

    QJsonArray todosArray = rootObj["todos"].toArray();

    for (const QJsonValue &value : todosArray) {
        QJsonObject todoObj = value.toObject();
        int id = todoObj["id"].toInt();

        // 查找是否存在相同ID的待办事项
        TodoItem *existingTodo = findTodoById(todos, id);
        if (existingTodo != nullptr) {
            // 检查内容是否真的不同
            QString importTitle = todoObj["title"].toString();
            QString importDescription = todoObj["description"].toString();
            QString importCategory = todoObj["category"].toString();

            if (existingTodo->title() != importTitle || existingTodo->description() != importDescription ||
                existingTodo->category() != importCategory) {
                // 发现真正冲突，创建冲突信息
                QVariantMap conflictInfo;
                conflictInfo["id"] = id;
                conflictInfo["existingTitle"] = existingTodo->title();
                conflictInfo["existingDescription"] = existingTodo->description();
                conflictInfo["existingCategory"] = existingTodo->category();
                conflictInfo["existingUpdatedAt"] = existingTodo->updatedAt();

                conflictInfo["importTitle"] = importTitle;
                conflictInfo["importDescription"] = importDescription;
                conflictInfo["importCategory"] = importCategory;
                conflictInfo["importUpdatedAt"] = QDateTime::fromString(todoObj["updatedAt"].toString(), Qt::ISODate);

                conflicts.append(conflictInfo);
                qDebug() << "发现真正冲突项目 ID:" << id << "现有标题:" << existingTodo->title()
                         << "导入标题:" << importTitle;
            } else {
                qDebug() << "ID相同且内容一致，直接跳过 ID:" << id << "标题:" << importTitle;
            }
        }
    }

    qDebug() << "冲突检查完成，冲突项目数量:" << conflicts.size();
    return conflicts;
}

/**
 * @brief 带冲突解决策略的导入
 * @param todos 待办事项容器引用
 * @param filePath 导入文件路径
 * @param conflictResolution 冲突解决策略
 * @return 导入是否成功
 */
bool TodoDataStorage::importTodosWithConflictResolution(std::vector<std::unique_ptr<TodoItem>> &todos,
                                                        const QString &filePath,
                                                        ConflictResolution conflictResolution) {
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件进行读取:" << filePath;
        emit dataOperationCompleted(false, QString("无法打开文件: %1").arg(filePath));
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON解析错误:" << error.errorString();
        emit dataOperationCompleted(false, QString("JSON解析错误: %1").arg(error.errorString()));
        return false;
    }

    QJsonObject rootObj = doc.object();

    if (!validateJsonFormat(rootObj)) {
        emit dataOperationCompleted(false, "文件格式无效");
        return false;
    }

    QJsonArray todosArray = rootObj["todos"].toArray();
    int importedCount = 0;
    int skippedCount = 0;
    int overwrittenCount = 0;

    for (const QJsonValue &value : todosArray) {
        QJsonObject todoObj = value.toObject();
        int id = todoObj["id"].toInt();

        // 查找是否已存在相同ID的待办事项
        TodoItem *existingTodo = findTodoById(todos, id);

        if (existingTodo != nullptr) {
            if (conflictResolution == ConflictResolution::Overwrite) {
                // 覆盖现有项目
                existingTodo->setTitle(todoObj["title"].toString());
                existingTodo->setDescription(todoObj["description"].toString());
                existingTodo->setCategory(todoObj["category"].toString());
                existingTodo->setImportant(todoObj["important"].toBool());
                existingTodo->setUpdatedAt(QDateTime::fromString(todoObj["updatedAt"].toString(), Qt::ISODate));
                existingTodo->setSynced(todoObj["synced"].toBool());
                overwrittenCount++;
            } else if (conflictResolution == ConflictResolution::Merge) {
                // 合并：保留较新的更新时间的版本
                QDateTime importUpdatedAt = QDateTime::fromString(todoObj["updatedAt"].toString(), Qt::ISODate);

                if (importUpdatedAt > existingTodo->updatedAt()) {
                    // 导入的版本更新，使用导入的数据
                    existingTodo->setTitle(todoObj["title"].toString());
                    existingTodo->setDescription(todoObj["description"].toString());
                    existingTodo->setCategory(todoObj["category"].toString());
                    existingTodo->setImportant(todoObj["important"].toBool());
                    existingTodo->setUpdatedAt(importUpdatedAt);
                    existingTodo->setSynced(todoObj["synced"].toBool());
                    overwrittenCount++;
                }
                // 如果现有版本更新或相同，则保持不变
            } else if (conflictResolution == ConflictResolution::Skip) {
                // 跳过冲突项目
                skippedCount++;
            }
        } else {
            // 创建新的待办事项
            auto newTodo = createTodoFromJson(todoObj, this);
            todos.push_back(std::move(newTodo));
            importedCount++;
        }
    }

    qDebug() << "导入完成 - 新增:" << importedCount << "个，覆盖:" << overwrittenCount << "个，跳过:" << skippedCount
             << "个";
    emit dataOperationCompleted(true, QString("导入完成 - 新增: %1 个，覆盖: %2 个，跳过: %3 个")
                                          .arg(importedCount)
                                          .arg(overwrittenCount)
                                          .arg(skippedCount));
    return true;
}

/**
 * @brief 带个别冲突解决的导入
 * @param todos 待办事项容器引用
 * @param filePath 导入文件路径
 * @param resolutions 个别解决方案映射
 * @return 导入是否成功
 */
bool TodoDataStorage::importTodosWithIndividualResolution(std::vector<std::unique_ptr<TodoItem>> &todos,
                                                          const QString &filePath, const QVariantMap &resolutions) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件进行读取:" << filePath;
        emit dataOperationCompleted(false, QString("无法打开文件: %1").arg(filePath));
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON解析错误:" << error.errorString();
        emit dataOperationCompleted(false, QString("JSON解析错误: %1").arg(error.errorString()));
        return false;
    }

    QJsonObject rootObj = doc.object();

    if (!validateJsonFormat(rootObj)) {
        emit dataOperationCompleted(false, "文件格式无效");
        return false;
    }

    QJsonArray todosArray = rootObj["todos"].toArray();
    int importedCount = 0;
    int updatedCount = 0;
    int skippedCount = 0;

    for (const QJsonValue &value : todosArray) {
        QJsonObject todoObj = value.toObject();
        int id = todoObj["id"].toInt();

        // 查找是否存在相同ID的项目
        TodoItem *existingTodo = findTodoById(todos, id);

        if (existingTodo != nullptr) {
            // 获取该项目的处理方式
            QString resolution = resolutions.value(QString::number(id), "skip").toString();

            if (resolution == "overwrite") {
                // 覆盖现有数据
                existingTodo->setTitle(todoObj["title"].toString());
                existingTodo->setDescription(todoObj["description"].toString());
                existingTodo->setCategory(todoObj["category"].toString());
                existingTodo->setCreatedAt(QDateTime::fromString(todoObj["createdAt"].toString(), Qt::ISODate));
                existingTodo->setUpdatedAt(QDateTime::fromString(todoObj["updatedAt"].toString(), Qt::ISODate));
                existingTodo->setSynced(false); // 标记为未同步
                updatedCount++;
            } else if (resolution == "merge") {
                // 智能合并：保留更新时间较新的版本
                QDateTime existingUpdated = existingTodo->updatedAt();
                QDateTime importUpdated = QDateTime::fromString(todoObj["updatedAt"].toString(), Qt::ISODate);

                if (importUpdated > existingUpdated) {
                    existingTodo->setTitle(todoObj["title"].toString());
                    existingTodo->setDescription(todoObj["description"].toString());
                    existingTodo->setCategory(todoObj["category"].toString());
                    existingTodo->setCreatedAt(QDateTime::fromString(todoObj["createdAt"].toString(), Qt::ISODate));
                    existingTodo->setUpdatedAt(importUpdated);
                    existingTodo->setSynced(false); // 标记为未同步
                    updatedCount++;
                } else {
                    skippedCount++; // 现有数据更新，跳过导入
                }
            } else {
                // skip - 跳过冲突项目
                skippedCount++;
            }
        } else {
            // 创建新项目（没有冲突的项目直接导入）
            auto newTodo = createTodoFromJson(todoObj, this);
            newTodo->setSynced(false); // 标记为未同步
            todos.push_back(std::move(newTodo));
            importedCount++;
        }
    }

    qDebug() << "个别冲突处理导入完成 - 新增:" << importedCount << "个，更新:" << updatedCount
             << "个，跳过:" << skippedCount << "个";
    emit dataOperationCompleted(true, QString("导入完成 - 新增: %1 个，更新: %2 个，跳过: %3 个")
                                          .arg(importedCount)
                                          .arg(updatedCount)
                                          .arg(skippedCount));
    return true;
}

/**
 * @brief 自动解决冲突的导入
 * @param todos 待办事项容器引用
 * @param filePath 导入文件路径
 * @return 冲突信息列表（包含非冲突项目的导入结果）
 */
QVariantList TodoDataStorage::importTodosWithAutoResolution(std::vector<std::unique_ptr<TodoItem>> &todos,
                                                            const QString &filePath) {
    QVariantList conflicts;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件进行读取:" << filePath;
        emit dataOperationCompleted(false, QString("无法打开文件: %1").arg(filePath));
        return conflicts;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON解析错误:" << error.errorString();
        emit dataOperationCompleted(false, QString("JSON解析错误: %1").arg(error.errorString()));
        return conflicts;
    }

    QJsonObject rootObj = doc.object();

    if (!validateJsonFormat(rootObj)) {
        emit dataOperationCompleted(false, "文件格式无效");
        return conflicts;
    }

    QJsonArray todosArray = rootObj["todos"].toArray();
    QJsonArray nonConflictTodos;

    qDebug() << "开始检查导入冲突，现有项目数量:" << todos.size() << "，导入项目数量:" << todosArray.size();

    for (const QJsonValue &value : todosArray) {
        QJsonObject todoObj = value.toObject();
        int id = todoObj["id"].toInt();

        bool hasConflict = false;
        bool shouldSkip = false;
        TodoItem *existingTodo = nullptr;

        // 查找是否存在相同ID的待办事项
        existingTodo = findTodoById(todos, id);
        if (existingTodo != nullptr) {
            // 检查内容是否真的不同
            QString importTitle = todoObj["title"].toString();
            QString importDescription = todoObj["description"].toString();
            QString importCategory = todoObj["category"].toString();

            if (existingTodo->title() != importTitle || existingTodo->description() != importDescription ||
                existingTodo->category() != importCategory) {
                hasConflict = true;
                qDebug() << "发现真正冲突项目 ID:" << id << "现有标题:" << existingTodo->title()
                         << "导入标题:" << importTitle;
            } else {
                qDebug() << "ID相同且内容一致，直接跳过 ID:" << id << "标题:" << importTitle;
                // 内容完全一致的项目直接跳过，既不导入也不显示冲突
                shouldSkip = true;
            }
        }

        if (shouldSkip) {
            // 跳过内容完全一致的项目
            continue;
        } else if (hasConflict) {
            // 发现冲突，添加到冲突列表
            QVariantMap conflictInfo;
            conflictInfo["id"] = id;
            conflictInfo["existingTitle"] = existingTodo->title();
            conflictInfo["existingDescription"] = existingTodo->description();
            conflictInfo["existingCategory"] = existingTodo->category();
            conflictInfo["existingUpdatedAt"] = existingTodo->updatedAt();

            conflictInfo["importTitle"] = todoObj["title"].toString();
            conflictInfo["importDescription"] = todoObj["description"].toString();
            conflictInfo["importCategory"] = todoObj["category"].toString();
            conflictInfo["importUpdatedAt"] = QDateTime::fromString(todoObj["updatedAt"].toString(), Qt::ISODate);

            conflicts.append(conflictInfo);
        } else {
            // 无冲突，添加到非冲突列表
            qDebug() << "无冲突项目 ID:" << id << "标题:" << todoObj["title"].toString();
            nonConflictTodos.append(value);
        }
    }

    qDebug() << "冲突检查完成，冲突项目数量:" << conflicts.size() << "，无冲突项目数量:" << nonConflictTodos.size();

    // 导入无冲突的项目
    if (nonConflictTodos.size() > 0) {
        for (const QJsonValue &value : nonConflictTodos) {
            QJsonObject todoObj = value.toObject();
            auto newTodo = createTodoFromJson(todoObj, this);
            todos.push_back(std::move(newTodo));
        }
    }

    emit dataOperationCompleted(
        true, QString("自动导入完成 - 新增: %1 个，冲突: %2 个").arg(nonConflictTodos.size()).arg(conflicts.size()));
    return conflicts;
}

/**
 * @brief 验证JSON文件格式
 * @param jsonObject JSON对象
 * @return 格式是否有效
 */
bool TodoDataStorage::validateJsonFormat(const QJsonObject &jsonObject) {
    // 检查版本字段
    if (!jsonObject.contains("version")) {
        qWarning() << "JSON验证" << "缺少版本字段";
        return false;
    }

    QString version = jsonObject["version"].toString();
    if (version != "1.0") {
        qWarning() << "JSON验证" << QString("不支持的文件版本: %1").arg(version);
        return false;
    }

    // 检查todos数组
    if (!jsonObject.contains("todos") || !jsonObject["todos"].isArray()) {
        qWarning() << "JSON验证" << "缺少或无效的todos数组";
        return false;
    }

    return true;
}

/**
 * @brief 从JSON对象创建TodoItem
 * @param jsonObject JSON对象
 * @param parent 父对象
 * @return TodoItem智能指针
 */
std::unique_ptr<TodoItem> TodoDataStorage::createTodoFromJson(const QJsonObject &jsonObject, QObject *parent) {
    return std::make_unique<TodoItem>(
        jsonObject["id"].toInt(),                                                       // id
        QUuid::fromString(jsonObject["uuid"].toString()),                               // uuid
        QUuid::fromString(jsonObject["userUuid"].toString()),                           // userUuid
        jsonObject["title"].toString(),                                                 // title
        jsonObject["description"].toString(),                                           // description
        jsonObject["category"].toString(),                                              // category
        jsonObject["important"].toBool(),                                               // important
        QDateTime::fromString(jsonObject["deadline"].toString(), Qt::ISODate),          // deadline
        jsonObject["recurrence_interval"].toInt(0),                                     // recurrenceInterval
        jsonObject["recurrence_count"].toInt(-1),                                       // recurrenceCount
        QDate::fromString(jsonObject["recurrence_start_date"].toString(), Qt::ISODate), // recurrenceStartDate
        jsonObject["isCompleted"].toBool(false),                                        // isCompleted
        QDateTime::fromString(jsonObject["completedAt"].toString(), Qt::ISODate),       // completedAt
        jsonObject["isDeleted"].toBool(false),                                          // isDeleted
        QDateTime::fromString(jsonObject["deletedAt"].toString(), Qt::ISODate),         // deletedAt
        QDateTime::fromString(jsonObject["createdAt"].toString(), Qt::ISODate),         // createdAt
        QDateTime::fromString(jsonObject["updatedAt"].toString(), Qt::ISODate),         // updatedAt
        QDateTime::fromString(jsonObject["lastModifiedAt"].toString(), Qt::ISODate),    // lastModifiedAt
        false,                                                                          // synced
        parent);
}

/**
 * @brief 将TodoItem转换为JSON对象
 * @param todo TodoItem指针
 * @return JSON对象
 */
QJsonObject TodoDataStorage::todoToJson(const TodoItem *todo) {
    QJsonObject todoObj;
    todoObj["id"] = todo->id();
    todoObj["uuid"] = todo->uuid().toString();
    todoObj["userUuid"] = todo->userUuid().toString();
    todoObj["title"] = todo->title();
    todoObj["description"] = todo->description();
    todoObj["category"] = todo->category();
    todoObj["important"] = todo->important();
    todoObj["createdAt"] = todo->createdAt().toString(Qt::ISODate);
    todoObj["updatedAt"] = todo->updatedAt().toString(Qt::ISODate);
    todoObj["synced"] = todo->synced();
    todoObj["deadline"] = todo->deadline().toString(Qt::ISODate);
    todoObj["recurrence_interval"] = todo->recurrenceInterval();
    todoObj["recurrence_count"] = todo->recurrenceCount();
    todoObj["recurrence_start_date"] = todo->recurrenceStartDate().toString(Qt::ISODate);
    todoObj["isCompleted"] = todo->isCompleted();
    todoObj["completedAt"] = todo->completedAt().toString(Qt::ISODate);
    todoObj["isDeleted"] = todo->isDeleted();
    todoObj["deletedAt"] = todo->deletedAt().toString(Qt::ISODate);
    todoObj["lastModifiedAt"] = todo->lastModifiedAt().toString(Qt::ISODate);
    return todoObj;
}

/**
 * @brief 查找具有指定ID的待办事项
 * @param todos 待办事项容器引用
 * @param id 待办事项ID
 * @return 找到的项目指针，未找到返回nullptr
 */
TodoItem *TodoDataStorage::findTodoById(const std::vector<std::unique_ptr<TodoItem>> &todos, int id) {
    auto it = std::find_if(todos.begin(), todos.end(),
                           [id](const std::unique_ptr<TodoItem> &todo) { return todo->id() == id; });
    return (it != todos.end()) ? it->get() : nullptr;
}
