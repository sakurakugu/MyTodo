/**
 * @file category_data_storage.h
 * @brief CategoryDataStorage类的头文件
 *
 * 该文件定义了CategoryDataStorage类，专门负责类别的本地存储和文件导入导出功能。
 * 从CategoryManager类中拆分出来，实现单一职责原则。
 *
 * @author Sakurakugu
 * @date 2025-01-27 00:00:00(UTC+8) 周一
 * @change 2025-01-27 00:00:00(UTC+8) 周一
 * @version 0.4.0
 */

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <memory>
#include <toml++/toml.hpp>
#include <vector>

#include "../../foundation/config.h"
#include "../items/categorie_item.h"
#include "setting.h"

/**
 * @class CategoryDataStorage
 * @brief 类别数据管理器，负责本地存储和文件导入导出功能
 *
 * CategoryDataStorage类专门处理类别的数据持久化和文件操作：
 *
 * **核心功能：**
 * - 本地配置存储和加载
 * - 文件导入导出（JSON格式）
 * - 导入冲突检测和解决
 * - 数据格式验证和转换
 * - 默认类别管理
 *
 * **设计原则：**
 * - 单一职责：专注于数据存储和文件操作
 * - 低耦合：与UI层和业务逻辑层分离
 * - 高内聚：相关的数据操作集中管理
 *
 * **使用场景：**
 * - CategoryManager需要加载/保存本地数据时
 * - 用户需要导入/导出类别文件时
 * - 需要处理数据冲突和合并时
 * - 初始化默认类别时
 *
 * @note 该类是线程安全的，所有文件操作都有异常处理
 * @see CategorieItem, Setting
 */
class CategoryDataStorage : public QObject {
    Q_OBJECT

  public:
    /**
     * @enum ConflictResolution
     * @brief 定义导入冲突的解决策略
     */
    enum ConflictResolution {
        Skip = 0,      // 跳过冲突项目
        Overwrite = 1, // 覆盖现有项目
        Merge = 2      // 合并（保留较新的版本）
    };
    Q_ENUM(ConflictResolution)

    /**
     * @enum FileFormat
     * @brief 定义文件格式类型
     */
    enum FileFormat {
        TOML = 0, // TOML格式
        JSON = 1  // JSON格式
    };
    Q_ENUM(FileFormat)

    // 构造函数
    explicit CategoryDataStorage(QObject *parent = nullptr);
    ~CategoryDataStorage();

    // 本地存储功能
    bool loadFromLocalStorage(std::vector<std::unique_ptr<CategorieItem>> &categories);     // 从本地存储加载类别
    bool saveToLocalStorage(const std::vector<std::unique_ptr<CategorieItem>> &categories); // 将类别保存到本地存储

    // 文件导入功能
    bool importFromToml(const QString &filePath,
                        std::vector<std::unique_ptr<CategorieItem>> &categories); // 从TOML文件导入类别
    bool importFromToml(const toml::table &table,
                        std::vector<std::unique_ptr<CategorieItem>> &categories); // 从TOML表导入类别
    bool importFromToml(const toml::table &table, std::vector<std::unique_ptr<CategorieItem>> &categories,
                        ConflictResolution resolution); // 从TOML表导入类别（指定冲突解决策略）
    bool importFromJson(const QString &filePath,
                        std::vector<std::unique_ptr<CategorieItem>> &categories); // 从JSON文件导入类别

    // 文件导出功能
    bool exportToToml(const QString &filePath,
                      const std::vector<std::unique_ptr<CategorieItem>> &categories); // 导出类别到TOML文件
    bool exportToToml(const std::vector<std::unique_ptr<CategorieItem>> &categories,
                      toml::table &table); // 导出类别到TOML表
    bool exportToJson(const QString &filePath,
                      const std::vector<std::unique_ptr<CategorieItem>> &categories); // 导出类别到JSON文件
    bool exportToJson(const std::vector<std::unique_ptr<CategorieItem>> &categories,
                      QJsonArray &jsonArray); // 导出类别到JSON数组

    // 默认类别管理
    void createDefaultCategories(std::vector<std::unique_ptr<CategorieItem>> &categories,
                                 const QUuid &userUuid);                                          // 创建默认类别
    bool hasDefaultCategory(const std::vector<std::unique_ptr<CategorieItem>> &categories) const; // 检查是否有默认类别

    // 数据验证和转换
    bool validateCategoryData(const QJsonObject &categoryObj) const;                             // 验证类别数据
    std::unique_ptr<CategorieItem> createCategoryFromJson(const QJsonObject &categoryObj) const; // 从JSON创建类别项目
    QJsonObject categoryToJson(const CategorieItem &category) const;                             // 将类别转换为JSON

  signals:
    void dataOperationCompleted(bool success, const QString &message);            // 数据操作完成信号
    void importCompleted(int importedCount, int skippedCount, int conflictCount); // 导入操作完成信号
    void exportCompleted(bool success, const QString &message);                   // 导出操作完成信号

  private:
    // 辅助方法
    std::unique_ptr<CategorieItem> createCategoryItemFromToml(const toml::table &categoryTable, int newId);
    void updateCategoryItemFromToml(CategorieItem *item, const toml::table &categoryTable);
    bool isValidCategoryName(const QString &name) const;
    int getNextAvailableId(const std::vector<std::unique_ptr<CategorieItem>> &categories) const;

    // 冲突处理
    bool handleConflict(const std::unique_ptr<CategorieItem> &newCategory,
                        std::vector<std::unique_ptr<CategorieItem>> &categories, ConflictResolution resolution);

    // 配置相关方法
    bool loadFromConfig(std::vector<std::unique_ptr<CategorieItem>> &categories);
    bool saveToConfig(const std::vector<std::unique_ptr<CategorieItem>> &categories);
    QString getCategoryConfigKey() const;

    // 成员变量
    Setting &m_setting; ///< 设置对象引用
    Config &m_config;   ///< 主配置对象引用
};