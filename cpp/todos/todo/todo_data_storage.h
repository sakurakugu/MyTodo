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

#include "../../foundation/database.h"
#include "setting.h"
#include "todo_item.h"

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
    using TodoList = std::vector<std::unique_ptr<TodoItem>>;

    // 定义导入冲突的解决策略
    enum ConflictResolution {
        Skip = 0,      // 跳过冲突项目
        Overwrite = 1, // 覆盖现有项目
        Merge = 2,     // 合并（保留较新的版本）
        Insert = 3     // 插入新项目
    };
    Q_ENUM(ConflictResolution)

    // 构造函数
    explicit TodoDataStorage(QObject *parent = nullptr);
    ~TodoDataStorage();

    // 本地存储功能
    bool 加载待办事项(TodoList &todos);             // 加载类别待办事项
    bool saveToLocalStorage(const TodoList &todos); // 将待办事项保存到本地存储

    // CRUD操作
    std::unique_ptr<TodoItem> 新增待办(TodoList &todos, const QString &title, const QString &description,
                                       const QString &category, bool important, const QDateTime &deadline,
                                       int recurrenceInterval, int recurrenceCount, const QDate &recurrenceStartDate,
                                       QUuid userUuid);
    bool 新增待办(TodoList &todos, std::unique_ptr<TodoItem> item);
    bool 更新待办(TodoList &todos, int id, const QString &title, const QString &description, const QString &category,
                  bool important, const QDateTime &deadline, int recurrenceInterval, int recurrenceCount,
                  const QDate &recurrenceStartDate);
    bool 更新待办(TodoList &todos, TodoItem &item);
    bool 回收待办(TodoList &todos, int id);
    bool 删除待办(TodoList &todos, int id);
    bool 软删除待办(TodoList &todos, int id);

  private:
    // 成员变量
    Setting &m_setting;   ///< 设置对象引用
    Database &m_database; ///< 数据库管理器引用
};