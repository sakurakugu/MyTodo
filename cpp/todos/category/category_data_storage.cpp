/**
 * @file category_data_storage.cpp
 * @brief CategoryDataStorage类的实现文件
 *
 * 该文件实现了CategoryDataStorage类，专门负责类别的本地存储和文件导入功能。
 * 从CategoryManager类中拆分出来，实现单一职责原则。
 *
 * @author Sakurakugu
 * @date 2025-09-11 00:04:40(UTC+8) 周四
 * @change 2025-09-23 18:45:36(UTC+8) 周二
 */

#include "category_data_storage.h"
#include "../../foundation/config.h"
#include "category_item.h"
#include <QJsonObject>
#include <QSqlError>
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

        // 从数据库加载数据
        QSqlDatabase db = m_database.getDatabase();
        if (!db.isOpen()) {
            qCritical() << "数据库未打开，无法加载类别";
            return false;
        }

        QSqlQuery query(db);
        const QString queryString =
            "SELECT id, uuid, name, user_uuid, created_at, updated_at, synced FROM categories ORDER BY updated_at";
        if (!query.exec(queryString)) {
            qCritical() << "加载类别查询失败:" << query.lastError().text();
            return false;
        }

        while (query.next()) {
            int id = query.value("id").toInt();
            QUuid uuid = QUuid::fromString(query.value("uuid").toString());
            QString name = query.value("name").toString();
            QUuid userUuid = QUuid::fromString(query.value("user_uuid").toString());
            // 兼容两种存储格式：首选整数UTC毫秒，其次旧的ISO文本
            auto parseMs = [](const QVariant &v) {
                if (v.isNull())
                    return QDateTime();
                // bool ok = false;
                // qint64 ms = v.toLongLong(&ok);
                qint64 ms = v.toLongLong();
                // if (ok)
                return QDateTime::fromMSecsSinceEpoch(ms, QTimeZone::UTC);
                // QDateTime dt = QDateTime::fromString(v.toString(), Qt::ISODate);
                // return dt.isValid() ? dt.toUTC() : QDateTime();
            };
            QDateTime createdAt = parseMs(query.value("created_at"));
            QDateTime updatedAt = parseMs(query.value("updated_at"));
            int synced = query.value("synced").toInt();

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
        QSqlDatabase db = m_database.getDatabase();
        if (!db.isOpen()) {
            qCritical() << "数据库未打开，无法新增类别";
            return nullptr;
        }

        // 创建新的类别项
        QUuid newUuid = QUuid::createUuid();
        QDateTime createdAt = QDateTime::currentDateTimeUtc();

        // 插入到数据库
        QSqlQuery insertQuery(db);
        const QString insertString =
            "INSERT INTO categories (uuid, name, user_uuid, created_at, updated_at, synced) VALUES (?,?,?,?,?,?)";
        insertQuery.prepare(insertString);
        insertQuery.addBindValue(newUuid.toString());
        insertQuery.addBindValue(name);
        insertQuery.addBindValue(userUuid.toString());
        insertQuery.addBindValue(createdAt.toMSecsSinceEpoch());
        insertQuery.addBindValue(createdAt.toMSecsSinceEpoch());
        insertQuery.addBindValue(source == ImportSource::Server ? 0 : 1);

        if (!insertQuery.exec()) {
            qCritical() << "插入类别到数据库失败:" << insertQuery.lastError().text();
            return nullptr;
        }

        // 获取自增ID
        int newId = 获取最后插入行ID(db);
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
    QSqlDatabase db = m_database.getDatabase();
    if (!db.isOpen()) {
        qCritical() << "数据库未打开，无法更新类别";
        return false;
    }
    auto it = std::find_if(categories.begin(), categories.end(),
                           [name](const std::unique_ptr<CategorieItem> &i) { return i->name() == name; });
    QSqlQuery updateQuery(db);
    const QString updateString = "UPDATE categories SET name = ?, updated_at = ?, synced = ? WHERE name = ?";
    updateQuery.prepare(updateString);
    QDateTime now = QDateTime::currentDateTimeUtc();
    updateQuery.addBindValue(newName);
    updateQuery.addBindValue(now.toMSecsSinceEpoch());
    updateQuery.addBindValue((*it)->synced() != 1 ? 2 : 1); // 如果之前是插入状态，不要改成更新状态
    updateQuery.addBindValue(name);
    if (!updateQuery.exec()) {
        qCritical() << "更新数据库中的类别失败:" << updateQuery.lastError().text();
        return false;
    }
    if (updateQuery.numRowsAffected() == 0) {
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
    QSqlDatabase db = m_database.getDatabase();
    if (!db.isOpen()) {
        qCritical() << "数据库未打开，无法更新类别";
        return false;
    }
    QSqlQuery updateQuery(db);
    const QString updateString = "UPDATE categories SET synced = ?, updated_at = ? WHERE name = ?";
    updateQuery.prepare(updateString);
    QDateTime now = QDateTime::currentDateTimeUtc();
    updateQuery.addBindValue(synced);
    updateQuery.addBindValue(now.toMSecsSinceEpoch());
    updateQuery.addBindValue(name);
    if (!updateQuery.exec()) {
        qCritical() << "更新数据库中的类别同步状态失败:" << updateQuery.lastError().text();
        return false;
    }
    if (updateQuery.numRowsAffected() == 0) {
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
    QSqlDatabase db = m_database.getDatabase();
    if (!db.isOpen()) {
        qCritical() << "数据库未打开，无法创建默认类别";
        return false;
    }
    for (const auto &item : categories)
        if (item && item->id() == 1) {
            qDebug() << "内存中已存在默认类别";
            return true;
        }
    QUuid defaultUuid = QUuid::fromString("{00000000-0000-0000-0000-000000000001}");
    QDateTime createdAt = QDateTime::currentDateTimeUtc();
    if (!db.transaction()) {
        qCritical() << "无法开始数据库事务:" << db.lastError().text();
        return false;
    }
    QSqlQuery insertQuery(db);
    insertQuery.prepare("INSERT OR IGNORE INTO categories (id, uuid, name, user_uuid, created_at, updated_at, synced) "
                        "VALUES (?,?,?,?,?,?,?)");
    insertQuery.addBindValue(1);
    insertQuery.addBindValue(defaultUuid.toString());
    insertQuery.addBindValue("未分类");
    insertQuery.addBindValue(userUuid.toString());
    insertQuery.addBindValue(createdAt.toMSecsSinceEpoch());
    insertQuery.addBindValue(createdAt.toMSecsSinceEpoch());
    insertQuery.addBindValue(0);
    if (!insertQuery.exec()) {
        qCritical() << "插入默认类别失败:" << insertQuery.lastError().text();
        db.rollback();
        return false;
    }
    bool wasInserted = insertQuery.numRowsAffected() > 0;
    if (!db.commit()) {
        qCritical() << "提交数据库事务失败:" << db.lastError().text();
        return false;
    }
    QSqlQuery selectQuery(db);
    selectQuery.prepare(
        "SELECT id, uuid, name, user_uuid, created_at, updated_at, synced FROM categories WHERE id=1 AND user_uuid=?");
    selectQuery.addBindValue(userUuid.toString());
    if (selectQuery.exec() && selectQuery.next()) {
        auto parse = [](const QVariant &v) {
            if (v.isNull())
                return QDateTime();
            bool ok = false;
            qint64 ms = v.toLongLong(&ok);
            return ok ? QDateTime::fromMSecsSinceEpoch(ms, QTimeZone::UTC) : QDateTime();
        };
        int id = selectQuery.value("id").toInt();
        QUuid uuid = QUuid::fromString(selectQuery.value("uuid").toString());
        QString name = selectQuery.value("name").toString();
        QUuid dbUserUuid = QUuid::fromString(selectQuery.value("user_uuid").toString());
        QDateTime createdAt = parse(selectQuery.value("created_at"));
        QDateTime updatedAt = parse(selectQuery.value("updated_at"));
        int synced = selectQuery.value("synced").toInt();
        auto defaultItem = std::make_unique<CategorieItem>(id, uuid, name, dbUserUuid, createdAt, updatedAt, synced);
        categories.push_back(std::move(defaultItem));
        qDebug() << (wasInserted ? "成功创建默认类别" : "默认类别已存在，已加载到内存");
    } else {
        qWarning() << "无法从数据库加载默认类别:" << selectQuery.lastError().text();
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

    QSqlDatabase db = m_database.getDatabase();
    if (!db.isOpen()) {
        qCritical() << "数据库未打开，无法导入类别";
        return false;
    }

    // 构建现有类别索引（按 uuid 与 name）
    QHash<QString, CategorieItem *> nameIndex;
    QHash<QString, CategorieItem *> uuidIndex;
    for (auto &item : categories) {
        if (item) {
            uuidIndex.insert(item->uuid().toString(QUuid::WithoutBraces), item.get());
            nameIndex.insert(item->name(), item.get());
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

            QString name = obj.value("name").toString();
            QUuid userUuid = QUuid::fromString(obj.value("user_uuid").toString());
            if (userUuid.isNull()) {
                qWarning() << "跳过无效类别（user_uuid 无效）";
                ++skipCount;
                continue;
            }

            QUuid uuid = obj.contains("uuid") ? QUuid::fromString(obj.value("uuid").toString()) : QUuid::createUuid();
            if (uuid.isNull())
                uuid = QUuid::createUuid();

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
                QSqlQuery updateQuery(db);
                updateQuery.prepare("UPDATE categories SET name = ?, user_uuid = ?, created_at = ?, updated_at = ?, "
                                    "synced = ? WHERE uuid = ? OR name = ?");
                updateQuery.addBindValue(name);
                updateQuery.addBindValue(userUuid.toString());
                updateQuery.addBindValue(createdAt.toUTC().toMSecsSinceEpoch());
                updateQuery.addBindValue(updatedAt.toUTC().toMSecsSinceEpoch());
                updateQuery.addBindValue(source == ImportSource::Server ? 0 : (existing->synced() == 1 ? 1 : 2));
                updateQuery.addBindValue(existing->uuid().toString());
                updateQuery.addBindValue(existing->name());
                if (!updateQuery.exec()) {
                    qCritical() << "更新类别失败(uuid=" << existing->uuid() << "):" << updateQuery.lastError().text();
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
        qCritical() << "导入类别时发生异常:" << e.what();
        db.rollback();
        success = false;
    } catch (...) {
        qCritical() << "导入类别时发生未知异常";
        db.rollback();
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
    QSqlDatabase db = m_database.getDatabase();
    if (!db.isOpen()) {
        qCritical() << "数据库未打开，无法初始化Category表";
        return false;
    }

    return 创建数据表();
}

/**
 * @brief 创建categories表
 * @return 创建是否成功
 */
bool CategoryDataStorage::创建数据表() {
    const QString createTableQuery = R"(
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
    QSqlDatabase db = m_database.getDatabase();
    QSqlQuery query(db);
    if (!query.exec(createTableQuery)) {
        qCritical() << "创建categories表失败:" << query.lastError().text();
        return false;
    }

    // 创建索引
    const QStringList indexes = //
        {"CREATE INDEX IF NOT EXISTS idx_categories_uuid ON categories(uuid)",
         "CREATE INDEX IF NOT EXISTS idx_categories_user_uuid ON categories(user_uuid)",
         "CREATE INDEX IF NOT EXISTS idx_categories_name ON categories(name)"};
    for (const QString &idx : indexes)
        if (!query.exec(idx))
            qWarning() << "创建categories表索引失败:" << query.lastError().text();
    qDebug() << "categories表初始化成功";
    return true;
}

/**
 * @brief 导出类别数据到JSON对象
 * @param output 输出的JSON对象引用
 * @return 导出成功返回true，否则返回false
 */
bool CategoryDataStorage::导出到JSON(QJsonObject &output) {
    QSqlDatabase db = m_database.getDatabase();
    if (!db.isOpen()) {
        qWarning() << "数据库未打开，无法导出类别数据";
        return false;
    }
    QSqlQuery query(db);
    const QString queryString = "SELECT id, uuid, name, user_uuid, created_at, updated_at, synced FROM categories";
    if (!query.exec(queryString)) {
        qWarning() << "查询类别数据失败:" << query.lastError().text();
        return false;
    }
    auto toIso = [&](const QVariant &v) -> QJsonValue {
        if (v.isNull())
            return QJsonValue();
        bool ok = false;
        qint64 ms = v.toLongLong(&ok);
        if (!ok)
            return QJsonValue();
        return QDateTime::fromMSecsSinceEpoch(ms, QTimeZone::UTC).toString(Qt::ISODateWithMs);
    };
    QJsonArray arr;
    while (query.next()) {
        QJsonObject obj;
        obj["id"] = query.value("id").toInt();
        obj["uuid"] = query.value("uuid").toString();
        obj["name"] = query.value("name").toString();
        obj["user_uuid"] = query.value("user_uuid").toString();
        obj["created_at"] = toIso(query.value("created_at"));
        obj["updated_at"] = toIso(query.value("updated_at"));
        obj["synced"] = query.value("synced").toInt();
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
bool CategoryDataStorage::导入从JSON(const QJsonObject &input, bool replaceAll) {
    QSqlDatabase db = m_database.getDatabase();
    if (!db.isOpen()) {
        qWarning() << "数据库未打开，无法导入类别数据";
        return false;
    }

    if (!input.contains("categories") || !input["categories"].isArray()) {
        return true; // 没有类别数据可导入
    }

    QSqlQuery query(db);

    // 如果是替换模式，先清空表
    if (replaceAll) {
        if (!query.exec("DELETE FROM categories")) {
            qWarning() << "清空类别表失败:" << query.lastError().text();
            return false;
        }
    }

    // 导入类别数据
    QJsonArray arr = input["categories"].toArray();
    for (const auto &v : arr) {
        QJsonObject obj = v.toObject();

        query.prepare("INSERT OR REPLACE INTO categories (id, uuid, name, user_uuid, created_at, updated_at, synced) "
                      "VALUES (?,?,?,?,?,?,?)");
        query.addBindValue(obj.value("id").toVariant());
        query.addBindValue(obj.value("uuid").toString());
        query.addBindValue(obj.value("name").toString());
        query.addBindValue(obj.value("user_uuid").toString());
        auto bindIso = [&](const char *k) {
            if (obj.contains(k) && !obj.value(k).isNull()) {
                QDateTime dt = QDateTime::fromString(obj.value(k).toString(), Qt::ISODate);
                query.addBindValue(dt.isValid() ? QVariant(dt.toUTC().toMSecsSinceEpoch()) : QVariant());
            } else
                query.addBindValue(QVariant());
        };
        bindIso("created_at");
        bindIso("updated_at");
        query.addBindValue(obj.value("synced").toInt());
        if (!query.exec()) {
            qWarning() << "导入类别数据失败:" << query.lastError().text();
            return false;
        }
    }

    qInfo() << "成功导入" << arr.size() << "条类别记录";
    return true;
}
