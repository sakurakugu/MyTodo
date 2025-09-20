/**
 * @file todo_manager.h
 * @brief TodoManager类的头文件
 *
 * 该文件定义了TodoManager类，用于管理待办事项的数据模型。
 *
 * @author Sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @change 2025-09-06 01:29:53(UTC+8) 周六
 * @version 0.4.0
 */

#pragma once

#include <QAbstractListModel>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QSortFilterProxyModel>
#include <QVariantMap>
#include <memory>
#include <vector>

#include "../category/category_manager.h" // 类别管理器
#include "foundation/network_request.h"
#include "setting.h"
#include "todo_data_storage.h" // 数据管理器
#include "todo_filter.h"       // 筛选管理器
#include "todo_item.h"
#include "todo_sorter.h"      // 排序管理器
#include "todo_sync_server.h" // 服务器同步管理器

class GlobalState; // 前向声明

/**
 * @class TodoManager
 * @brief 待办事项列表模型，负责管理所有待办事项的核心类
 *
 * TodoManager类是应用程序的核心数据模型，提供了完整的待办事项管理功能：
 *
 * **核心功能：**
 * - 待办事项的CRUD操作（创建、读取、更新、删除）
 *
 * **架构特点：**
 * - 继承自QAbstractListModel，与Qt视图系统完美集成
 * - 支持QML属性绑定和信号槽机制
 * - 使用智能指针管理内存，确保安全性
 * - 实现了过滤缓存机制，提升性能
 *
 * **使用场景：**
 * - 在QML中作为ListView的model
 * - 在C++中作为数据访问层
 * - 支持在线/离线模式切换
 *
 * @note 该类是线程安全的，所有网络操作都在后台线程执行
 * @see TodoItem, CategorieItem, NetworkRequest, Config
 */
class TodoManager : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY isLoggedInChanged)

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
        IsDeletedRole,             // 任务是否已删除
        DeletedAtRole,             // 任务删除时间
        CreatedAtRole,             // 任务创建时间
        UpdatedAtRole,             // 任务更新时间
        LastModifiedAtRole,        // 任务最后修改时间
        SyncedRole                 // 任务是否已同步
    };
    Q_ENUM(TodoRoles)

    explicit TodoManager(UserAuth &userAuth, CategoryManager &categoryManager,QObject *parent = nullptr);
    ~TodoManager();

    // QAbstractListModel 必要的实现方法
    int rowCount(const QModelIndex &parent = QModelIndex()) const override; // 获取模型中的行数（待办事项数量）
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override; // 获取指定索引和角色的数据
    QHash<int, QByteArray> roleNames() const override;                                  // 获取角色名称映射，用于QML访问
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override; // 设置指定索引和角色的数据

    // 筛选和排序功能访问器
    // 访问器
    Q_INVOKABLE TodoFilter *filter() const;
    Q_INVOKABLE TodoSorter *sorter() const;
    Q_INVOKABLE TodoSyncServer *syncServer() const;

    // 属性访问器
    bool isLoggedIn() const;

    // CRUD操作
    Q_INVOKABLE void addTodo(const QString &title, const QString &description = QString(),
                             const QString &category = "未分类", bool important = false,
                             const QString &deadline = QString());                          // 添加新的待办事项
    Q_INVOKABLE bool updateTodo(int index, const QVariantMap &todoData);                    // 更新现有待办事项
    Q_INVOKABLE bool updateTodo(int index, const QString &roleName, const QVariant &value); // 更新现有待办事项
    Q_INVOKABLE bool removeTodo(int index);
    Q_INVOKABLE bool restoreTodo(int index);
    Q_INVOKABLE bool permanentlyDeleteTodo(int index); // 删除待办事项
    Q_INVOKABLE void deleteAllTodos(bool deleteLocal); // 删除所有待办事项
    Q_INVOKABLE bool markAsDoneOrTodo(int index);      // 将待办事项标记为已完成或未完成
    Q_INVOKABLE bool updateAllTodosUserUuid();         // 更新所有待办事项的用户UUID

    // 网络同步操作
    Q_INVOKABLE void syncWithServer(); // 与服务器同步待办事项数据

    // 排序相关
    Q_INVOKABLE void sortTodos(); // 对待办事项进行排序

  signals:
    void syncStarted();                                                        // 同步操作开始信号
    void syncCompleted(bool success, const QString &errorMessage = QString()); // 同步操作完成信号
    void isLoggedInChanged();                                                  // 登录状态改变信号

  private slots:
    void onSyncStarted();                                                            // 处理同步开始
    void onSyncCompleted(TodoSyncServer::SyncResult result, const QString &message); // 处理同步完成
    void onTodosUpdatedFromServer(const QJsonArray &todosArray);                     // 处理从服务器更新的待办事项

  private:
  public:
    // 在应用退出前显式保存到本地存储（避免在析构阶段访问 QSqlDatabase）
    void saveTodosToLocalStorage();

    void updateTodosFromServer(const QJsonArray &todosArray); // 从服务器数据更新待办事项
    void updateSyncManagerData();                             // 更新同步管理器的待办事项数据
    QVariant getItemData(const TodoItem *item, int role) const;
    QModelIndex indexFromItem(TodoItem *todoItem) const; // 获取指定TodoItem的模型索引
    TodoRoles roleFromName(const QString &name) const;   // 从名称获取角色

    // 性能优化相关方法
    void updateFilterCache();                   // 更新过滤缓存
    TodoItem *getFilteredItem(int index) const; // 获取过滤后的项目（带边界检查）
    void invalidateFilterCache();               // 使过滤缓存失效

    // 成员变量
    std::vector<std::unique_ptr<TodoItem>> m_todos; ///< 待办事项列表（使用智能指针）
    QList<TodoItem *> m_filteredTodos;              ///< 过滤后的待办事项列表（让QML快速访问的缓存）
    bool m_filterCacheDirty;                        ///< 过滤缓存是否需要更新
    NetworkRequest &m_networkRequest;               ///< 网络管理器
    Setting &m_setting;                             ///< 应用设置
    bool m_isAutoSync;                              ///< 是否自动同步

    // 管理器
    TodoSyncServer *m_syncManager;      ///< 同步管理器 - 负责所有服务器同步相关功能
    TodoDataStorage *m_dataManager;     ///< 数据管理器 - 负责本地存储和文件导入导出
    CategoryManager *m_categoryManager; ///< 类别管理器 - 负责类别相关操作
    TodoFilter *m_filter;               ///< 筛选管理器 - 负责所有筛选相关功能
    TodoSorter *m_sorter;               ///< 排序管理器 - 负责所有排序相关功能
    UserAuth &m_userAuth;               ///< 用户认证管理器

    // 辅助方法
    int generateUniqueId(); ///< 生成唯一的待办事项ID
};
