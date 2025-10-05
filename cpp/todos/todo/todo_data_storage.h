/**
 * @file todo_data_storage.h
 * @brief TodoDataStorage类的头文件
 *
 * 该文件定义了TodoDataStorage类，专门负责待办事项的本地存储和文件导入导出功能。
 * 从TodoManager类中拆分出来，实现单一职责原则。
 *
 * @author Sakurakugu
 * @date 2025-08-25 00:54:11(UTC+8) 周一
 * @change 2025-09-24 03:10:10(UTC+8) 周三
 */

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <algorithm>
#include <memory>
#include <vector>

#include "domain_base/base_data_storage.h"

class TodoItem;

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
class TodoDataStorage : public BaseDataStorage {
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

    // 构造函数
    explicit TodoDataStorage(QObject *parent = nullptr);
    ~TodoDataStorage() override;

    // 数据库初始化
    bool 初始化数据表() override;

    // 本地存储功能
    bool 加载待办(TodoList &todos);

    // CRUD操作
    bool 新增待办(TodoList &todos, const QString &title, const QString &description, const QString &category,
                  bool important, const QDateTime &deadline, int recurrenceInterval, int recurrenceCount,
                  const QDate &recurrenceStartDate, QUuid userUuid);
    bool 新增待办(TodoList &todos, std::unique_ptr<TodoItem> item);
    bool 更新待办(TodoList &todos, const QUuid &uuid, const QVariantMap &todoData);
    bool 更新待办(TodoList &todos, TodoItem &item);
    bool 回收待办(TodoList &todos, const QUuid &uuid);   // (标记isTrashed为删除)
    bool 软删除待办(TodoList &todos, const QUuid &uuid); // (标记synced为删除)
    bool 删除待办(TodoList &todos, const QUuid &uuid);

    // 退出登录操作
    bool 删除所有待办(TodoList &todos);
    bool 更新所有待办用户UUID(TodoList &todos, const QUuid &newUserUuid, int synced);

    bool 导入待办事项从JSON(TodoList &todos, const QJsonArray &todosArray, //
                            ImportSource source = ImportSource::Server,    //
                            解决冲突方案 resolution = 解决冲突方案::Merge);

    // 数据库侧过滤 + 排序查询（仅返回符合条件的 id 列表，后续由上层映射为对象）
    QList<int> 查询待办ID列表(const QueryOptions &options);

    // IDataExporter接口实现
    bool 导出到JSON(QJsonObject &output) override;
    bool 导入从JSON(const QJsonObject &input, bool replaceAll) override;

  private:
    static QString 构建SQL排序语句(int sortType, bool descending);    // 构建查询SQL

    // 数据库操作
    bool 创建数据表() override; // 创建todos表
};