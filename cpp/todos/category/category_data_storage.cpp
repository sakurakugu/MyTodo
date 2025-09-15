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

#include <QJsonObject>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include "category_data_storage.h"
#include "../../foundation/config.h"

CategoryDataStorage::CategoryDataStorage(QObject *parent)
    : QObject(parent),                                    // 父对象
      m_config(Config::GetInstance()),                    // 配置
      m_databaseManager(DatabaseManager::GetInstance())   // 数据库管理器
{
    // 确保数据库已初始化
    if (!m_databaseManager.initializeDatabase()) {
        qCritical() << "CategoryDataStorage: 数据库初始化失败";
    }
}

CategoryDataStorage::~CategoryDataStorage() {
}

/**
 * @brief 从本地存储加载类别
 * @param categories 类别列表引用
 * @return 加载成功返回true，否则返回false
 */
bool CategoryDataStorage::loadFromLocalStorage(std::vector<std::unique_ptr<CategorieItem>> &categories) {
    bool success = true;

    try {
        // 清除当前数据
        categories.clear();

        // 从数据库加载数据
        QSqlDatabase db = m_databaseManager.getDatabase();
        if (!db.isOpen()) {
            qCritical() << "数据库未打开，无法加载类别";
            return false;
        }

        QSqlQuery query(db);
        const QString queryString = "SELECT id, uuid, name, user_uuid, created_at, synced FROM categories ORDER BY id";
        
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
            bool synced = query.value("synced").toBool();

            auto item = std::make_unique<CategorieItem>(
                id,        // 唯一标识符
                uuid,      // 唯一标识符（UUID）
                name,      // 分类名称
                userUuid,  // 用户UUID
                createdAt, // 创建时间
                synced,    // 是否已同步
                this);

            categories.push_back(std::move(item));
        }

        qDebug() << "成功从数据库加载" << categories.size() << "个分类";
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
 * @brief 删除类别
 * @param categories 类别列表引用
 * @param id 类别ID
 * @return 删除成功返回true，否则返回false
 */
bool CategoryDataStorage::deleteCategory(std::vector<std::unique_ptr<CategorieItem>> &categories, int id) {
    bool success = false;

    try {
        QSqlDatabase db = m_databaseManager.getDatabase();
        if (!db.isOpen()) {
            qCritical() << "数据库未打开，无法删除类别";
            return false;
        }

        // 从数据库中删除类别
        QSqlQuery deleteQuery(db);
        const QString deleteString = "DELETE FROM categories WHERE id = ?";
        deleteQuery.prepare(deleteString);
        deleteQuery.addBindValue(id);

        if (!deleteQuery.exec()) {
            qCritical() << "从数据库删除类别失败:" << deleteQuery.lastError().text();
            return false;
        }

        if (deleteQuery.numRowsAffected() == 0) {
            qWarning() << "未找到要删除的类别，ID:" << id;
            return false;
        }

        // 从内存中删除类别
        auto it = std::find_if(categories.begin(), categories.end(),
                               [id](const std::unique_ptr<CategorieItem> &item) {
                                   return item->id() == id;
                               });

        if (it != categories.end()) {
            QString name = (*it)->name();
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
 * @brief 将类别保存到本地存储
 * @param categories 类别列表
 * @return 保存成功返回true，否则返回false
 */
bool CategoryDataStorage::saveToLocalStorage(const std::vector<std::unique_ptr<CategorieItem>> &categories) {
    bool success = true;

    try {
        QSqlDatabase db = m_databaseManager.getDatabase();
        if (!db.isOpen()) {
            qCritical() << "数据库未打开，无法保存类别";
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

        // 插入新数据
        QSqlQuery insertQuery(db);
        const QString insertString = "INSERT INTO categories (id, uuid, name, user_uuid, created_at, synced) VALUES (?, ?, ?, ?, ?, ?)";
        insertQuery.prepare(insertString);

        for (const auto &item : categories) {
            insertQuery.addBindValue(item->id());
            insertQuery.addBindValue(item->uuid().toString());
            insertQuery.addBindValue(item->name());
            insertQuery.addBindValue(item->userUuid().toString());
            insertQuery.addBindValue(item->createdAt().toString(Qt::ISODate));
            insertQuery.addBindValue(item->synced());

            if (!insertQuery.exec()) {
                qCritical() << "插入类别数据失败:" << insertQuery.lastError().text();
                db.rollback();
                return false;
            }
        }

        // 提交事务
        if (!db.commit()) {
            qCritical() << "提交数据库事务失败:" << db.lastError().text();
            return false;
        }

        qDebug() << "已成功保存" << categories.size() << "个分类到数据库";
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
 * @brief 从TOML表导入类别（指定冲突解决策略）
 * @param table TOML表
 * @param categories 类别列表引用
 * @param resolution 冲突解决策略
 * @return 导入成功返回true，否则返回false
 */
bool CategoryDataStorage::importFromToml(const toml::table &table,
                                         std::vector<std::unique_ptr<CategorieItem>> &categories,
                                         ConflictResolution resolution) {
    int importedCount = 0;
    int skippedCount = 0;
    int conflictCount = 0;

    try {
        // 查找categories数组
        auto categoriesArray = table["categories"].as_array();
        if (!categoriesArray) {
            qDebug() << "TOML中未找到categories数组";
            return true; // 不算错误
        }

        for (const auto &element : *categoriesArray) {
            auto categoryTable = element.as_table();
            if (!categoryTable) {
                skippedCount++;
                continue;
            }

            try {
                int newId = getNextAvailableId(categories);
                auto newCategory = createCategoryItemFromToml(*categoryTable, newId);

                if (!newCategory) {
                    skippedCount++;
                    continue;
                }

                // 检查冲突
                bool hasConflict = false;
                for (const auto &existingCategory : categories) {
                    if (existingCategory->name() == newCategory->name() ||
                        existingCategory->uuid() == newCategory->uuid()) {
                        hasConflict = true;
                        conflictCount++;
                        break;
                    }
                }

                // 处理冲突
                if (hasConflict) {
                    if (!handleConflict(newCategory, categories, resolution)) {
                        skippedCount++;
                        continue;
                    }
                } else {
                    categories.push_back(std::move(newCategory));
                }

                importedCount++;

            } catch (const std::exception &e) {
                qWarning() << "导入单个类别时发生异常:" << e.what();
                skippedCount++;
            }
        }

        qDebug() << "类别导入完成 - 导入:" << importedCount << "跳过:" << skippedCount << "冲突:" << conflictCount;
        return true;

    } catch (const std::exception &e) {
        qWarning() << "从TOML导入类别时发生异常:" << e.what();
        return false;
    }
}

/**
 * @brief 创建默认类别
 * @param categories 类别列表引用
 * @param userUuid 用户UUID
 */
void CategoryDataStorage::createDefaultCategories(std::vector<std::unique_ptr<CategorieItem>> &categories,
                                                  const QUuid &userUuid) {
    // 清空现有类别
    categories.clear();
    // 添加默认的"未分类"选项
    auto defaultCategory = std::make_unique<CategorieItem>( //
        1,                                                  //
        QUuid::createUuid(),                                //
        "未分类",                                           //
        userUuid,                                           //
        QDateTime::currentDateTime(),                       //
        false                                               //
    );
    categories.push_back(std::move(defaultCategory));
}

/**
 * @brief 检查是否有默认类别
 * @param categories 类别列表
 * @return 有默认类别返回true，否则返回false
 */
bool CategoryDataStorage::hasDefaultCategory(const std::vector<std::unique_ptr<CategorieItem>> &categories) const {
    for (const auto &category : categories) {
        if (category && category->name() == "未分类") {
            return true;
        }
    }
    return false;
}

/**
 * @brief 验证类别数据
 * @param categoryObj JSON对象
 * @return 验证通过返回true，否则返回false
 */
bool CategoryDataStorage::validateCategoryData(const QJsonObject &categoryObj) const {
    // 检查必需字段
    if (!categoryObj.contains("name") || categoryObj["name"].toString().trimmed().isEmpty()) {
        return false;
    }

    // 检查名称长度
    QString name = categoryObj["name"].toString().trimmed();
    if (!isValidCategoryName(name)) {
        return false;
    }

    return true;
}

// 私有辅助方法实现

/**
 * @brief 从TOML表创建类别项目
 * @param categoryTable TOML表
 * @param newId 新ID
 * @return 类别项目智能指针
 */
std::unique_ptr<CategorieItem> CategoryDataStorage::createCategoryItemFromToml(const toml::table &categoryTable,
                                                                               int newId) {
    try {
        auto nameNode = categoryTable["name"];
        if (!nameNode) {
            qWarning() << "TOML类别缺少name字段";
            return nullptr;
        }

        QString name = QString::fromStdString(nameNode.value_or(""));
        if (!isValidCategoryName(name)) {
            qWarning() << "无效的类别名称:" << name;
            return nullptr;
        }

        int id = categoryTable["id"].value_or(newId);
        QString uuidStr = QString::fromStdString(categoryTable["uuid"].value_or(""));
        QString userUuidStr = QString::fromStdString(categoryTable["user_uuid"].value_or(""));
        QString createdAtStr = QString::fromStdString(categoryTable["created_at"].value_or(""));
        bool synced = categoryTable["synced"].value_or(false);

        QUuid uuid = uuidStr.isEmpty() ? QUuid::createUuid() : QUuid::fromString(uuidStr);
        QUuid userUuid = QUuid::fromString(userUuidStr);
        QDateTime createdAt =
            createdAtStr.isEmpty() ? QDateTime::currentDateTime() : QDateTime::fromString(createdAtStr, Qt::ISODate);

        return std::make_unique<CategorieItem>(id, uuid, name, userUuid, createdAt, synced);

    } catch (const std::exception &e) {
        qWarning() << "从TOML创建类别项目时发生异常:" << e.what();
        return nullptr;
    }
}

/**
 * @brief 从TOML更新类别项目
 * @param item 类别项目指针
 * @param categoryTable TOML表
 */
void CategoryDataStorage::updateCategoryItemFromToml(CategorieItem *item, const toml::table &categoryTable) {
    if (!item)
        return;

    try {
        auto nameNode = categoryTable["name"];
        if (nameNode) {
            QString name = QString::fromStdString(nameNode.value_or(""));
            if (isValidCategoryName(name)) {
                item->setName(name);
            }
        }

        auto syncedNode = categoryTable["synced"];
        if (syncedNode) {
            item->setSynced(syncedNode.value_or(false));
        }

    } catch (const std::exception &e) {
        qWarning() << "更新类别项目时发生异常:" << e.what();
    }
}

/**
 * @brief 验证类别名称
 * @param name 类别名称
 * @return 有效返回true，否则返回false
 */
bool CategoryDataStorage::isValidCategoryName(const QString &name) const {
    QString trimmedName = name.trimmed();
    return !trimmedName.isEmpty() && trimmedName.length() <= 50;
}

/**
 * @brief 获取下一个可用ID
 * @param categories 类别列表
 * @return 下一个可用ID
 */
int CategoryDataStorage::getNextAvailableId(const std::vector<std::unique_ptr<CategorieItem>> &categories) const {
    int maxId = 0;
    for (const auto &category : categories) {
        if (category && category->id() > maxId) {
            maxId = category->id();
        }
    }
    return maxId + 1;
}

/**
 * @brief 添加类别
 * @param categories 类别列表引用
 * @param name 类别名称
 * @param userUuid 用户UUID
 * @return 添加成功返回true，否则返回false
 */
bool CategoryDataStorage::addCategory(std::vector<std::unique_ptr<CategorieItem>> &categories,
                                      const QString &name,
                                      const QUuid &userUuid) {
    bool success = true;

    try {
        QSqlDatabase db = m_databaseManager.getDatabase();
        if (!db.isOpen()) {
            qCritical() << "数据库未打开，无法添加类别";
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
        
        auto newItem = std::make_unique<CategorieItem>(
            newId,                    // 新的唯一标识符
            newUuid,                  // 新的UUID
            name,                     // 类别名称
            userUuid,                 // 用户UUID
            createdAt,                // 当前时间
            false,                    // 未同步
            this);

        // 插入到数据库
        QSqlQuery insertQuery(db);
        const QString insertString = "INSERT INTO categories (id, uuid, name, user_uuid, created_at, synced) VALUES (?, ?, ?, ?, ?, ?)";
        insertQuery.prepare(insertString);
        insertQuery.addBindValue(newId);
        insertQuery.addBindValue(newUuid.toString());
        insertQuery.addBindValue(name);
        insertQuery.addBindValue(userUuid.toString());
        insertQuery.addBindValue(createdAt.toString(Qt::ISODate));
        insertQuery.addBindValue(false);

        if (!insertQuery.exec()) {
            qCritical() << "插入类别到数据库失败:" << insertQuery.lastError().text();
            return false;
        }

        // 添加到内存列表
        categories.push_back(std::move(newItem));

        qDebug() << "成功添加类别:" << name;
    } catch (const std::exception &e) {
        qCritical() << "添加类别时发生异常:" << e.what();
        success = false;
    } catch (...) {
        qCritical() << "添加类别时发生未知异常";
        success = false;
    }

    return success;
}

/**
 * @brief 更新类别
 * @param categories 类别列表引用
 * @param id 类别ID
 * @param name 新的类别名称
 * @return 更新成功返回true，否则返回false
 */
bool CategoryDataStorage::updateCategory(std::vector<std::unique_ptr<CategorieItem>> &categories,
                                         int id,
                                         const QString &name) {
    bool success = false;

    try {
        QSqlDatabase db = m_databaseManager.getDatabase();
        if (!db.isOpen()) {
            qCritical() << "数据库未打开，无法更新类别";
            return false;
        }

        // 更新数据库中的类别
        QSqlQuery updateQuery(db);
        const QString updateString = "UPDATE categories SET name = ?, synced = ? WHERE id = ?";
        updateQuery.prepare(updateString);
        updateQuery.addBindValue(name);
        updateQuery.addBindValue(false); // 标记为未同步
        updateQuery.addBindValue(id);

        if (!updateQuery.exec()) {
            qCritical() << "更新数据库中的类别失败:" << updateQuery.lastError().text();
            return false;
        }

        if (updateQuery.numRowsAffected() == 0) {
            qWarning() << "未找到要更新的类别，ID:" << id;
            return false;
        }

        // 更新内存中的类别
        for (auto &item : categories) {
            if (item->id() == id) {
                item->setName(name);
                item->setSynced(false); // 标记为未同步
                success = true;
                break;
            }
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
 * @brief 处理冲突
 * @param newCategory 新类别
 * @param categories 类别列表
 * @param resolution 冲突解决策略
 * @return 处理成功返回true，否则返回false
 */
bool CategoryDataStorage::handleConflict(const std::unique_ptr<CategorieItem> &newCategory,
                                         std::vector<std::unique_ptr<CategorieItem>> &categories,
                                         ConflictResolution resolution) {
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

    switch (resolution) {
    case Skip:
        return false; // 跳过冲突项目

    case Overwrite:
        // 覆盖现有项目
        *it = std::move(const_cast<std::unique_ptr<CategorieItem> &>(newCategory));
        return true;

    case Merge:
        // 合并（保留较新的版本）
        if (newCategory->createdAt() > (*it)->createdAt()) {
            *it = std::move(const_cast<std::unique_ptr<CategorieItem> &>(newCategory));
        }
        return true;

    default:
        return false;
    }
}
