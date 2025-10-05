/**
 * @file todo_model.h
 * @brief TodoModel类的头文件
 *
 * 该文件定义了TodoModel类，专门负责待办事项的数据模型显示。
 * 从TodoManager中拆分出来，专门负责QAbstractListModel的实现。
 *
 * @author Sakurakugu
 * @date 2025-09-23 18:45:36(UTC+8) 周二
 * @change 2025-09-24 03:45:31(UTC+8) 周三
 */

#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QVariant>

class TodoDataStorage;
class TodoSyncServer;
class TodoQueryer;
class TodoItem;

/**
 * @class TodoModel
 * @brief 待办事项数据模型，专门负责QML界面显示
 *
 * TodoModel类继承自QAbstractListModel，专门负责待办事项数据的视图层：
 *
 * **核心功能：**
 * - 为QML ListView提供数据模型
 * - 支持待办事项数据的增删改查显示
 * - 处理模型数据的角色映射
 * - 响应数据变化并通知视图更新
 * - 支持过滤和排序后的数据显示
 *
 * **架构特点：**
 * - 继承自QAbstractListModel，与Qt视图系统完美集成
 * - 支持QML属性绑定和信号槽机制
 * - 与TodoManager协作，实现数据与视图的分离
 * - 支持动态数据更新和通知
 * - 高效的过滤缓存机制
 *
 * **使用场景：**
 * - 在QML中作为ListView的model
 * - 为待办事项列表提供数据源
 * - 支持待办事项列表的实时更新显示
 * - 处理过滤和排序后的数据展示
 *
 * @note 该类专注于视图层，业务逻辑由TodoManager处理
 * @see TodoManager, TodoItem
 */
class TodoModel : public QAbstractListModel {
    Q_OBJECT

  public:
    /**
     * @enum TodoRoles
     * @brief 定义待办事项模型中的数据角色
     */
    enum TodoRoles {
        IdRole = Qt::UserRole + 1, // 任务ID
        UuidRole,                  // 任务UUID
        UserUuidRole,              // 用户UUID
        TitleRole,                 // 任务标题
        DescriptionRole,           // 任务描述
        CategoryRole,              // 任务分类
        ImportantRole,             // 任务重要程度
        DeadlineRole,              // 任务截止时间
        RecurrenceIntervalRole,    // 循环间隔
        RecurrenceCountRole,       // 循环次数
        RecurrenceStartDateRole,   // 循环开始日期
        IsCompletedRole,           // 任务是否已完成
        CompletedAtRole,           // 任务完成时间
        IsTrashedRole,             // 任务是否已删除
        TrashedAtRole,             // 任务删除时间
        CreatedAtRole,             // 任务创建时间
        UpdatedAtRole,             // 任务更新时间
        SyncedRole                 // 任务是否已同步
    };
    Q_ENUM(TodoRoles)

    explicit TodoModel(TodoDataStorage &dataStorage, TodoSyncServer &syncServer, //
                       TodoQueryer &queryer, QObject *parent = nullptr);
    ~TodoModel();

    // QAbstractListModel 必要的实现方法
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    bool 加载待办(); // 从数据库加载待办事项

    bool 新增待办(const QString &title, const QUuid &userUuid, const QString &description = QString(), //
                  const QString &category = "未分类", bool important = false,                          //
                  const QDateTime &deadline = QDateTime(), int recurrenceInterval = 0,                 //
                  int recurrenceCount = 0, const QDate &recurrenceStartDate = QDate());                //
    bool 更新待办(int index, const QVariantMap &todoData);
    bool 标记完成(int index, bool completed);
    bool 标记删除(int index, bool trashed);                     // isTrashed = 1/0 回收或删除
    bool 软删除待办(int index);                                 // synced = 3
    bool 删除待办(int index);                                   // 永久删除
    bool 删除所有待办(bool deleteLocal, const QUuid &userUuid); // 永久删除所有

    void 更新过滤后的待办();
    void 需要重新筛选();

    void 更新同步管理器的数据();

    // 网络同步操作
    void 与服务器同步();     // 与服务器同步待办事项数据

  signals:
    void dataUpdated(); ///< 数据更新信号

  public slots:
    void onDataChanged();                                        ///< 处理数据变化
    void onRowsInserted();                                       ///< 处理行插入
    void onRowsRemoved();                                        ///< 处理行删除
    void onTodosUpdatedFromServer(const QJsonArray &todosArray); // 处理从服务器更新的待办事项

  private:
    std::vector<std::unique_ptr<TodoItem>> m_todos; ///< 待办事项列表
    QList<TodoItem *> m_filteredTodos;              ///< 过滤后的待办事项列表
    bool m_filterCacheDirty;                        ///< 过滤缓存是否需要更新
    std::unordered_map<int, TodoItem *> m_idIndex;  ///< id -> TodoItem* 快速索引

    // 辅助方法
    QVariant 获取项目数据(const TodoItem *item, int role) const; ///< 根据角色获取项目数据
    QModelIndex 获取内容在待办列表中的索引(TodoItem *todoItem) const;
    TodoItem *获取过滤后的待办(int index) const; // 获取过滤后的项目（带边界检查）

    void 重建ID索引(); ///< 重建 id 索引
    void 添加到ID索引(TodoItem *item);
    void 从ID索引中移除(int id);

    TodoDataStorage &m_dataManager; ///< 数据存储管理器 - 负责数据的增删改查
    TodoSyncServer &m_syncManager;  ///< 同步管理器 - 负责与服务器同步
    TodoQueryer &m_queryer;         ///< 筛选器 - 负责所有筛选排序相关功能
};