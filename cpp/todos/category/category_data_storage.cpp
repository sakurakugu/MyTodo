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
#include "category_data_storage.h"
#include "../../foundation/config.h"

CategoryDataStorage::CategoryDataStorage(QObject *parent)
    : QObject(parent),                // 父对象
      m_config(Config::GetInstance()) // 配置
{
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

        // 从设置中加载数据
        int count = m_config.get("categories/size", 0).toInt();

        // 获取当前最大的id，用于为没有id的项目分配新id
        int maxId = 0;
        for (int i = 0; i < count; ++i) {
            QString prefix = QString("categories/%1/").arg(i);
            int currentId = m_config.get(prefix + "id").toInt();
            if (currentId > maxId) {
                maxId = currentId;
            }
        }

        for (int i = 0; i < count; ++i) {
            QString prefix = QString("categories/%1/").arg(i);

            // 验证必要字段
            if (!m_config.contains(prefix + "uuid")) {
                QUuid uuid = QUuid::createUuid();
                m_config.save(prefix + "uuid", uuid.toString());
            }

            // 获取id，如果没有或为0，则分配一个新的唯一id
            int itemId = m_config.get(prefix + "id").toInt();
            if (itemId == 0) {
                itemId = ++maxId;
                // 保存新分配的id到设置中
                m_config.save(prefix + "id", itemId);
            }

            auto item = std::make_unique<CategorieItem>(
                itemId,                                                          // 唯一标识符
                QUuid::fromString(m_config.get(prefix + "uuid").toString()),     // 唯一标识符（UUID）
                m_config.get(prefix + "name").toString(),                        // 分类名称
                QUuid::fromString(m_config.get(prefix + "userUuid").toString()), // 用户UUID
                m_config.get(prefix + "createdAt").toDateTime(),                 // 创建时间
                m_config.get(prefix + "synced").toBool(),                        // 是否已同步
                this);

            categories.push_back(std::move(item));
        }

        qDebug() << "成功从本地存储加载" << categories.size() << "个分类";
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
 * @brief 将类别保存到本地存储
 * @param categories 类别列表
 * @return 保存成功返回true，否则返回false
 */
bool CategoryDataStorage::saveToLocalStorage(const std::vector<std::unique_ptr<CategorieItem>> &categories) {
    bool success = true;

    try {
        // 如果数量小于原大小，先删除所有现有的分类条目
        const size_t currentSize = m_config.get("categories/size", 0).toInt();
        if (categories.size() < currentSize) {
            // 获取当前存储的分类数量

            // 删除所有现有的分类条目
            for (size_t i = 0; i < currentSize; ++i) {
                QString prefix = QString("categories/%1").arg(i);

                // 删除该分类的所有属性
                m_config.remove(prefix + "/id");
                m_config.remove(prefix + "/uuid");
                m_config.remove(prefix + "/name");
                m_config.remove(prefix + "/userUuid");
                m_config.remove(prefix + "/createdAt");
                m_config.remove(prefix + "/synced");
                // 删除整个分类条目 (categories/x)
                m_config.remove(prefix);
            }
        }

        // 保存分类数量
        m_config.save("categories/size", static_cast<int>(categories.size()));

        // 保存每个分类
        for (size_t i = 0; i < categories.size(); ++i) {
            const CategorieItem *item = categories.at(i).get();
            QString prefix = QString("categories/%1/").arg(i);

            m_config.save(prefix + "id", item->id());
            m_config.save(prefix + "uuid", item->uuid());
            m_config.save(prefix + "name", item->name());
            m_config.save(prefix + "userUuid", item->userUuid());
            m_config.save(prefix + "createdAt", item->createdAt());
            m_config.save(prefix + "synced", item->synced());
        }

        qDebug() << "已成功保存" << categories.size() << "个分类到本地存储";
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
