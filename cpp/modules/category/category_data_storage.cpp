/**
 * @file category_data_storage.cpp
 * @brief CategoryDataStorage类的实现文件
 *
 * 该文件实现了CategoryDataStorage类，专门负责类别的本地存储和文件导入功能。
 * 从CategoryManager类中拆分出来，实现单一职责原则。
 *
 * @author Sakurakugu
 * @date 2025-09-11 00:04:40(UTC+8) 周四
 * @change 2025-10-06 02:13:23(UTC+8) 周一
 */

#include "category_data_storage.h"
#include "category_item.h"
#include "config.h"
#include "utility.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QTimeZone>

CategoryDataStorage::CategoryDataStorage(QObject *parent)
    : BaseDataStorage("categories", parent) // 调用基类构造函数
{
    // 在子类构造完成后初始化数据表
    初始化();
}

CategoryDataStorage::~CategoryDataStorage() {
    // 基类析构函数会自动处理注销
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

        auto query = m_database.createQuery();
        const std::string queryString =
            "SELECT id, uuid, name, user_uuid, created_at, updated_at, synced FROM categories ORDER BY updated_at";
        if (!query->exec(queryString)) {
            qCritical() << "加载类别查询失败:" << query->lastError();
            return false;
        }

        while (query->next()) {
            int id = std::get<int>(query->value("id"));
            QUuid uuid = QUuid::fromString(std::get<QString>(query->value("uuid")));
            QString name = std::get<QString>(query->value("name"));
            QUuid userUuid = QUuid::fromString(std::get<QString>(query->value("user_uuid")));
            QDateTime createdAt = Utility::timestampToDateTime(std::get<int64_t>(query->value("created_at")));
            QDateTime updatedAt = Utility::timestampToDateTime(std::get<int64_t>(query->value("updated_at")));
            int synced = std::get<int>(query->value("synced"));

            auto item = std::make_unique<CategorieItem>( //
                id,                                      // 唯一标识符
                uuid,                                    // 唯一标识符（UUID）
                name,                                    // 分类名称
                userUuid,                                // 用户UUID
                createdAt,                               // 创建时间
                updatedAt,                               // 更新时间
                synced                                   // 是否已同步
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
 * @param source 导入来源
 * @return 添加成功返回新创建的CategorieItem指针，否则返回nullptr
 */
std::unique_ptr<CategorieItem> CategoryDataStorage::新增类别( //
    CategorieList &categories, const QString &name, const QUuid &userUuid, ImportSource source) {
    try {
        // 创建新的类别项
        QUuid newUuid = QUuid::createUuid();
        QDateTime createdAt = QDateTime::currentDateTimeUtc();

        // 插入到数据库
        auto query = m_database.createQuery();
        query->prepare( //
            "INSERT INTO categories (uuid, name, user_uuid, created_at, updated_at, synced) VALUES (?,?,?,?,?,?)");
        query->bindValues(                 //
            newUuid.toString(),            //
            name,                          //
            userUuid.toString(),           //
            createdAt.toMSecsSinceEpoch(), //
            createdAt.toMSecsSinceEpoch(), //
            source == ImportSource::Server ? 0 : 1);

        if (!query->exec()) {
            qCritical() << "插入类别到数据库失败:" << query->lastError();
            return nullptr;
        }

        // 获取自增ID
        int newId = query->lastInsertRowId();
        auto newItem = std::make_unique<CategorieItem>( //
            newId,                                      // 占位ID，插入后回填
            newUuid,                                    // 新的UUID
            name,                                       // 类别名称
            userUuid,                                   // 用户UUID
            createdAt,                                  // 创建时间
            createdAt,                                  // 更新时间
            source == ImportSource::Server ? 0 : 1      // 默认未同步，插入
        );

        // 添加到内存列表
        categories.push_back(std::move(newItem));

        qDebug() << "成功新增类别:" << name << "ID:" << newId;
        return newItem;
    } catch (const std::exception &e) {
        qCritical() << "新增类别时发生异常:" << e.what();
        return nullptr;
    } catch (...) {
        qCritical() << "新增类别时发生未知异常";
        return nullptr;
    }
}

/**
 * @brief 更新类别
 * @param categories 类别列表引用
 * @param name 类别名称
 * @param newName 新的类别名称
 * @return 更新成功返回true，否则返回false
 */
bool CategoryDataStorage::更新类别(CategorieList &categories, const QString &name, const QString &newName) {
    auto it = std::find_if(categories.begin(), categories.end(),
                           [name](const std::unique_ptr<CategorieItem> &i) { return i->name() == name; });
    auto query = m_database.createQuery();
    QDateTime now = QDateTime::currentDateTimeUtc();
    const std::string updateString = "UPDATE categories SET name = ?, updated_at = ?, synced = ? WHERE name = ?";
    query->bindValues(                //
        newName,                      //
        now.toMSecsSinceEpoch(),      //
        now.toMSecsSinceEpoch(),      //
        (*it)->synced() != 1 ? 2 : 1, // 如果之前是插入状态，不要改成更新状态
        name);
    if (!query->exec()) {
        qCritical() << "更新数据库中的类别失败:" << query->lastError();
        return false;
    }
    if (query->rowsAffected() == 0) {
        qWarning() << "未找到要更新的类别，名称:" << name;
        return false;
    }
    if (it != categories.end()) {
        (*it)->setName(newName);
        (*it)->setSynced(2); // 标记为未同步，待更新
        (*it)->setUpdatedAt(now);
    }
    qDebug() << "成功更新类别:" << name;
    return true;
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
        // 从数据库中删除类别
        auto deleteQuery = m_database.createQuery();
        deleteQuery->prepare("DELETE FROM categories WHERE name = ?");
        deleteQuery->bindValues(0, name);

        if (!deleteQuery->exec()) {
            qCritical() << "从数据库删除类别失败:" << deleteQuery->lastError();
            return false;
        }

        if (deleteQuery->rowsAffected() == 0) {
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
        // 更新内存中的类别
        auto it = std::find_if(categories.begin(), categories.end(),
                               [name](const std::unique_ptr<CategorieItem> &item) { return item->name() == name; });

        if (it != categories.end()) {
            if ((*it)->synced() == 1) {
                success = 删除类别(categories, name); // 如果之前是插入状态，直接从内存中删除
            } else {
                success = 更新同步状态(categories, name, 3); // 标记为未同步，待删除
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
 * @return 成功返回item在待办类别中的指针，否则返回nullptr
 */
bool CategoryDataStorage::更新同步状态(CategorieList &categories, const QString &name, int synced) {
    auto updateQuery = m_database.createQuery();
    QDateTime now = QDateTime::currentDateTimeUtc();
    const std::string updateString = "UPDATE categories SET synced = ?, updated_at = ? WHERE name = ?";
    updateQuery->bindValues( //
        synced, now.toMSecsSinceEpoch(), name);
    if (!updateQuery->exec()) {
        qCritical() << "更新数据库中的类别同步状态失败:" << updateQuery->lastError();
        return false;
    }
    if (updateQuery->rowsAffected() == 0) {
        qWarning() << "未找到要更新的类别，名称:" << name;
        return false;
    }
    auto it = std::find_if(categories.begin(), categories.end(),
                           [name](const std::unique_ptr<CategorieItem> &i) { return i->name() == name; });
    if (it != categories.end()) {
        (*it)->setSynced(synced);
        (*it)->setUpdatedAt(now);
        qDebug() << "成功更新类别:" << (*it)->name() << "的同步状态 ->" << synced;
    }
    return true;
}

/**
 * @brief 创建默认类别
 * @param categories 类别列表引用
 * @param userUuid 用户UUID
 */
bool CategoryDataStorage::创建默认类别(CategorieList &categories, const QUuid &userUuid) {
    for (const auto &item : categories)
        if (item && item->id() == 1) {
            qDebug() << "内存中已存在默认类别";
            return true;
        }
    QUuid defaultUuid = QUuid::fromString("{00000000-0000-0000-0000-000000000001}");
    QDateTime createdAt = QDateTime::currentDateTimeUtc();
    if (!m_database.beginTransaction()) {
        qCritical() << "无法开始数据库事务:" << m_database.lastError();
        return false;
    }
    auto insertQuery = m_database.createQuery();
    insertQuery->prepare("INSERT OR IGNORE INTO categories (id, uuid, name, user_uuid, created_at, updated_at, synced) "
                         "VALUES (?,?,?,?,?,?,?)");
    insertQuery->bindValues(1,                             //
                            defaultUuid.toString(),        //
                            "未分类",                      //
                            userUuid.toString(),           //
                            createdAt.toMSecsSinceEpoch(), //
                            createdAt.toMSecsSinceEpoch(), //
                            0);
    if (!insertQuery->exec()) {
        qCritical() << "插入默认类别失败:" << insertQuery->lastError();
        m_database.rollbackTransaction();
        return false;
    }
    bool wasInserted = insertQuery->rowsAffected() > 0;
    if (!m_database.commitTransaction()) {
        qCritical() << "提交数据库事务失败:" << m_database.lastError();
        return false;
    }
    auto selectQuery = m_database.createQuery();
    selectQuery->prepare(
        "SELECT id, uuid, name, user_uuid, created_at, updated_at, synced FROM categories WHERE id=1 AND user_uuid=?");
    selectQuery->bindValue(0, userUuid.toString().toStdString());
    if (!selectQuery->exec()) {
        qCritical() << "查询默认类别失败:" << selectQuery->lastError();
        return false;
    }
    if (selectQuery->next()) {
        int id = std::get<int>(selectQuery->value("id"));
        QUuid uuid = QUuid::fromString(std::get<QString>(selectQuery->value("uuid")));
        QString name = std::get<QString>(selectQuery->value("name"));
        QUuid dbUserUuid = QUuid::fromString(std::get<QString>(selectQuery->value("user_uuid")));
        QDateTime createdAt = Utility::timestampToDateTime(std::get<QDateTime>(selectQuery->value("created_at")));
        QDateTime updatedAt = Utility::timestampToDateTime(std::get<QDateTime>(selectQuery->value("updated_at")));
        int synced = std::get<int>(selectQuery->value("synced"));
        auto defaultItem = std::make_unique<CategorieItem>(id, uuid, name, dbUserUuid, createdAt, updatedAt, synced);
        categories.push_back(std::move(defaultItem));
        qDebug() << (wasInserted ? "成功创建默认类别" : "默认类别已存在，已加载到内存");
    } else {
        qWarning() << "无法从数据库加载默认类别:" << selectQuery->lastError();
        return false;
    }
    return true;
}

/**
 * @brief 导入类别从JSON
 * @param categories 类别列表引用
 * @param categoriesArray JSON类别数组
 * @param source 来源（服务器或本地备份）
 * @param resolution 冲突解决策略
 */
bool CategoryDataStorage::导入类别从JSON(CategorieList &categories, const QJsonArray &categoriesArray,
                                         ImportSource source, 解决冲突方案 resolution) {
    bool success = true;

    // 构建现有类别索引（按 uuid 与 name）
    QHash<QString, CategorieItem *> nameIndex;
    QHash<QString, CategorieItem *> uuidIndex;
    for (auto &item : categories) {
        if (item) {
            uuidIndex.insert(item->uuid().toString(QUuid::WithoutBraces), item.get());
            nameIndex.insert(item->name(), item.get());
        }
    }

    if (!m_database.beginTransaction()) {
        qCritical() << "无法开启事务以导入类别:" << m_database.lastError();
        return false;
    }

    int insertCount = 0;
    int updateCount = 0;
    int skipCount = 0;

    try {
        for (const QJsonValue &value : categoriesArray) {
            if (!value.isObject()) {
                qWarning() << "跳过无效类别（非对象）";
                ++skipCount;
                continue;
            }
            QJsonObject obj = value.toObject();
            if (!obj.contains("name") || !obj.contains("user_uuid")) {
                qWarning() << "跳过无效类别（缺字段）";
                ++skipCount;
                continue;
            }

            QString name = obj["name"].toString();
            QUuid userUuid = QUuid::fromString(obj["user_uuid"].toString());
            if (userUuid.isNull()) {
                qWarning() << "跳过无效类别（user_uuid 无效）";
                ++skipCount;
                continue;
            }

            QUuid uuid = obj.contains("uuid") ? QUuid::fromString(obj["uuid"].toString()) : QUuid::createUuid();
            if (uuid.isNull())
                uuid = QUuid::createUuid();

            QDateTime createdAt = obj.contains("created_at") ? Utility::fromIsoString(obj["created_at"].toString())
                                                             : QDateTime::currentDateTime();
            if (!createdAt.isValid())
                createdAt = QDateTime::currentDateTime();
            QDateTime updatedAt =
                obj.contains("updated_at") ? Utility::fromIsoString(obj["updated_at"].toString()) : createdAt;
            if (!updatedAt.isValid())
                updatedAt = createdAt;

            // 构造临时 incoming 对象（不立即放入列表）
            CategorieItem incoming(-1, uuid, name, userUuid, createdAt, updatedAt,
                                   source == ImportSource::Server ? 0 : 1);

            // 查找现有
            CategorieItem *existing = uuidIndex.value(uuid.toString(QUuid::WithoutBraces), nullptr);
            if (!existing)
                existing = nameIndex.value(name, nullptr);

            解决冲突方案 action = 评估冲突(existing, incoming, resolution);
            if (action == 解决冲突方案::Skip) {
                ++skipCount;
                continue;
            }

            if (action == 解决冲突方案::Insert) {
                auto newPtr = 新增类别(categories, name, userUuid, source);
                // 更新索引
                nameIndex.insert(name, newPtr.get());
                uuidIndex.insert(uuid.toString(QUuid::WithoutBraces), newPtr.get());
                ++insertCount;
            } else if (action == 解决冲突方案::Overwrite && existing) {
                auto updateQuery = m_database.createQuery();
                updateQuery->prepare("UPDATE categories SET name = ?, user_uuid = ?, created_at = ?, updated_at = ?, "
                                     "synced = ? WHERE uuid = ? OR name = ?");
                updateQuery->bindValues(                                                    //
                    name,                                                                   //
                    userUuid.toString(),                                                    //
                    createdAt.toUTC().toMSecsSinceEpoch(),                                  //
                    updatedAt.toUTC().toMSecsSinceEpoch(),                                  //
                    source == ImportSource::Server ? 0 : (existing->synced() == 1 ? 1 : 2), //
                    existing->uuid().toString(),                                            //
                    existing->name()                                                        //
                );

                if (!updateQuery->exec()) {
                    qCritical() << "更新类别失败(uuid=" << existing->uuid() << "):" << updateQuery->lastError();
                    success = false;
                    break;
                }
                // 更新内存
                existing->setName(name);
                existing->setUserUuid(userUuid);
                existing->setCreatedAt(createdAt);
                existing->setUpdatedAt(updatedAt);
                existing->setSynced(source == ImportSource::Server ? 0 : 2);
                ++updateCount;
            }
        }

        if (success) {
            if (!m_database.commitTransaction()) {
                qCritical() << "提交事务失败:" << m_database.lastError();
                success = false;
                m_database.rollbackTransaction();
            } else {
                qDebug() << "导入完成 - 新增:" << insertCount << ", 更新:" << updateCount << ", 跳过:" << skipCount;
            }
        } else {
            m_database.rollbackTransaction();
        }
    } catch (const std::exception &e) {
        qCritical() << "导入类别时发生异常:" << e.what();
        m_database.rollbackTransaction();
        success = false;
    } catch (...) {
        qCritical() << "导入类别时发生未知异常";
        m_database.rollbackTransaction();
        success = false;
    }

    return success;
}

// 私有辅助方法实现

/**
 * @brief 初始化Category表
 * @return 初始化是否成功
 */
bool CategoryDataStorage::初始化数据表() {
    // 确保数据库连接已建立
    return 创建数据表();
}

/**
 * @brief 创建categories表
 * @return 创建是否成功
 */
bool CategoryDataStorage::创建数据表() {
    const std::string createTableQuery = R"(
        CREATE TABLE IF NOT EXISTS categories (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            uuid TEXT UNIQUE NOT NULL,
            name TEXT NOT NULL,
            user_uuid TEXT NOT NULL,
            created_at INTEGER NOT NULL,
            updated_at INTEGER NOT NULL,
            synced INTEGER NOT NULL DEFAULT 1
        )
    )";
    auto createQuery = m_database.createQuery();
    if (!createQuery->exec(createTableQuery)) {
        qCritical() << "创建categories表失败:" << createQuery->lastError();
        return false;
    }

    // 创建索引
    const std::vector<std::string> indexes = //
        {"CREATE INDEX IF NOT EXISTS idx_categories_uuid ON categories(uuid)",
         "CREATE INDEX IF NOT EXISTS idx_categories_user_uuid ON categories(user_uuid)",
         "CREATE INDEX IF NOT EXISTS idx_categories_name ON categories(name)"};
    for (const std::string &idx : indexes)
        if (!createQuery->exec(idx))
            qWarning() << "创建categories表索引失败:" << createQuery->lastError();
    qDebug() << "categories表初始化成功";
    return true;
}

/**
 * @brief 导出类别数据到JSON对象
 * @param output 输出的JSON对象引用
 * @return 导出成功返回true，否则返回false
 */
bool CategoryDataStorage::exportToJson(QJsonObject &output) {
    auto query = m_database.createQuery();
    const std::string queryString = "SELECT id, uuid, name, user_uuid, created_at, updated_at, synced FROM categories";
    if (!query->exec(queryString)) {
        qWarning() << "查询类别数据失败:" << query->lastError();
        return false;
    }
    QJsonArray arr;
    while (query->next()) {
        QJsonObject obj;
        // TODO:id 是自增主键，不应该导出
        obj["id"] = std::get<int>(query->value("id"));
        obj["uuid"] = std::get<QString>(query->value("uuid"));
        obj["name"] = std::get<QString>(query->value("name"));
        obj["user_uuid"] = std::get<QString>(query->value("user_uuid"));
        obj["created_at"] = Utility::timestampToIsoJson(std::get<QDateTime>(query->value("created_at")));
        obj["updated_at"] = Utility::timestampToIsoJson(std::get<QDateTime>(query->value("updated_at")));
        obj["synced"] = std::get<int>(query->value("synced"));
        arr.append(obj);
    }
    output["categories"] = arr;
    return true;
}

/**
 * @brief 从JSON对象导入类别数据
 * @param input 输入的JSON对象引用
 * @param replaceAll 是否替换现有数据
 * @return 导入成功返回true，否则返回false
 */
bool CategoryDataStorage::importFromJson(const QJsonObject &input, bool replaceAll) {
    if (!input.contains("categories") || !input["categories"].isArray()) {
        return true; // 没有类别数据可导入
    }

    auto query = m_database.createQuery();

    // 如果是替换模式，先清空表
    if (replaceAll) {
        if (!query->exec("DELETE FROM categories")) {
            qWarning() << "清空类别表失败:" << query->lastError();
            return false;
        }
    }

    // 导入类别数据
    QJsonArray arr = input["categories"].toArray();
    for (const auto &v : arr) {
        QJsonObject obj = v.toObject();
        // TODO:id 是自增主键，不应该从JSON导入
        query->prepare( //
            "INSERT OR REPLACE INTO categories (id, uuid, name, user_uuid, created_at, updated_at, synced) VALUES "
            "(?,?,?,?,?,?,?)");
        query->bindValues(                             //
            obj["id"].toInt(),                         //
            obj["uuid"].toString(),                    //
            obj["name"].toString(),                    //
            obj["user_uuid"].toString(),               //
            Utility::fromJsonValue(obj["created_at"]), //
            Utility::fromJsonValue(obj["updated_at"]), //
            obj["synced"].toInt()                      //
        );
        if (!query->exec()) {
            qWarning() << "导入类别数据失败:" << query->lastError();
            return false;
        }
    }

    qInfo() << "成功导入" << arr.size() << "条类别记录";
    return true;
}
