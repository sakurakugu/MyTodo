/**
 * @file category_data_storage.cpp
 * @brief CategoryDataStorage类的实现文件
 *
 * 该文件实现了CategoryDataStorage类，专门负责类别的本地存储和文件导入导出功能。
 * 从CategoryManager类中拆分出来，实现单一职责原则。
 *
 * @author Sakurakugu
 * @date 2025-01-27 00:00:00(UTC+8) 周一
 * @change 2025-01-27 00:00:00(UTC+8) 周一
 * @version 0.4.0
 */

#include "category_data_storage.h"
#include "default_value.h"
#include <QDebug>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <algorithm>

CategoryDataStorage::CategoryDataStorage(Setting &setting, QObject *parent)
    : QObject(parent), m_setting(setting) {
    qDebug() << "CategoryDataStorage 初始化完成";
}

CategoryDataStorage::~CategoryDataStorage() {
    qDebug() << "CategoryDataStorage 析构";
}

/**
 * @brief 从本地存储加载类别
 * @param categories 类别列表引用
 * @return 加载成功返回true，否则返回false
 */
bool CategoryDataStorage::loadFromLocalStorage(std::vector<std::unique_ptr<CategorieItem>> &categories) {
    qDebug() << "开始从本地存储加载类别数据";
    
    try {
        // 获取配置文件路径
        QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QDir configDir(configPath);
        if (!configDir.exists()) {
            configDir.mkpath(".");
        }
        
        QString categoryFilePath = configDir.filePath("categories.toml");
        QFile file(categoryFilePath);
        
        if (!file.exists()) {
            qDebug() << "类别配置文件不存在，将创建默认类别";
            emit dataOperationCompleted(true, "首次运行，创建默认类别");
            return true; // 首次运行，不算错误
        }
        
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "无法打开类别配置文件:" << categoryFilePath;
            emit dataOperationCompleted(false, "无法打开类别配置文件");
            return false;
        }
        
        QString content = file.readAll();
        file.close();
        
        if (content.isEmpty()) {
            qDebug() << "类别配置文件为空";
            emit dataOperationCompleted(true, "配置文件为空，使用默认设置");
            return true;
        }
        
        // 解析TOML内容
        toml::table config;
        try {
            config = toml::parse(content.toStdString());
        } catch (const toml::parse_error &err) {
            qWarning() << "解析类别配置文件失败:" << err.what();
            emit dataOperationCompleted(false, "配置文件格式错误");
            return false;
        }
        
        // 导入类别数据
        bool result = importFromToml(config, categories);
        if (result) {
            qDebug() << "从本地存储加载类别成功，共" << categories.size() << "个类别";
            emit dataOperationCompleted(true, QString("成功加载 %1 个类别").arg(categories.size()));
        }
        
        return result;
        
    } catch (const std::exception &e) {
        qWarning() << "加载类别数据时发生异常:" << e.what();
        emit dataOperationCompleted(false, "加载类别数据时发生异常");
        return false;
    }
}

/**
 * @brief 将类别保存到本地存储
 * @param categories 类别列表
 * @return 保存成功返回true，否则返回false
 */
bool CategoryDataStorage::saveToLocalStorage(const std::vector<std::unique_ptr<CategorieItem>> &categories) {
    qDebug() << "开始保存类别数据到本地存储";
    
    try {
        // 获取配置文件路径
        QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QDir configDir(configPath);
        if (!configDir.exists()) {
            configDir.mkpath(".");
        }
        
        QString categoryFilePath = configDir.filePath("categories.toml");
        
        // 创建TOML表
        toml::table config;
        if (!exportToToml(categories, config)) {
            emit dataOperationCompleted(false, "导出类别数据失败");
            return false;
        }
        
        // 写入文件
        QFile file(categoryFilePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "无法创建类别配置文件:" << categoryFilePath;
            emit dataOperationCompleted(false, "无法创建配置文件");
            return false;
        }
        
        std::ostringstream oss;
        oss << config;
        QString content = QString::fromStdString(oss.str());
        
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);
        stream << content;
        file.close();
        
        qDebug() << "类别数据保存成功，共" << categories.size() << "个类别";
        emit dataOperationCompleted(true, QString("成功保存 %1 个类别").arg(categories.size()));
        return true;
        
    } catch (const std::exception &e) {
        qWarning() << "保存类别数据时发生异常:" << e.what();
        emit dataOperationCompleted(false, "保存类别数据时发生异常");
        return false;
    }
}

