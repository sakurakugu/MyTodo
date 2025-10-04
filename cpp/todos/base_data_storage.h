/**
 * @file base_data_storage.h
 * @brief BaseDataStorage基类的头文件
 *
 * 该文件定义了BaseDataStorage基类，为CategoryDataStorage和TodoDataStorage
 * 提供通用的数据存储功能和接口。实现了单一职责原则和代码复用。
 *
 * @author Sakurakugu
 * @date 2025-10-05 01:07(UTC+8)
 */

#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QUuid>
#include "../foundation/database.h"

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

    // 构造函数和析构函数
    explicit BaseDataStorage(const QString &exporterName, QObject *parent = nullptr);
    ~BaseDataStorage() override;

    // 公共初始化方法 - 子类构造完成后调用
    bool 初始化();

    // 纯虚函数 - 子类必须实现
    virtual bool 初始化数据表() = 0;

    // IDataExporter接口实现 - 子类必须重写
    bool 导出到JSON(QJsonObject &output) override = 0;
    bool 导入从JSON(const QJsonObject &input, bool replaceAll) override = 0;

protected:
    // 通用的数据库操作方法
    int 获取最后插入行ID(QSqlDatabase &db) const;
    bool 执行SQL查询(const QString &queryString, QSqlQuery &query);
    bool 执行SQL查询(const QString &queryString);

    // 通用的表创建方法
    virtual bool 创建数据表() = 0;

    // 成员变量
    Database &m_database;        ///< 数据库管理器引用
    QString m_exporterName;      ///< 导出器名称

private:
    // 禁用拷贝构造和赋值操作
    BaseDataStorage(const BaseDataStorage &) = delete;
    BaseDataStorage &operator=(const BaseDataStorage &) = delete;
};