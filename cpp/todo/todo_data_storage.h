/**
 * @file todo_data_storage.h
 * @brief TodoDataStorage类的头文件
 *
 * 该文件定义了TodoDataStorage类，专门负责待办事项的本地存储和文件导入导出功能。
 * 从TodoManager类中拆分出来，实现单一职责原则。
 *
 * @author Sakurakugu
 * @date 2025-08-25 00:54:11(UTC+8) 周一
 * @change 2025-08-25 18:38:47(UTC+8) 周一
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

#include "items/todo_item.h"
#include "setting.h"

/**
 * @class TodoDataStorage
 * @brief 待办事项数据管理器，负责本地存储和文件导入导出功能
 *
 * TodoDataStorage类专门处理待办事项的数据持久化和文件操作：
 *
 * **核心功能：**
 * - 本地配置存储和加载
 * - 文件导入导出（JSON格式）
 * - 导入冲突检测和解决
 * - 数据格式验证和转换
 *
 * **设计原则：**
 * - 单一职责：专注于数据存储和文件操作
 * - 低耦合：与UI层和业务逻辑层分离
 * - 高内聚：相关的数据操作集中管理
 *
 * **使用场景：**
 * - TodoManager需要加载/保存本地数据时
 * - 用户需要导入/导出待办事项文件时
 * - 需要处理数据冲突和合并时
 *
 * @note 该类是线程安全的，所有文件操作都有异常处理
 * @see TodoItem, Setting
 */
class TodoDataStorage : public QObject {
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

    // 构造函数
    explicit TodoDataStorage(Setting &setting, QObject *parent = nullptr);
    ~TodoDataStorage();

    // 本地存储功能
    bool loadFromLocalStorage(std::vector<std::unique_ptr<TodoItem>> &todos);     // 从本地存储加载待办事项
    bool saveToLocalStorage(const std::vector<std::unique_ptr<TodoItem>> &todos); // 将待办事项保存到本地存储

    // 文件导入功能
    bool importFromToml(const toml::table &table,
                        std::vector<std::unique_ptr<TodoItem>> &todos); // 从TOML表导入待办事项
    bool importFromToml(const toml::table &table,
                        std::vector<std::unique_ptr<TodoItem>> &todos,
                        ConflictResolution resolution); // 从TOML表导入待办事项（指定冲突解决策略）

  signals:
    void dataOperationCompleted(bool success, const QString &message);            // 数据操作完成信号
    void importCompleted(int importedCount, int skippedCount, int conflictCount); // 导入操作完成信号

  private:
    // 辅助方法
    std::unique_ptr<TodoItem> createTodoItemFromToml(const toml::table &todoTable, int newId);
    void updateTodoItemFromToml(TodoItem *item, const toml::table &todoTable);
    
    // 成员变量
    Setting &m_setting; ///< 设置对象引用
};