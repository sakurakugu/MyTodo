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

#include "../../foundation/database.h"
#include "categorie_item.h"
#include <toml++/toml.hpp>

class Config;

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
    using CategorieList = std::vector<std::unique_ptr<CategorieItem>>;

    // 定义导入冲突的解决策略
    enum ConflictResolution {
        Skip = 0,      // 跳过冲突项目
        Overwrite = 1, // 覆盖现有项目
        Merge = 2,     // 合并（保留较新的版本）
        Insert = 3     // 插入新项目
    };
    Q_ENUM(ConflictResolution)

    // 定义导入来源
    enum ImportSource {
        Server = 0, // 服务器
        Local = 1   // 本地/本地备份
    };
    Q_ENUM(ImportSource)

    // 构造函数
    explicit CategoryDataStorage(QObject *parent = nullptr);
    ~CategoryDataStorage();

    // 本地存储功能
    bool 加载类别(CategorieList &categorieList);

    // CRUD操作
    std::unique_ptr<CategorieItem> 新增类别(CategorieList &categories, const QString &name, const QUuid &userUuid,
                                            ImportSource source = Local);
    bool 更新类别(CategorieList &categories, const QString &name, const QString &newName);
    bool 删除类别(CategorieList &categories, const QString &name);
    bool 软删除类别(CategorieList &categories, const QString &name);
    bool 更新同步状态(CategorieList &categories, const QString &name, int synced = 0);

    bool 创建默认类别(CategorieList &categories, const QUuid &userUuid);
    bool 导入类别从JSON(CategorieList &categories, const QJsonArray &categoriesArray,
                        ImportSource source = ImportSource::Server,
                        ConflictResolution resolution = ConflictResolution::Merge);

  private:
    // 辅助方法
    ConflictResolution 评估冲突(const CategorieItem *existing, const CategorieItem &incoming,
                                ConflictResolution resolution) const; // 返回应执行的动作
    int 获取最后插入行ID(QSqlDatabase &db) const;                     // 获取自增ID

    // 成员变量
    Database &m_database; ///< 数据库管理器引用
};