/**
 * @brief 从TOML文件导入类别
 * @param filePath 文件路径
 * @param categories 类别列表
 * @return 是否成功
 */
bool CategoryDataStorage::importFromToml(const QString &filePath, std::vector<std::unique_ptr<CategorieItem>> &categories) {
    try {
        auto table = toml::parse_file(filePath.toStdString());
        return importFromToml(table, categories, ConflictResolution::Skip);
    } catch (const toml::parse_error &err) {
        qWarning() << "TOML解析错误:" << err.what();
        emit dataOperationCompleted(false, QString("TOML解析错误: %1").arg(err.what()));
        return false;
    } catch (const std::exception &e) {
        qWarning() << "导入TOML文件时发生异常:" << e.what();
        emit dataOperationCompleted(false, QString("导入失败: %1").arg(e.what()));
        return false;
    }
}

/**
 * @brief 从TOML表导入类别
 * @param table TOML表
 * @param categories 类别列表引用
 * @return 导入成功返回true，否则返回false
 */
bool CategoryDataStorage::importFromToml(const toml::table &table, std::vector<std::unique_ptr<CategorieItem>> &categories) {
    return importFromToml(table, categories, ConflictResolution::Skip);
}

/**
 * @brief 从TOML表导入类别（指定冲突解决策略）
 * @param table TOML表
 * @param categories 类别列表引用
 * @param resolution 冲突解决策略
 * @return 导入成功返回true，否则返回false
 */
bool CategoryDataStorage::importFromToml(const toml::table &table, std::vector<std::unique_ptr<CategorieItem>> &categories, ConflictResolution resolution) {
    qDebug() << "开始从TOML导入类别数据";
    
    int importedCount = 0;
    int skippedCount = 0;
    int conflictCount = 0;
    
    try {
        // 查找categories数组
        auto categoriesArray = table["categories"].as_array();
        if (!categoriesArray) {
            qDebug() << "TOML中未找到categories数组";
            emit importCompleted(0, 0, 0);
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
        emit importCompleted(importedCount, skippedCount, conflictCount);
        return true;
        
    } catch (const std::exception &e) {
        qWarning() << "从TOML导入类别时发生异常:" << e.what();
        emit importCompleted(importedCount, skippedCount, conflictCount);
        return false;
    }
}

/**
 * @brief 从JSON文件导入类别
 * @param filePath 文件路径
 * @param categories 类别列表
 * @return 是否成功
 */
bool CategoryDataStorage::importFromJson(const QString &filePath, std::vector<std::unique_ptr<CategorieItem>> &categories) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开JSON文件:" << filePath;
        emit dataOperationCompleted(false, "无法打开文件");
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

    if (!doc.isArray()) {
        qWarning() << "JSON文件格式错误，期望数组格式";
        emit dataOperationCompleted(false, "JSON文件格式错误");
        return false;
    }

    QJsonArray jsonArray = doc.array();
    int importedCount = 0;
    int skippedCount = 0;

    for (const auto &value : jsonArray) {
        if (!value.isObject()) {
            skippedCount++;
            continue;
        }

        QJsonObject categoryObj = value.toObject();
        if (!validateCategoryData(categoryObj)) {
            skippedCount++;
            continue;
        }

        auto category = createCategoryFromJson(categoryObj);
        if (category) {
            // 检查是否已存在同名类别
            bool exists = false;
            for (const auto &existing : categories) {
                if (existing && existing->name() == category->name()) {
                    exists = true;
                    break;
                }
            }

            if (!exists) {
                categories.push_back(std::move(category));
                importedCount++;
            } else {
                skippedCount++;
            }
        } else {
            skippedCount++;
        }
    }

    emit importCompleted(importedCount, skippedCount, 0);
    emit dataOperationCompleted(true, QString("导入完成，成功: %1，跳过: %2").arg(importedCount).arg(skippedCount));
    
    qDebug() << "从JSON文件导入类别完成，成功:" << importedCount << "跳过:" << skippedCount;
    return importedCount > 0;
}

