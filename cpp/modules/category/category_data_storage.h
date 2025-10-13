/**
 * @file category_data_storage.h
 * @brief CategoryDataStorage类的头文件
 *
 * 该文件定义了CategoryDataStorage类，专门负责类别的本地存储和文件导入导出功能。
 * 从CategoryManager类中拆分出来，实现单一职责原则。
 *
 * @author Sakurakugu
 * @date 2025-09-11 00:04:40(UTC+8) 周四
 * @change 2025-10-06 02:13:23(UTC+8) 周一
 */

#pragma once

#include "base_data_storage.h"

class Config;
class CategorieItem;

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
using CategorieList = std::vector<std::unique_ptr<CategorieItem>>;

class CategoryDataStorage : public BaseDataStorage {
    Q_OBJECT

  public:
    // 构造函数
    explicit CategoryDataStorage(QObject *parent = nullptr);
    ~CategoryDataStorage() override;

    // 基类纯虚函数实现
    bool 初始化数据表() override; // 初始化Category表

    // 本地存储功能
    bool 加载类别(CategorieList &categorieList);

    // CRUD操作
    std::unique_ptr<CategorieItem> 新增类别(CategorieList &categories, const std::string &name, const uuids::uuid &userUuid,
                                            ImportSource source = Local);
    bool 更新类别(CategorieList &categories, const std::string &name, const std::string &newName);
    bool 删除类别(CategorieList &categories, const std::string &name);
    bool 软删除类别(CategorieList &categories, const std::string &name);
    bool 更新同步状态(CategorieList &categories, const std::string &name, int synced = 0);

    bool 创建默认类别(CategorieList &categories, const uuids::uuid &userUuid);
    bool 导入类别从JSON(CategorieList &categories, const QJsonArray &categoriesArray,
                        ImportSource source = ImportSource::Server,
                        解决冲突方案 resolution = 解决冲突方案::Merge);

    // IDataExporter接口实现
    bool exportToJson(QJsonObject &output) override;
    bool importFromJson(const QJsonObject &input, bool replaceAll) override;

  protected:
    // 基类虚函数实现
    bool 创建数据表() override; // 创建categories表

  private:
};