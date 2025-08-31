/**
 * @file todo_sync_server.h
 * @brief TodoSyncServer类的头文件
 *
 * 该文件定义了TodoSyncServer类，专门负责待办事项的服务器同步功能。
 * 将同步逻辑从TodoManager中分离，实现单一职责原则。
 *
 * @author Sakurakugu
 * @date 2025-01-25
 * @version 1.0.0
 */

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>

#include "foundation/network_request.h"
#include "items/todo_item.h"
#include "setting.h"

/**
 * @class TodoSyncServer
 * @brief 待办事项同步管理器，负责与服务器的数据同步
 *
 * TodoSyncServer类专门处理待办事项与服务器的同步操作，包括：
 *
 * **核心功能：**
 * - 双向数据同步（上传本地更改，下载服务器数据）
 * - 自动同步和手动同步
 * - 冲突检测和解决
 * - 离线模式支持
 * - 同步状态管理
 *
 * **设计原则：**
 * - 单一职责：专注于同步功能
 * - 松耦合：通过信号槽与其他组件通信
 * - 可配置：支持同步策略配置
 * - 容错性：处理网络异常和数据冲突
 *
 * **使用场景：**
 * - 用户登录后的数据同步
 * - 定时自动同步
 * - 手动触发同步
 * - 网络恢复后的数据恢复
 *
 * **重构说明：**
 * 从TodoManager中重构出来的独立同步管理类，实现以下目标：
 * - 分离关注点：将同步逻辑从TodoManager中独立出来
 * - 提高可维护性：同步相关代码集中管理
 * - 增强可测试性：可以独立测试同步功能
 * - 改善代码组织：遵循单一职责原则
 *
 * @note 该类是线程安全的，所有网络操作都在后台线程执行
 * @see TodoItem, NetworkRequest, Setting
 */