/**
 * @brief 导出类别到TOML文件
 * @param filePath 文件路径
 * @param categories 类别列表
 * @return 是否成功
 */
bool CategoryDataStorage::exportToToml(const QString &filePath, const std::vector<std::unique_ptr<CategorieItem>> &categories) {
    toml::table table;
    if (!exportToToml(categories, table)) {
        return false;
    }

    try {
        std::ofstream file(filePath.toStdString());
        if (!file.is_open()) {
            qWarning() << "无法创建TOML文件:" << filePath;
            emit exportCompleted(false, "无法创建文件");
            return false;
        }

        file << table;
        file.close();
        
        emit exportCompleted(true, "导出成功");
        qDebug() << "类别导出到TOML文件成功:" << filePath;
        return true;
    } catch (const std::exception &e) {
        qWarning() << "导出TOML文件时发生异常:" << e.what();
        emit exportCompleted(false, QString("导出失败: %1").arg(e.what()));
        return false;
    }
}

/**
 * @brief 导出类别到TOML表
 * @param categories 类别列表
 * @param table TOML表
 * @return 是否成功
 */
bool CategoryDataStorage::exportToToml(const std::vector<std::unique_ptr<CategorieItem>> &categories, toml::table &table) {
    try {
        toml::array categoriesArray;
        
        for (const auto &category : categories) {
            if (!category) continue;
            
            toml::table categoryTable;
            categoryTable.insert("id", category->id());
            categoryTable.insert("uuid", category->uuid().toString().toStdString());
            categoryTable.insert("name", category->name().toStdString());
            categoryTable.insert("user_uuid", category->userUuid().toString().toStdString());
            categoryTable.insert("created_at", category->createdAt().toString(Qt::ISODate).toStdString());
            categoryTable.insert("synced", category->synced());
            
            categoriesArray.push_back(categoryTable);
        }
        
        table.insert("categories", categoriesArray);
        return true;
        
    } catch (const std::exception &e) {
        qWarning() << "导出类别到TOML时发生异常:" << e.what();
        return false;
    }
}

/**
 * @brief 导出类别到JSON文件
 * @param filePath 文件路径
 * @param categories 类别列表
 * @return 是否成功
 */
bool CategoryDataStorage::exportToJson(const QString &filePath, const std::vector<std::unique_ptr<CategorieItem>> &categories) {
    QJsonArray jsonArray;
    if (!exportToJson(categories, jsonArray)) {
        return false;
    }

    QJsonDocument doc(jsonArray);
    QFile file(filePath);
    
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "无法创建JSON文件:" << filePath;
        emit exportCompleted(false, "无法创建文件");
        return false;
    }

    file.write(doc.toJson());
    file.close();
    
    emit exportCompleted(true, "导出成功");
    qDebug() << "类别导出到JSON文件成功:" << filePath;
    return true;
}

/**
 * @brief 导出类别到JSON数组
 * @param categories 类别列表
 * @param jsonArray JSON数组引用
 * @return 导出成功返回true，否则返回false
 */
bool CategoryDataStorage::exportToJson(const std::vector<std::unique_ptr<CategorieItem>> &categories, QJsonArray &jsonArray) {
    try {
        for (const auto &category : categories) {
            if (!category) continue;
            
            QJsonObject categoryObj = categoryToJson(*category);
            jsonArray.append(categoryObj);
        }
        
        return true;
        
    } catch (const std::exception &e) {
        qWarning() << "导出类别到JSON时发生异常:" << e.what();
        return false;
    }
}

