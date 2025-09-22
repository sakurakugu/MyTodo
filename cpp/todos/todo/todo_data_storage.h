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

    /**
     * @brief 查询参数结构（数据库端过滤 + 排序）
     * 允许按分类、状态、搜索文本、日期范围、排序等生成 SQL。
     */
    struct QueryOptions {
        QString category;     // 分类过滤，空=全部
        QString statusFilter; // "" | "todo" | "done" | "recycle" | "all"
        QString searchText;   // 模糊搜索（title/description/category）
        bool dateFilterEnabled{false};
        QDate dateStart;        // 起始日期（deadline）
        QDate dateEnd;          // 截止日期（deadline）
        int sortType{0};        // 对应 TodoSorter::SortType
        bool descending{false}; // 是否倒序
        int limit{0};           // 分页限制，0=不限制 // TODO: 未来可支持分页，目前数据量不大
        int offset{0};          // 偏移量            // TODO: 未来可支持分页，目前数据量不大
    };

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

    bool 导入类别从JSON(TodoList &categories, const QJsonArray &categoriesArray,
                        ImportSource source = ImportSource::Server,
                        ConflictResolution resolution = ConflictResolution::Merge);

    // 数据库侧过滤 + 排序查询（仅返回符合条件的 id 列表，后续由上层映射为对象）
    QList<int> 查询待办ID列表(const QueryOptions &options);

  private:
    ConflictResolution 评估冲突(const TodoItem *existing, const TodoItem &incoming,
                                ConflictResolution resolution) const; // 返回应执行的动作
    int 获取最后插入行ID(QSqlDatabase &db) const;                     // 获取自增ID
    static QString 构建SQL排序语句(int sortType, bool descending);    // 构建查询SQL

    // 成员变量
    Setting &m_setting;   ///< 设置对象引用
    Database &m_database; ///< 数据库管理器引用
};