class TodoSyncServer : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isAutoSyncEnabled READ isAutoSyncEnabled WRITE setAutoSyncEnabled NOTIFY autoSyncEnabledChanged)
    Q_PROPERTY(bool isSyncing READ isSyncing NOTIFY syncingChanged)
    Q_PROPERTY(QString lastSyncTime READ lastSyncTime NOTIFY lastSyncTimeChanged)
    Q_PROPERTY(int autoSyncInterval READ autoSyncInterval WRITE setAutoSyncInterval NOTIFY autoSyncIntervalChanged)

  public:
    /**
     * @enum SyncResult
     * @brief 同步操作结果枚举
     */
    enum SyncResult {
        Success = 0,       // 同步成功
        NetworkError = 1,  // 网络错误
        AuthError = 2,     // 认证错误
        ConflictError = 3, // 数据冲突
        UnknownError = 4   // 未知错误
    };
    Q_ENUM(SyncResult)

    /**
     * @enum SyncDirection
     * @brief 同步方向枚举
     */
    enum SyncDirection {
        Bidirectional = 0, // 双向同步（默认）
        UploadOnly = 1,    // 仅上传
        DownloadOnly = 2   // 仅下载
    };
    Q_ENUM(SyncDirection)

    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit TodoSyncServer(QObject *parent = nullptr);
    ~TodoSyncServer();

    // 属性访问器
    Q_INVOKABLE bool isAutoSyncEnabled() const;        // 获取自动同步是否启用
    Q_INVOKABLE void setAutoSyncEnabled(bool enabled); // 设置自动同步启用状态
    bool isSyncing() const;                            // 获取当前是否正在同步
    QString lastSyncTime() const;                      // 获取最后同步时间
    int autoSyncInterval() const;                      // 获取自动同步间隔（分钟）
    void setAutoSyncInterval(int minutes);             // 设置自动同步间隔

    // 同步操作
    Q_INVOKABLE void syncWithServer(SyncDirection direction = Bidirectional); // 与服务器同步
    Q_INVOKABLE void cancelSync();                                            // 取消当前同步操作
    Q_INVOKABLE void resetSyncState();                                        // 重置同步状态

    // 数据操作接口
    void setTodoItems(const QList<TodoItem *> &items); // 设置待同步的待办事项列表
    QList<TodoItem *> getUnsyncedItems() const;        // 获取未同步的项目
    void markItemAsSynced(TodoItem *item);             // 标记项目为已同步
    void markItemAsUnsynced(TodoItem *item);           // 标记项目为未同步

    // 配置管理
    void updateServerConfig(const QString &baseUrl, const QString &apiEndpoint); // 更新服务器配置
    QString getServerBaseUrl() const;                                            // 获取服务器基础URL
    QString getApiEndpoint() const;                                              // 获取API端点
    QString getApiUrl(const QString &endpoint) const;                            // 获取完整API URL

  signals:
    // 同步状态信号
    void syncStarted();                                                        // 同步开始
    void syncCompleted(SyncResult result, const QString &message = QString()); // 同步完成
    void syncProgress(int percentage, const QString &status);                  // 同步进度
    void syncingChanged();                                                     // 同步状态变化

    // 数据更新信号
    void todosUpdatedFromServer(const QJsonArray &todosArray);  // 从服务器更新的待办事项
    void localChangesUploaded(const QList<TodoItem *> &items);  // 本地更改已上传
    void syncConflictDetected(const QJsonArray &conflictItems); // 检测到同步冲突

    // 配置和状态信号
    void autoSyncEnabledChanged();  // 自动同步启用状态变化
    void lastSyncTimeChanged();     // 最后同步时间变化
    void autoSyncIntervalChanged(); // 自动同步间隔变化
    void serverConfigChanged();     // 服务器配置变化

  private slots:
    void onNetworkRequestCompleted(NetworkRequest::RequestType type, const QJsonObject &response); // 网络请求完成
    void onNetworkRequestFailed(NetworkRequest::RequestType type, NetworkRequest::NetworkError error,
                                const QString &message); // 网络请求失败
    void onAutoSyncTimer();                              // 自动同步定时器触发
    void onBaseUrlChanged(const QString &newBaseUrl);    // 服务器URL变化

  private:
    // 同步操作实现
    void performSync(SyncDirection direction);                     // 执行同步操作
    void fetchTodosFromServer();                                   // 从服务器获取待办事项
    void pushLocalChangesToServer();                               // 推送本地更改到服务器
    void pushSingleItem(TodoItem *item);                           // 推送单个待办事项
    void pushNextItem();                                           // 推送下一个待办事项
    void handleSingleItemPushSuccess(const QJsonObject &response); // 处理单个项目推送成功
    void handleSyncSuccess(const QJsonObject &response);           // 处理同步成功
    void handleFetchTodosSuccess(const QJsonObject &response);     // 处理获取待办事项成功
    void handlePushChangesSuccess(const QJsonObject &response);    // 处理推送更改成功
    void pushBatchToServer(const QList<TodoItem *> &batch);        // 推送批次到服务器
    void pushNextBatch();                                          // 推送下一个批次

    // 辅助方法
    void initializeServerConfig(); // 初始化服务器配置
    void updateLastSyncTime();     // 更新最后同步时间
    bool canPerformSync() const;   // 检查是否可以执行同步
    void startAutoSyncTimer();     // 启动自动同步定时器
    void stopAutoSyncTimer();      // 停止自动同步定时器

    // 成员变量
    NetworkRequest &m_networkRequest; ///< 网络请求管理器
    Setting &m_setting;               ///< 应用设置
    QTimer *m_autoSyncTimer;          ///< 自动同步定时器

    // 同步状态
    bool m_isAutoSyncEnabled;             ///< 自动同步是否启用
    bool m_isSyncing;                     ///< 当前是否正在同步
    QString m_lastSyncTime;               ///< 最后同步时间
    int m_autoSyncInterval;               ///< 自动同步间隔（分钟）
    SyncDirection m_currentSyncDirection; ///< 当前同步方向

    // 服务器配置
    QString m_serverBaseUrl;   ///< 服务器基础URL
    QString m_todoApiEndpoint; ///< 待办事项API端点

    // 数据管理
    QList<TodoItem *> m_todoItems;            ///< 待同步的待办事项列表
    QList<TodoItem *> m_pendingUnsyncedItems; ///< 等待推送的未同步项目
    QList<TodoItem *> m_allUnsyncedItems;     ///< 存储所有待同步项目
    int m_currentPushIndex;                   ///< 当前推送的项目索引
    int m_currentBatchIndex;                  ///< 当前批次索引
    int m_totalBatches;                       ///< 总批次数
    static const int m_maxBatchSize = 100;    ///< 最大批次大小
};