/**
 * @brief 创建默认类别
 * @param categories 类别列表引用
 * @param userUuid 用户UUID
 */
void CategoryDataStorage::createDefaultCategories(std::vector<std::unique_ptr<CategorieItem>> &categories, const QUuid &userUuid) {
    qDebug() << "创建默认类别";
    
    // 清空现有类别
    categories.clear();
    
    // 添加默认的"未分类"选项
    auto defaultCategory = std::make_unique<CategorieItem>(
        1,
        QUuid::createUuid(),
        "未分类",
        userUuid,
        QDateTime::currentDateTime(),
        false
    );
    
    categories.push_back(std::move(defaultCategory));
    qDebug() << "默认类别创建完成";
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

/**
 * @brief 从JSON创建类别项目
 * @param categoryObj JSON对象
 * @return 类别项目智能指针
 */
std::unique_ptr<CategorieItem> CategoryDataStorage::createCategoryFromJson(const QJsonObject &categoryObj) const {
    if (!validateCategoryData(categoryObj)) {
        return nullptr;
    }
    
    int id = categoryObj["id"].toInt(0);
    QString uuidStr = categoryObj["uuid"].toString();
    QString name = categoryObj["name"].toString().trimmed();
    QString userUuidStr = categoryObj["user_uuid"].toString();
    QString createdAtStr = categoryObj["created_at"].toString();
    bool synced = categoryObj["synced"].toBool(false);
    
    QUuid uuid = uuidStr.isEmpty() ? QUuid::createUuid() : QUuid::fromString(uuidStr);
    QUuid userUuid = QUuid::fromString(userUuidStr);
    QDateTime createdAt = createdAtStr.isEmpty() ? QDateTime::currentDateTime() : QDateTime::fromString(createdAtStr, Qt::ISODate);
    
    return std::make_unique<CategorieItem>(id, uuid, name, userUuid, createdAt, synced);
}

/**
 * @brief 将类别转换为JSON
 * @param category 类别项目
 * @return JSON对象
 */
QJsonObject CategoryDataStorage::categoryToJson(const CategorieItem &category) const {
    QJsonObject obj;
    obj["id"] = category.id();
    obj["uuid"] = category.uuid().toString();
    obj["name"] = category.name();
    obj["user_uuid"] = category.userUuid().toString();
    obj["created_at"] = category.createdAt().toString(Qt::ISODate);
    obj["synced"] = category.synced();
    return obj;
}

// 私有辅助方法实现

/**
 * @brief 从TOML表创建类别项目
 * @param categoryTable TOML表
 * @param newId 新ID
 * @return 类别项目智能指针
 */
std::unique_ptr<CategorieItem> CategoryDataStorage::createCategoryItemFromToml(const toml::table &categoryTable, int newId) {
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
        QDateTime createdAt = createdAtStr.isEmpty() ? QDateTime::currentDateTime() : QDateTime::fromString(createdAtStr, Qt::ISODate);
        
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
    if (!item) return;
    
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
    if (!newCategory) return false;
    
    // 查找冲突的类别
    auto it = std::find_if(categories.begin(), categories.end(),
                           [&newCategory](const std::unique_ptr<CategorieItem> &existing) {
                               return existing && (existing->name() == newCategory->name() || 
                                                   existing->uuid() == newCategory->uuid());
                           });
    
    if (it == categories.end()) {
        // 没有冲突，直接添加
        categories.push_back(std::move(const_cast<std::unique_ptr<CategorieItem>&>(newCategory)));
        return true;
    }
    
    switch (resolution) {
    case Skip:
        return false; // 跳过冲突项目
        
    case Overwrite:
        // 覆盖现有项目
        *it = std::move(const_cast<std::unique_ptr<CategorieItem>&>(newCategory));
        return true;
        
    case Merge:
        // 合并（保留较新的版本）
        if (newCategory->createdAt() > (*it)->createdAt()) {
            *it = std::move(const_cast<std::unique_ptr<CategorieItem>&>(newCategory));
        }
        return true;
        
    default:
        return false;
    }
}