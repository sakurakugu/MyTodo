/**
 * @file base_data_storage.h
 * @brief BaseDataStorage基类的头文件
 *
 * 该文件定义了BaseDataStorage基类，为CategoryDataStorage和TodoDataStorage
 * 提供通用的数据存储功能和接口。实现了单一职责原则和代码复用。
 *
 * @author Sakurakugu
 * @date 2025-10-05 01:07:45(UTC+8) 周日
 * @change 2025-10-06 01:32:19(UTC+8) 周一
 */

#pragma once

#include "database.h"
#include <QObject>
#include <nlohmann/json.hpp>
#include <uuid.h>

// Todo和Category的Item类型概念
template <typename T>
concept ItemType = requires(T t) {
    t.uuid();
    t.updatedAt();
};

/**
 * @class BaseDataStorage
 * @brief 数据存储基类，提供通用的数据存储功能
 *
 * BaseDataStorage类为各种数据存储类提供通用功能：
 *
 * **核心功能：**
 * - 数据库连接管理
 * - 导入导出器注册/注销
 * - 通用的数据库操作方法
 * - 冲突解决策略定义
 * - 导入来源定义
 *
 * **设计原则：**
 * - 单一职责：专注于通用数据存储功能
 * - 开闭原则：对扩展开放，对修改封闭
 * - 里氏替换：子类可以替换基类使用
 *
 * **使用场景：**
 * - 作为CategoryDataStorage和TodoDataStorage的基类
 * - 提供统一的数据存储接口
 * - 减少代码重复，提高维护性
 *
 * @note 该类是抽象基类，不能直接实例化
 * @see CategoryDataStorage, TodoDataStorage, IDataExporter
 */
class BaseDataStorage : public QObject, public IDataExporter {
    Q_OBJECT

  public:
    // 定义导入冲突的解决策略
    enum 解决冲突方案 {
        Skip = 0,      // 跳过冲突项目
        Overwrite = 1, // 覆盖现有项目
        Merge = 2,     // 合并（保留较新的版本）
        Insert = 3     // 插入新项目
    };

    // 定义导入来源
    enum ImportSource {
        Server = 0, // 服务器
        Local = 1   // 本地/本地备份
    };

    // 构造函数和析构函数
    explicit BaseDataStorage(const std::string &exporterName, QObject *parent = nullptr);
    ~BaseDataStorage() override;

    // 公共初始化方法 - 子类构造完成后调用
    bool 初始化();

    // 纯虚函数 - 子类必须实现
    virtual bool 初始化数据表() = 0;

    // IDataExporter接口实现 - 子类必须重写
    bool exportToJson(nlohmann::json &output) override = 0;
    bool importFromJson(const nlohmann::json &input, bool replaceAll) override = 0;

  protected:
    // 通用的表创建方法
    virtual bool 创建数据表() = 0;

    // 评估冲突
    template <ItemType T>
    解决冲突方案 评估冲突(const T *existing, const T &incoming,
                          解决冲突方案 resolution) const; // 返回应执行的动作

    // 成员变量
    Database &m_database;       ///< 数据库管理器引用
    std::string m_exporterName; ///< 导出器名称

  private:
    // 禁用拷贝构造和赋值操作
    BaseDataStorage(const BaseDataStorage &) = delete;
    BaseDataStorage &operator=(const BaseDataStorage &) = delete;
};

/**
 * @brief 评估冲突并决定动作
 * @param existing 现有类别指针（若无则为nullptr）
 * @param incoming 新导入的类别引用
 * @param resolution 冲突解决策略
 * @return 决定的冲突动作
 */
template <ItemType T>
BaseDataStorage::解决冲突方案 BaseDataStorage::评估冲突( //
    const T *existing,                                   //
    const T &incoming,                                   //
    解决冲突方案 resolution) const                       //

{
    // 无冲突，直接插入
    if (!existing)
        return 解决冲突方案::Insert;

    switch (resolution) {
    case Skip:
        return 解决冲突方案::Skip; // 直接跳过
    case Overwrite:
        return 解决冲突方案::Overwrite; // 强制覆盖
    case Merge: {
        // Merge: 选择更新时间新的那条；若相等则保留旧
        if (incoming.updatedAt() > existing->updatedAt()) {
            return 解决冲突方案::Overwrite;
        } else {
            return 解决冲突方案::Skip;
        }
    }
    case Insert:
        return 解决冲突方案::Insert;
    default:
        return 解决冲突方案::Skip;
    }
}