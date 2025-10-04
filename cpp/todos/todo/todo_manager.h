/**
 * @file todo_manager.h
 * @brief TodoManager类的头文件
 *
 * 该文件定义了TodoManager类，用于管理待办事项的数据模型。
 *
 * @author Sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @change 2025-09-24 00:55:58(UTC+8) 周三
 */

#pragma once

#include <QDate>
#include <QObject>
#include <QVariant>

// 要不声明这些类型为不透明指针，用于Qt MOC；要不直接用头文件
#include "todo_model.h"   // 数据模型
#include "todo_queryer.h" // 筛选管理器

class UserAuth;
class GlobalState;
class NetworkRequest;
class CategoryManager;
class TodoItem;
class TodoSyncServer;
class TodoDataStorage;

/**
 * @class TodoManager
 * @brief 待办事项管理器，负责管理所有待办事项的业务逻辑
 *
 * TodoManager类是应用程序的核心业务逻辑类，提供了完整的待办事项管理功能：
 *
 * **核心功能：**
 * - 待办事项的CRUD操作（创建、读取、更新、删除）
 * - 业务逻辑处理和数据验证
 * - 与服务器的同步操作
 * - 为视图模型提供数据访问接口
 *
 * **架构特点：**
 * - 专注于业务逻辑处理，不直接处理视图层
 * - 使用智能指针管理内存，确保安全性
 * - 实现了过滤缓存机制，提升性能
 * - 通过信号槽机制通知数据变化
 *
 * **使用场景：**
 * - 作为TodoModel的数据源
 * - 在C++中作为数据访问层
 * - 支持在线/离线模式切换
 *
 * @note 该类是线程
 *
 * 安全的，所有网络操作都在后台线程执行
 * @see TodoItem, CategorieItem, NetworkRequest, Config, TodoModel
 */
class TodoManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(TodoQueryer *queryer READ queryer CONSTANT)
    Q_PROPERTY(TodoModel *todoModel READ todoModel CONSTANT)

  public:
    explicit TodoManager(UserAuth &userAuth, CategoryManager &categoryManager, QObject *parent = nullptr);
    ~TodoManager();

    // 访问器
    Q_INVOKABLE TodoQueryer *queryer() const;
    TodoModel *todoModel() const;

    // CRUD操作
    void loadTodo();                                                                               // 加载待办
    Q_INVOKABLE void addTodo(const QString &title, const QString &description = QString(),         //
                             const QString &category = "未分类", bool important = false,           //
                             const QString &deadline = QString(), int recurrenceInterval = 0,      //
                             int recurrenceCount = 0, const QDate &recurrenceStartDate = QDate()); // 添加新的待办事项
    Q_INVOKABLE bool updateTodo(int index, const QString &roleName, const QVariant &value);        // 更新现有待办事项
    Q_INVOKABLE bool updateTodo();                                // 更新现有待办事项（使用全局状态中的选中待办）
    Q_INVOKABLE bool markAsRemove(int index, bool remove = true); // 标记待办事项为删除状态
    Q_INVOKABLE bool markAsDone(int index, bool remove = true);   // 标记待办为已完成或未完成
    Q_INVOKABLE bool permanentlyDeleteTodo(int index);            // 删除待办事项
    Q_INVOKABLE void deleteAllTodos(bool deleteLocal);            // 删除所有待办事项
    Q_INVOKABLE void syncWithServer();                            // 与服务器同步
    Q_INVOKABLE void forceSyncWithServer();                       // 强制与服务器同步

  signals:
    // 转发同步相关信号供 QML 使用（底层由 TodoSyncServer/BaseSyncServer 发出）
    void syncStarted();
    void syncCompleted(int result, const QString &message = QString());

  private slots:
    void onFirstAuthCompleted(); // 处理首次认证完成信号

  private:
    // 管理器
    NetworkRequest &m_networkRequest; ///< 网络管理器
    UserAuth &m_userAuth;             ///< 用户认证管理器
    GlobalState &m_globalState;       ///< 全局状态管理器

    TodoDataStorage *m_dataManager;     ///< 数据管理器 - 负责本地存储和文件导入导出
    TodoSyncServer *m_syncManager;      ///< 同步管理器 - 负责所有服务器同步相关功能
    CategoryManager *m_categoryManager; ///< 类别管理器 - 负责类别相关操作
    TodoQueryer *m_queryer;             ///< 查询管理器 - 负责所有筛选排序相关功能
    TodoModel *m_todoModel;             ///< 待办事项数据模型
};
