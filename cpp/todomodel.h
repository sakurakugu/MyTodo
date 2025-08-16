#ifndef TODOMODEL_H
#define TODOMODEL_H

#include "todoItem.h"
#include "settings.h"
#include <QAbstractListModel>
#include <QDebug>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

/**
 * @class TodoModel
 * @brief 待办事项列表模型，负责管理所有待办事项
 *
 * TodoModel类提供了管理待办事项的功能，包括本地存储、网络同步和过滤排序。
 * 该类实现了QAbstractListModel接口，可以与Qt的视图类和QML无缝集成。
 * 支持在线/离线模式，具有自动同步和错误处理能力。
 */
class TodoModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(bool isOnline READ isOnline WRITE setIsOnline NOTIFY isOnlineChanged)
    Q_PROPERTY(QString currentCategory READ currentCategory WRITE setCurrentCategory NOTIFY currentCategoryChanged)
    Q_PROPERTY(QString currentFilter READ currentFilter WRITE setCurrentFilter NOTIFY currentFilterChanged)

  public:
    /**
     * @enum TodoRoles
     * @brief 定义待办事项模型中的数据角色
     */
    enum TodoRoles {
        IdRole = Qt::UserRole + 1, // 任务ID
        TitleRole,                 // 任务标题
        DescriptionRole,           // 任务描述
        CategoryRole,              // 任务分类
        UrgencyRole,               // 任务紧急程度
        ImportanceRole,            // 任务重要程度
        StatusRole,                // 任务状态
        CreatedAtRole,             // 任务创建时间
        UpdatedAtRole,             // 任务更新时间
        SyncedRole                 // 任务是否已同步
    };
    Q_ENUM(TodoRoles)

    explicit TodoModel(QObject *parent = nullptr, Settings *settings = nullptr, Settings::StorageType storageType = Settings::Registry);
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

    // CRUD操作
    Q_INVOKABLE void addTodo(const QString &title, const QString &description = QString(),
                             const QString &category = "default", const QString &urgency = "medium",
                             const QString &importance = "medium");      // 添加新的待办事项
    Q_INVOKABLE bool updateTodo(int index, const QVariantMap &todoData); // 更新现有待办事项
    Q_INVOKABLE bool removeTodo(int index);                              // 删除待办事项
    Q_INVOKABLE bool markAsDone(int index);                              // 将待办事项标记为已完成

    // 网络同步操作
    Q_INVOKABLE void syncWithServer();                                        // 与服务器同步待办事项数据
    Q_INVOKABLE void login(const QString &username, const QString &password); // 使用用户凭据登录服务器
    Q_INVOKABLE void logout();                                                // 注销当前用户
    Q_INVOKABLE bool isLoggedIn() const;                                      // 检查用户是否已登录

    Q_INVOKABLE QString getUsername() const; // 获取用户名
    Q_INVOKABLE QString getEmail() const;    // 获取邮箱
    
    // 导出导入功能
    Q_INVOKABLE bool exportTodos(const QString &filePath); // 导出待办事项到文件
    Q_INVOKABLE bool importTodos(const QString &filePath); // 从文件导入待办事项

    // 获取设置对象
    Q_INVOKABLE Settings* settings() const { return m_settings; }

  signals:
    void isOnlineChanged();                                                    // 在线状态变化信号
    void currentCategoryChanged();                                             // 当前分类筛选器变化信号
    void currentFilterChanged();                                               // 当前筛选条件变化信号
    void syncStarted();                                                        // 同步操作开始信号
    void syncCompleted(bool success, const QString &errorMessage = QString()); // 同步操作完成信号
    void loginSuccessful(const QString &username);                             // 登录成功信号
    void loginFailed(const QString &errorMessage);                             // 登录失败信号
    void logoutSuccessful();                                                   // 退出登录成功信号

  private slots:
    void onNetworkReplyFinished(QNetworkReply *reply); // 处理网络请求完成事件
    void handleLoginResponse(QNetworkReply *reply);    // 处理登录请求的响应
    void handleSyncResponse(QNetworkReply *reply);     // 处理同步请求的响应
    void handleNetworkTimeout();                       // 处理网络请求超时

  private:
    bool loadFromLocalStorage();                                 // 从本地存储加载待办事项
    bool saveToLocalStorage();                                   // 将待办事项保存到本地存储
    void fetchTodosFromServer();                                 // 从服务器获取最新的待办事项
    void pushLocalChangesToServer();                             // 将本地更改推送到服务器
    void logError(const QString &context, const QString &error); // 记录错误信息
    QVariant getItemData(const TodoItem *item, int role) const;
    void initializeServerConfig(); // 初始化服务器配置

    // 成员变量
    QList<TodoItem *> m_todos;               ///< 待办事项列表
    bool m_isOnline;                         ///< 是否在线
    QString m_currentCategory;               ///< 当前分类筛选器
    QString m_currentFilter;                 ///< 当前筛选条件
    QNetworkAccessManager *m_networkManager; ///< 网络管理器
    Settings* m_settings;                    ///< 应用设置
    QTimer *m_networkTimeoutTimer;           ///< 网络请求超时计时器

    QString m_accessToken;  ///< 访问令牌
    QString m_refreshToken; ///< 刷新令牌
    QString m_username;     ///< 用户名
    QString m_email;        ///< 邮箱

    // 服务器配置
    QString m_serverBaseUrl;    ///< 服务器基础URL
    QString m_todoApiEndpoint;  ///< 待办事项API端点
    QString m_authApiEndpoint;  ///< 认证API端点

    static const int NETWORK_TIMEOUT_MS = 10000; ///< 网络请求超时时间(毫秒)
    static const int MAX_RETRIES = 3;            ///< 最大重试次数
    int m_currentRetryCount;                     ///< 当前重试计数

    // 辅助方法
    QString getApiUrl(const QString &endpoint) const; ///< 获取完整的API URL
};

#endif // TODOMODEL_H
