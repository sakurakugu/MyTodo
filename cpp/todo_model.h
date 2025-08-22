/**
 * @file todo_model.h
 * @brief TodoModel类的头文件
 *
 * 该文件定义了TodoModel类，用于管理待办事项的数据模型。
 *
 * @author Sakurakugu
 * @date 2025
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

#include "items/todo_item.h"
#include "foundation/network_request.h"
#include "setting.h"

/**
 * @class TodoModel
 * @brief 待办事项列表模型，负责管理所有待办事项的核心类
 *
 * TodoModel类是应用程序的核心数据模型，提供了完整的待办事项管理功能：
 *
 * **核心功能：**
 * - 待办事项的CRUD操作（创建、读取、更新、删除）
 * - 本地数据持久化存储
 * - 网络同步和离线支持
 * - 数据过滤和分类
 * - 用户认证和会话管理
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
class TodoModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(bool isOnline READ isOnline WRITE setIsOnline NOTIFY isOnlineChanged)
    Q_PROPERTY(QString currentCategory READ currentCategory WRITE setCurrentCategory NOTIFY currentCategoryChanged)
    Q_PROPERTY(QString currentFilter READ currentFilter WRITE setCurrentFilter NOTIFY currentFilterChanged)
    Q_PROPERTY(bool currentImportant READ currentImportant WRITE setCurrentImportant NOTIFY currentImportantChanged)
    Q_PROPERTY(QDate dateFilterStart READ dateFilterStart WRITE setDateFilterStart NOTIFY dateFilterStartChanged)
    Q_PROPERTY(QDate dateFilterEnd READ dateFilterEnd WRITE setDateFilterEnd NOTIFY dateFilterEndChanged)
    Q_PROPERTY(bool dateFilterEnabled READ dateFilterEnabled WRITE setDateFilterEnabled NOTIFY dateFilterEnabledChanged)
    Q_PROPERTY(QString username READ getUsername NOTIFY usernameChanged)
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY isLoggedInChanged)
    Q_PROPERTY(QStringList categories READ getCategories NOTIFY categoriesChanged)

  public:
    /**
     * @enum TodoRoles
     * @brief 定义待办事项模型中的数据角色
     */
    enum TodoRoles {
        IdRole = Qt::UserRole + 1, // 任务ID
        UserIdRole,                // 用户ID
        UuidRole,                  // 任务UUID
        TitleRole,                 // 任务标题
        DescriptionRole,           // 任务描述
        CategoryRole,              // 任务分类
        ImportantRole,             // 任务重要程度
        CreatedAtRole,             // 任务创建时间
        UpdatedAtRole,             // 任务更新时间
        SyncedRole,                // 任务是否已同步
        DeadlineRole,              // 任务截止时间
        RecurrenceIntervalRole,    // 循环间隔
        RecurrenceCountRole,       // 循环次数
        RecurrenceStartDateRole,   // 循环开始日期
        IsCompletedRole,           // 任务是否已完成
        CompletedAtRole,           // 任务完成时间
        IsDeletedRole,             // 任务是否已删除
        DeletedAtRole,             // 任务删除时间
        LastModifiedAtRole         // 任务最后修改时间
    };
    Q_ENUM(TodoRoles)

    /**
     * @brief 构造函数
     *
     * 创建TodoModel实例，初始化网络管理器、设置对象和本地存储。
     */
    explicit TodoModel(QObject *parent = nullptr);
    ~TodoModel();

    // QAbstractListModel 必要的实现方法
    int rowCount(const QModelIndex &parent = QModelIndex()) const override; // 获取模型中的行数（待办事项数量）
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override; // 获取指定索引和角色的数据
    QHash<int, QByteArray> roleNames() const override;                                  // 获取角色名称映射，用于QML访问
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override; // 设置指定索引和角色的数据

    // 网络连接状态管理
    bool isOnline() const;         // 获取当前在线状态
    void setIsOnline(bool online); // 设置在线状态

    // 筛选和排序功能
    QString currentCategory() const;                  // 获取当前激活的分类筛选器
    void setCurrentCategory(const QString &category); // 设置分类筛选器
    QString currentFilter() const;                    // 获取当前激活的筛选条件
    void setCurrentFilter(const QString &filter);     // 设置筛选条件
    bool currentImportant() const;                    // 获取当前激活的重要程度筛选器
    void setCurrentImportant(bool important);         // 设置重要程度筛选器
    
    // 日期筛选相关方法
    QDate dateFilterStart() const;                   // 获取日期筛选开始日期
    void setDateFilterStart(const QDate &date);      // 设置日期筛选开始日期
    QDate dateFilterEnd() const;                     // 获取日期筛选结束日期
    void setDateFilterEnd(const QDate &date);        // 设置日期筛选结束日期
    bool dateFilterEnabled() const;                  // 获取日期筛选是否启用
    void setDateFilterEnabled(bool enabled);         // 设置日期筛选是否启用

    // CRUD操作
    Q_INVOKABLE void addTodo(const QString &title, const QString &description = QString(),
                             const QString &category = "default", bool important = false,
                             const QString &deadline = QString());       // 添加新的待办事项
    Q_INVOKABLE bool updateTodo(int index, const QVariantMap &todoData); // 更新现有待办事项
    Q_INVOKABLE bool removeTodo(int index);
    Q_INVOKABLE bool restoreTodo(int index);
    Q_INVOKABLE bool permanentlyDeleteTodo(int index);                              // 删除待办事项
    Q_INVOKABLE bool markAsDone(int index);                              // 将待办事项标记为已完成

    // 网络同步操作
    Q_INVOKABLE void syncWithServer();                                        // 与服务器同步待办事项数据
    Q_INVOKABLE void login(const QString &username, const QString &password); // 使用用户凭据登录服务器
    Q_INVOKABLE void logout();                                                // 注销当前用户
    Q_INVOKABLE bool isLoggedIn() const;                                      // 检查用户是否已登录

    Q_INVOKABLE QString getUsername() const; // 获取用户名
    Q_INVOKABLE QString getEmail() const;    // 获取邮箱

    // 导出导入功能
    Q_INVOKABLE bool exportTodos(const QString &filePath);                  // 导出待办事项到文件
    Q_INVOKABLE bool importTodos(const QString &filePath);                  // 从文件导入待办事项
    Q_INVOKABLE QVariantList checkImportConflicts(const QString &filePath); // 检查导入冲突
    Q_INVOKABLE bool importTodosWithConflictResolution(const QString &filePath,
                                                       const QString &conflictResolution); // 带冲突处理的导入
    Q_INVOKABLE bool importTodosWithIndividualResolution(const QString &filePath,
                                                         const QVariantMap &resolutions); // 带个别冲突处理的导入
    Q_INVOKABLE QVariantList
    importTodosWithAutoResolution(const QString &filePath); // 自动导入无冲突项目，返回冲突项目列表

    // 获取设置对象
    Q_INVOKABLE Config &config() {
        return m_config;
    }

    // 服务器配置相关
    Q_INVOKABLE bool isHttpsUrl(const QString &url) const;       // 检查URL是否使用HTTPS
    Q_INVOKABLE void updateServerConfig(const QString &baseUrl); // 更新服务器配置

    // 类别管理相关
    Q_INVOKABLE QStringList getCategories() const;                    // 获取类别列表
    Q_INVOKABLE void fetchCategories();                               // 从服务器获取类别列表
    Q_INVOKABLE void createCategory(const QString &name);             // 创建新类别
    Q_INVOKABLE void updateCategory(int id, const QString &name);     // 更新类别名称
    Q_INVOKABLE void deleteCategory(int id);                          // 删除类别

  signals:
    void isOnlineChanged();                                                    // 在线状态变化信号
    void currentCategoryChanged();                                             // 当前分类筛选器变化信号
    void currentFilterChanged();                                               // 当前筛选条件变化信号
    void currentImportantChanged();                                            // 当前重要程度筛选器变化信号
    void dateFilterStartChanged();                                             // 日期筛选开始日期变化信号
    void dateFilterEndChanged();                                               // 日期筛选结束日期变化信号
    void dateFilterEnabledChanged();                                           // 日期筛选启用状态变化信号
    void usernameChanged();                                                    // 用户名变化信号
    void isLoggedInChanged();                                                  // 登录状态变化信号
    void syncStarted();                                                        // 同步操作开始信号
    void syncCompleted(bool success, const QString &errorMessage = QString()); // 同步操作完成信号
    void loginSuccessful(const QString &username);                             // 登录成功信号
    void loginFailed(const QString &errorMessage);                             // 登录失败信号
    void loginRequired();                                                      // 需要登录信号
    void logoutSuccessful();                                                   // 退出登录成功信号
    void categoriesChanged();                                                  // 类别列表变化信号
    void categoryOperationCompleted(bool success, const QString &message);    // 类别操作完成信号

  private slots:
    void onNetworkRequestCompleted(NetworkRequest::RequestType type, const QJsonObject &response); // 处理网络请求成功
    void onNetworkRequestFailed(NetworkRequest::RequestType type, NetworkRequest::NetworkError error,
                                const QString &message); // 处理网络请求失败
    void onNetworkStatusChanged(bool isOnline);          // 处理网络状态变化
    void onAuthTokenExpired();                           // 处理认证令牌过期

  private:
    bool loadFromLocalStorage();                                 // 从本地存储加载待办事项
    bool saveToLocalStorage();                                   // 将待办事项保存到本地存储
    void fetchTodosFromServer();                                 // 从服务器获取最新的待办事项
    void pushLocalChangesToServer();                             // 将本地更改推送到服务器
    void handleFetchTodosSuccess(const QJsonObject &response);   // 处理获取待办事项成功
    void handlePushChangesSuccess(const QJsonObject &response);  // 处理推送更改成功
    void handleLoginSuccess(const QJsonObject &response);        // 处理登录成功
    void handleSyncSuccess(const QJsonObject &response);         // 处理同步成功
    void handleFetchCategoriesSuccess(const QJsonObject &response);               // 处理获取类别列表成功响应
    void handleCategoryOperationSuccess(const QJsonObject &response);             // 处理类别操作成功响应
    void updateTodosFromServer(const QJsonArray &todosArray);    // 从服务器数据更新待办事项
    void logError(const QString &context, const QString &error); // 记录错误信息
    QVariant getItemData(const TodoItem *item, int role) const;
    void initializeServerConfig(); // 初始化服务器配置

    // 性能优化相关方法
    void updateFilterCache();                           // 更新过滤缓存
    bool itemMatchesFilter(const TodoItem *item) const; // 检查项目是否匹配当前过滤条件
    TodoItem *getFilteredItem(int index) const;         // 获取过滤后的项目（带边界检查）
    void invalidateFilterCache();                       // 使过滤缓存失效

    // 成员变量
    std::vector<std::unique_ptr<TodoItem>> m_todos; ///< 待办事项列表（使用智能指针）
    QList<TodoItem *> m_filteredTodos;              ///< 过滤后的待办事项列表（缓存）
    bool m_filterCacheDirty;                        ///< 过滤缓存是否需要更新
    bool m_isOnline;                                ///< 是否在线
    QString m_currentCategory;                      ///< 当前分类筛选器
    QString m_currentFilter;                        ///< 当前筛选条件
    bool m_currentImportant;                        ///< 当前重要程度筛选器
    QDate m_dateFilterStart;                        ///< 日期筛选开始日期
    QDate m_dateFilterEnd;                          ///< 日期筛选结束日期
    bool m_dateFilterEnabled;                       ///< 日期筛选是否启用
    NetworkRequest &m_networkRequest;               ///< 网络管理器
    Config &m_config;                               ///< 应用配置
    Setting &m_setting;                             ///< 应用设置

    QString m_accessToken;  ///< 访问令牌
    QString m_refreshToken; ///< 刷新令牌
    QString m_username;     ///< 用户名
    QString m_email;        ///< 邮箱

    // 服务器配置
    QString m_serverBaseUrl;   ///< 服务器基础URL
    QString m_todoApiEndpoint; ///< 待办事项API端点
    QString m_authApiEndpoint; ///< 认证API端点

    // 待同步项目的临时存储
    QList<TodoItem *> m_pendingUnsyncedItems; ///< 待同步项目列表

    // 类别管理相关
    QStringList m_categories; ///< 类别列表

    // 辅助方法
    QString getApiUrl(const QString &endpoint) const; ///< 获取完整的API URL
};
