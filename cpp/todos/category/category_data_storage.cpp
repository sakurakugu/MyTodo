/**
 * @file category_data_storage.cpp
 * @brief CategoryDataStorage类的实现文件
 *
 * 该文件实现了CategoryDataStorage类，专门负责类别的本地存储和文件导入功能。
 * 从CategoryManager类中拆分出来，实现单一职责原则。
 *
 * @author Sakurakugu
 * @date 2025-01-27 00:00:00(UTC+8) 周一
 * @change 2025-01-27 00:00:00(UTC+8) 周一
 * @version 0.4.0
 */

#include "category_data_storage.h"
#include "../../foundation/config.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

CategoryDataStorage::CategoryDataStorage(QObject *parent)
    : QObject(parent),                    // 父对象
      m_database(Database::GetInstance()) // 数据库管理器
{
    // 确保数据库已初始化
    if (!m_database.initializeDatabase()) {
        qCritical() << "数据库未初始化";
    }
}

CategoryDataStorage::~CategoryDataStorage() {
}

/**
 * @brief 加载类别
 * @param categories 类别列表引用
 * @return 加载成功返回true，否则返回false
 */
bool CategoryDataStorage::加载类别(CategorieList &categories) {
    bool success = true;

    try {
        // 清除当前数据
        categories.clear();

        // 从数据库加载数据
        QSqlDatabase db = m_database.getDatabase();
        if (!db.isOpen()) {
            qCritical() << "数据库未打开，无法加载类别";
            return false;
        }

        QSqlQuery query(db);
        const QString queryString = //
            "SELECT id, uuid, name, user_uuid, created_at, updated_at, "
            "synced FROM categories ORDER BY id"; // 按id排序

        if (!query.exec(queryString)) {
            qCritical() << "加载类别查询失败:" << query.lastError().text();
            return false;
        }

        while (query.next()) {
            int id = query.value("id").toInt();
            QUuid uuid = QUuid::fromString(query.value("uuid").toString());
            QString name = query.value("name").toString();
            QUuid userUuid = QUuid::fromString(query.value("user_uuid").toString());
            QDateTime createdAt = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);
            QDateTime updatedAt = QDateTime::fromString(query.value("updated_at").toString(), Qt::ISODate);
            int synced = query.value("synced").toInt();

            auto item = std::make_unique<CategorieItem>( //
                id,                                      // 唯一标识符
                uuid,                                    // 唯一标识符（UUID）
                name,                                    // 分类名称
                userUuid,                                // 用户UUID
                createdAt,                               // 创建时间
                updatedAt,                               // 更新时间
                synced,                                  // 是否已同步
                this                                     // 父对象
            );
            categories.push_back(std::move(item));
        }

        qDebug() << "成功从数据库加载" << categories.size() << "个类别";
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
 * @brief 新增类别
 * @param categories 类别列表引用
 * @param name 类别名称
 * @param userUuid 用户UUID
 * @return 添加成功返回true，否则返回false
 */
bool CategoryDataStorage::新增类别(CategorieList &categories, const QString &name, const QUuid &userUuid) {
    bool success = true;

    try {
        QSqlDatabase db = m_database.getDatabase();
        if (!db.isOpen()) {
            qCritical() << "数据库未打开，无法新增类别";
            return false;
        }

        // 获取当前最大的id
        int maxId = 0;
        QSqlQuery maxIdQuery(db);
        if (maxIdQuery.exec("SELECT MAX(id) FROM categories")) {
            if (maxIdQuery.next()) {
                maxId = maxIdQuery.value(0).toInt();
            }
        }

        // 创建新的类别项
        int newId = maxId + 1;
        QUuid newUuid = QUuid::createUuid();
        QDateTime createdAt = QDateTime::currentDateTime();

        auto newItem = std::make_unique<CategorieItem>( //
            newId,                                      // 新的唯一标识符
            newUuid,                                    // 新的UUID
            name,                                       // 类别名称
            userUuid,                                   // 用户UUID
            createdAt,                                  // 当前时间
            createdAt,                                  // 更新时间
            1,                                          // 未同步，插入
            this);

        // 插入到数据库
        QSqlQuery insertQuery(db);
        const QString insertString = //
            "INSERT INTO categories (id, uuid, name, user_uuid, created_at, updated_at, synced) VALUES (?, ?, ?, ?, ?, "
            "?, ?, ?)";
        insertQuery.prepare(insertString);
        insertQuery.addBindValue(newId);
        insertQuery.addBindValue(newUuid.toString());
        insertQuery.addBindValue(name);
        insertQuery.addBindValue(userUuid.toString());
        insertQuery.addBindValue(createdAt.toString(Qt::ISODate));
        insertQuery.addBindValue(createdAt.toString(Qt::ISODate));
        insertQuery.addBindValue(1); // 未同步，插入

        if (!insertQuery.exec()) {
            qCritical() << "插入类别到数据库失败:" << insertQuery.lastError().text();
            return false;
        }

        // 添加到内存列表
        categories.push_back(std::move(newItem));

        qDebug() << "成功新增类别:" << name;
    } catch (const std::exception &e) {
        qCritical() << "新增类别时发生异常:" << e.what();
        success = false;
    } catch (...) {
        qCritical() << "新增类别时发生未知异常";
        success = false;
    }

    return success;
}

/**
 * @brief 更新类别
 * @param categories 类别列表引用
 * @param name 类别名称
 * @param newName 新的类别名称
 * @return 更新成功返回true，否则返回false
 */
bool CategoryDataStorage::更新类别(CategorieList &categories, const QString &name, const QString &newName) {
    bool success = false;

    try {
        QSqlDatabase db = m_database.getDatabase();
        if (!db.isOpen()) {
            qCritical() << "数据库未打开，无法更新类别";
            return false;
        }

        // 更新数据库中的类别
        QSqlQuery updateQuery(db);
        const QString updateString = "UPDATE categories SET name = ?, updated_at = ?, synced = ? WHERE name = ?";
        updateQuery.prepare(updateString);
        QString currentTime = QDateTime::currentDateTime().toString(Qt::ISODate);
        updateQuery.addBindValue(newName);
        updateQuery.addBindValue(currentTime); // updated_at
        updateQuery.addBindValue(2);           // 标记为未同步，待更新
        updateQuery.addBindValue(name);        // WHERE条件

        if (!updateQuery.exec()) {
            qCritical() << "更新数据库中的类别失败:" << updateQuery.lastError().text();
            return false;
        }

        if (updateQuery.numRowsAffected() == 0) {
            qWarning() << "未找到要更新的类别，名称:" << name;
            return false;
        }

        // 更新内存中的类别
        auto it = std::find_if(categories.begin(), categories.end(),
                               [name](const std::unique_ptr<CategorieItem> &item) { return item->name() == name; });

        if (it != categories.end()) {
            (*it)->setName(newName);
            if ((*it)->synced() != 1) { // 如果之前是插入状态，不要改成更新状态
                (*it)->setSynced(2);    // 标记为未同步，待更新
            } else if ((*it)->synced() == 1) {
                // 如果之前是插入状态，保持为插入状态
                updateQuery.addBindValue(1);
            }
            (*it)->setUpdatedAt(QDateTime::fromString(currentTime, Qt::ISODate));
            success = true;
        }

        if (success) {
            qDebug() << "成功更新类别:" << name;
        }
    } catch (const std::exception &e) {
        qCritical() << "更新类别时发生异常:" << e.what();
        success = false;
    } catch (...) {
        qCritical() << "更新类别时发生未知异常";
        success = false;
    }

    return success;
}

/**
 * @brief 删除类别
 * @param categories 类别列表引用
 * @param name 类别名称
 * @return 删除成功返回true，否则返回false
 */
bool CategoryDataStorage::删除类别(CategorieList &categories, const QString &name) {
    bool success = false;

    try {
        QSqlDatabase db = m_database.getDatabase();
        if (!db.isOpen()) {
            qCritical() << "数据库未打开，无法删除类别";
            return false;
        }

        // 从数据库中删除类别
        QSqlQuery deleteQuery(db);
        const QString deleteString = "DELETE FROM categories WHERE name = ?";
        deleteQuery.prepare(deleteString);
        deleteQuery.addBindValue(name);

        if (!deleteQuery.exec()) {
            qCritical() << "从数据库删除类别失败:" << deleteQuery.lastError().text();
            return false;
        }

        if (deleteQuery.numRowsAffected() == 0) {
            qWarning() << "未找到要删除的类别，名称:" << name;
            return false;
        }

        // 从内存中删除类别
        auto it = std::find_if(categories.begin(), categories.end(),
                               [name](const std::unique_ptr<CategorieItem> &item) { return item->name() == name; });

        if (it != categories.end()) {
            categories.erase(it);
            success = true;
            qDebug() << "成功删除类别:" << name;
        }
    } catch (const std::exception &e) {
        qCritical() << "删除类别时发生异常:" << e.what();
        success = false;
    } catch (...) {
        qCritical() << "删除类别时发生未知异常";
        success = false;
    }

    return success;
}

/**
 * @brief 软删除类别
 * @param categories 类别列表引用
 * @param name 类别名称
 */
bool CategoryDataStorage::软删除类别(CategorieList &categories, const QString &name) {
    bool success = false;

    try {
        QSqlDatabase db = m_database.getDatabase();
        if (!db.isOpen()) {
            qCritical() << "数据库未打开，无法软删除类别";
            return false;
        }

        // 更新数据库中的类别
        QSqlQuery softDeleteQuery(db);
        const QString updateString = "UPDATE categories SET updated_at = ?, synced = ? WHERE name = ?";
        softDeleteQuery.prepare(updateString);
        QString currentTime = QDateTime::currentDateTime().toString(Qt::ISODate);
        softDeleteQuery.addBindValue(currentTime); // updated_at
        softDeleteQuery.addBindValue(3);           // 标记为未同步，待删除
        softDeleteQuery.addBindValue(name);        // WHERE条件

        if (!softDeleteQuery.exec()) {
            qCritical() << "软删除数据库中的类别失败:" << softDeleteQuery.lastError().text();
            return false;
        }

        if (softDeleteQuery.numRowsAffected() == 0) {
            qWarning() << "未找到要软删除的类别，名称:" << name;
            return false;
        }

        // 更新内存中的类别
        auto it = std::find_if(categories.begin(), categories.end(),
                               [name](const std::unique_ptr<CategorieItem> &item) { return item->name() == name; });

        if (it != categories.end()) {
            if ((*it)->synced() == 1) {
                // 如果之前是插入状态，直接从内存中删除
                success = 删除类别(categories, name);
            } else {
                (*it)->setSynced(3); // 标记为未同步，待删除
                (*it)->setUpdatedAt(QDateTime::fromString(currentTime, Qt::ISODate));
                success = true;
                qDebug() << "成功软删除类别:" << name;
            }
        }
    } catch (const std::exception &e) {
        qCritical() << "更新类别时发生异常:" << e.what();
        success = false;
    } catch (...) {
        qCritical() << "更新类别时发生未知异常";
        success = false;
    }

    return success;
}

/**
 * @brief 更新同步状态
 * @param categories 类别列表引用
 * @param uuid 类别UUID
 * @param synced 同步状态（0：已同步，1：未同步插入，2：未同步更新，3：未同步删除）
 * @return 更新成功返回true，否则返回false
 */
bool CategoryDataStorage::更新同步状态(CategorieList &categories, const QUuid &uuid, int synced) {
    bool success = false;

    try {
        QSqlDatabase db = m_database.getDatabase();
        if (!db.isOpen()) {
            qCritical() << "数据库未打开，无法更新类别";
            return false;
        }

        // 更新数据库中的类别
        QSqlQuery updateQuery(db);
        const QString updateString = "UPDATE categories SET synced = ?, updated_at = ?, synced = ? WHERE uuid = ?";
        updateQuery.prepare(updateString);
        QString currentTime = QDateTime::currentDateTime().toString(Qt::ISODate);
        updateQuery.addBindValue(synced);
        updateQuery.addBindValue(currentTime); // updated_at
        updateQuery.addBindValue(2);           // 标记为未同步，待更新
        updateQuery.addBindValue(uuid);        // WHERE条件

        if (!updateQuery.exec()) {
            qCritical() << "更新数据库中的类别同步状态失败:" << updateQuery.lastError().text();
            return false;
        }

        if (updateQuery.numRowsAffected() == 0) {
            qWarning() << "未找到要更新的类别，uuid:" << uuid;
            return false;
        }

        // 更新内存中的类别
        auto it = std::find_if(categories.begin(), categories.end(),
                               [uuid](const std::unique_ptr<CategorieItem> &item) { return item->uuid() == uuid; });

        if (it != categories.end()) {
            (*it)->setSynced(0); // 标记为已同步
            (*it)->setUpdatedAt(QDateTime::fromString(currentTime, Qt::ISODate));
            success = true;
        }

        if (success) {
            qDebug() << "成功更新类别:" << (*it)->name() << "的同步状态";
        }
    } catch (const std::exception &e) {
        qCritical() << "更新类别时发生异常:" << e.what();
        success = false;
    } catch (...) {
        qCritical() << "更新类别时发生未知异常";
        success = false;
    }

    return success;
}

/**
 * @brief 创建默认类别
 * @param categories 类别列表引用
 * @param userUuid 用户UUID
 */
bool CategoryDataStorage::创建默认类别(CategorieList &categories, const QUuid &userUuid) {

    bool success = true;
    QSqlDatabase db = m_database.getDatabase();

    try {
        if (!db.isOpen()) {
            qCritical() << "数据库未打开，无法创建默认类别";
            return false;
        }

        // 开始事务
        if (!db.transaction()) {
            qCritical() << "无法开始数据库事务:" << db.lastError().text();
            return false;
        }

        // 清除现有数据
        QSqlQuery deleteQuery(db);
        if (!deleteQuery.exec("DELETE FROM categories")) {
            qCritical() << "清除类别数据失败:" << deleteQuery.lastError().text();
            db.rollback();
            return false;
        }

        categories.clear();
        // 添加默认的"未分类"选项
        QUuid newUuid = QUuid::createUuid();
        QDateTime createdAt = QDateTime::currentDateTime();

        auto newItem = std::make_unique<CategorieItem>( //
            1,                                          // 新的唯一标识符
            newUuid,                                    // 新的UUID
            "未分类",                                   // 类别名称
            userUuid,                                   // 用户UUID
            createdAt,                                  // 当前时间
            createdAt,                                  // 更新时间
            0,                                          // 已同步（不需要同步）
            this);

        // 插入到数据库
        QSqlQuery insertQuery(db);
        const QString insertString = //
            "INSERT INTO categories (id, uuid, name, user_uuid, created_at, updated_at, synced) VALUES (?, ?, ?, ?, ?, "
            "?, ?, ?)";
        insertQuery.prepare(insertString);
        insertQuery.addBindValue(1);
        insertQuery.addBindValue(newUuid.toString());
        insertQuery.addBindValue("未分类");
        insertQuery.addBindValue(userUuid.toString());
        insertQuery.addBindValue(createdAt.toString(Qt::ISODate));
        insertQuery.addBindValue(createdAt.toString(Qt::ISODate));
        insertQuery.addBindValue(0); // 已同步（不需要同步）

        if (!insertQuery.exec()) {
            qCritical() << "插入类别到数据库失败:" << insertQuery.lastError().text();
            db.rollback();
            return false;
        }

        // 提交事务
        if (!db.commit()) {
            qCritical() << "提交数据库事务失败:" << db.lastError().text();
            return false;
        }

        // 添加到内存列表
        categories.push_back(std::move(newItem));

        qDebug() << "成功添加默认类别";
    } catch (const std::exception &e) {
        qCritical() << "添加默认类别时发生异常:" << e.what();
        db.rollback();
        success = false;
    } catch (...) {
        qCritical() << "添加默认类别时发生未知异常";
        db.rollback();
        success = false;
    }

    return success;
}

/**
 * @brief 导入类别从JSON
 * @param categories 类别列表引用
 * @param categoriesArray JSON类别数组
 * @param source 来源（服务器或本地备份）
 * @param resolution 冲突解决策略
 */
bool CategoryDataStorage::导入类别从JSON(CategorieList &categories, const QJsonArray &categoriesArray,
                                         CategoryImportSource source, ConflictResolution resolution) {
    bool success = true;

    try {
        for (const QJsonValue &value : categoriesArray) {
            if (!value.isObject()) {
                qWarning() << "跳过无效的类别项（非对象）";
                continue;
            }

            QJsonObject obj = value.toObject();
            if (!obj.contains("name") || !obj.contains("user_uuid")) {
                qWarning() << "跳过无效的类别项（缺少必要字段）";
                continue;
            }

            QString name = obj.value("name").toString();
            QUuid userUuid = QUuid::fromString(obj.value("user_uuid").toString());
            QUuid uuid = obj.contains("uuid") ? QUuid::fromString(obj.value("uuid").toString()) : QUuid::createUuid();
            QDateTime createdAt = obj.contains("created_at")
                                      ? QDateTime::fromString(obj.value("created_at").toString(), Qt::ISODate)
                                      : QDateTime::currentDateTime();
            QDateTime updatedAt = obj.contains("updated_at")
                                      ? QDateTime::fromString(obj.value("updated_at").toString(), Qt::ISODate)
                                      : createdAt;

            auto newItem = std::make_unique<CategorieItem>(     //
                获取下一个可用ID(categories),                   // 唯一标识符
                uuid,                                           // 唯一标识符（UUID）
                name,                                           // 分类名称
                userUuid,                                       // 用户UUID
                createdAt,                                      // 创建时间
                updatedAt,                                      // 更新时间
                source == CategoryImportSource::Server ? 0 : 1, // 服务器就是已同步（0）否则是未同步，插入（1）
                this);

            处理冲突(categories, newItem, source, resolution);
        }

        qDebug() << "成功从JSON导入类别，当前总数:" << categories.size();
    } catch (const std::exception &e) {
        qCritical() << "从JSON导入类别时发生异常:" << e.what();
        success = false;
    } catch (...) {
        qCritical() << "从JSON导入类别时发生未知异常";
        success = false;
    }

    return success;
}

// 私有辅助方法实现

/**
 * @brief 获取下一个可用ID
 * @param categories 类别列表
 * @return 下一个可用ID
 */
int CategoryDataStorage::获取下一个可用ID(const CategorieList &categories) const {
    int maxId = 0;
    for (const auto &category : categories) {
        if (category && category->id() > maxId) {
            maxId = category->id();
        }
    }
    return maxId + 1;
}

/**
 * @brief 处理冲突
 * @param categories 类别列表
 * @param newCategory 新类别
 * @param resolution 冲突解决策略
 * @return 处理成功返回true，否则返回false
 */
bool CategoryDataStorage::处理冲突(CategorieList &categories, const std::unique_ptr<CategorieItem> &newCategory,
                                   CategoryImportSource source, ConflictResolution resolution) {
    if (!newCategory)
        return false;

    // 查找冲突的类别
    auto it = std::find_if(
        categories.begin(), categories.end(), [&newCategory](const std::unique_ptr<CategorieItem> &existing) {
            return existing && (existing->name() == newCategory->name() || existing->uuid() == newCategory->uuid());
        });

    if (it == categories.end()) {
        // 没有冲突，直接添加
        categories.push_back(std::move(const_cast<std::unique_ptr<CategorieItem> &>(newCategory)));
        return true;
    }

    newCategory->setSynced(source == CategoryImportSource::Server ? 0 : 1);

    switch (resolution) {
    case Skip:
        return false; // 跳过冲突项目

    case Overwrite:
        // 覆盖现有项目
        *it = std::move(const_cast<std::unique_ptr<CategorieItem> &>(newCategory));
        return true;

    case Merge:
        // 合并（保留较新的版本）
        if (newCategory->updatedAt() > (*it)->updatedAt()) {
            *it = std::move(const_cast<std::unique_ptr<CategorieItem> &>(newCategory));
        }
        return true;

    default:
        return false;
    }
}
