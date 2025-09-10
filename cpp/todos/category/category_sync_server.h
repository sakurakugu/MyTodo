/**
 * @file category_sync_server.h
 * @brief CategorySyncServer类的头文件
 *
 * 该文件定义了CategorySyncServer类，专门负责类别的服务器同步功能。
 * 将同步逻辑从CategoryManager中分离，实现单一职责原则。
 *
 * @author Sakurakugu
 * @date 2025-01-09 00:00:00(UTC+8) 周四
 * @version 0.4.0
 */

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>
#include <memory>
#include <vector>

#include "foundation/network_request.h"
#include "../items/categorie_item.h"
#include "setting.h"

/**
 * @class CategorySyncServer
 * @brief 类别同步管理器，负责与服务器的类别数据同步
 *
 * CategorySyncServer类专门处理类别与服务器的同步操作，包括：
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
 * - 用户登录后的类别数据同步
 * - 定时自动同步
 * - 手动触发同步
 * - 网络恢复后的数据恢复
 *
 * **重构说明：**
 * 从CategoryManager中重构出来的独立同步管理类，实现以下目标：
 * - 分离关注点：将同步逻辑从CategoryManager中独立出来
 * - 提高可维护性：同步相关代码集中管理
 * - 增强可测试性：可以独立测试同步功能
 * - 改善代码组织：遵循单一职责原则
 *
 * @note 该类是线程安全的，所有网络操作都在后台线程执行
 * @see CategorieItem, NetworkRequest, Setting
 */
class CategorySyncServer : public QObject {
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
    explicit CategorySyncServer(QObject *parent = nullptr);
    ~CategorySyncServer();

    // 属性访问器
    Q_INVOKABLE bool isAutoSyncEnabled() const;        // 获取自动同步是否启用
    Q_INVOKABLE void setAutoSyncEnabled(bool enabled); // 设置自动同步启用状态
    bool isSyncing() const;                            // 获取当前是否正在同步
    QString lastSyncTime() const;                      // 获取最后同步时间
    int autoSyncInterval() const;                      // 获取自动同步间隔（分钟）
    void setAutoSyncInterval(int minutes);             // 设置自动同步间隔

    // 同步操作
    Q_INVOKABLE void syncWithServer(SyncDirection direction = Bidirectional); // 与服务器同步
    Q_INVOKABLE void fetchCategories();                                       // 从服务器获取类别列表
    Q_INVOKABLE void pushCategories();                                        // 推送类别列表到服务器
    Q_INVOKABLE void cancelSync();                                            // 取消当前同步操作
    Q_INVOKABLE void resetSyncState();                                        // 重置同步状态

    // 类别操作接口
    Q_INVOKABLE void createCategoryWithServer(const QString &name);                         // 创建新类别到服务器
    Q_INVOKABLE void updateCategoryWithServer(const QString &name, const QString &newName); // 更新类别名称到服务器
    Q_INVOKABLE void deleteCategoryWithServer(const QString &name);                         // 删除类别到服务器

    // 数据操作接口
    void setCategoryItems(const std::vector<std::unique_ptr<CategorieItem>> &items); // 设置待同步的类别列表
    std::vector<CategorieItem *> getUnsyncedItems() const;                          // 获取未同步的项目
    void markItemAsSynced(CategorieItem *item);                                     // 标记项目为已同步
    void markItemAsUnsynced(CategorieItem *item);                                   // 标记项目为未同步

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
    void categoriesUpdatedFromServer(const QJsonArray &categoriesArray);    // 从服务器更新的类别
    void localChangesUploaded(const std::vector<CategorieItem *> &items);   // 本地更改已上传
    void syncConflictDetected(const QJsonArray &conflictItems);             // 检测到同步冲突

    // 类别操作信号
    void categoryCreated(const QString &name, bool success, const QString &message);                         // 类别创建完成
    void categoryUpdated(const QString &oldName, const QString &newName, bool success, const QString &message); // 类别更新完成
    void categoryDeleted(const QString &name, bool success, const QString &message);                         // 类别删除完成

    // 配置和状态信号
    void autoSyncEnabledChanged();  // 自动同步启用状态变化
    void lastSyncTimeChanged();     // 最后同步时间变化
    void autoSyncIntervalChanged(); // 自动同步间隔变化
    void serverConfigChanged();     // 服务器配置变化

    // 统一的操作完成信号
    void operationCompleted(const QString &operation, bool success, const QString &message); // 操作完成信号

  private slots:
    void onNetworkRequestCompleted(NetworkRequest::RequestType type, const QJsonObject &response); // 网络请求完成
    void onNetworkRequestFailed(NetworkRequest::RequestType type, NetworkRequest::NetworkError error,
                                const QString &message); // 网络请求失败
    void onAutoSyncTimer();                              // 自动同步定时器触发
    void onBaseUrlChanged(const QString &newBaseUrl);    // 服务器URL变化

  private:
    // 同步操作实现
    void performSync(SyncDirection direction);                     // 执行同步操作
    void fetchCategoriesFromServer();                              // 从服务器获取类别
    void pushLocalChangesToServer();                               // 推送本地更改到服务器
    void pushSingleItem(CategorieItem *item);                      // 推送单个类别
    void pushNextItem();                                           // 推送下一个类别
    void handleSingleItemPushSuccess();                            // 处理单个项目推送成功
    void handleSyncSuccess(const QJsonObject &response);           // 处理同步成功
    void handleFetchCategoriesSuccess(const QJsonObject &response); // 处理获取类别成功
    void handlePushChangesSuccess(const QJsonObject &response);    // 处理推送更改成功
    void pushBatchToServer(const std::vector<CategorieItem *> &batch); // 推送批次到服务器
    void pushNextBatch();                                          // 推送下一个批次

    // 类别操作实现
    void handleCategoryOperationSuccess(const QJsonObject &response); // 处理类别操作成功响应
    void handleCreateCategorySuccess(const QJsonObject &response);     // 处理创建类别成功
    void handleUpdateCategorySuccess(const QJsonObject &response);     // 处理更新类别成功
    void handleDeleteCategorySuccess(const QJsonObject &response);     // 处理删除类别成功

    // 辅助方法
    void initializeServerConfig(); // 初始化服务器配置
    void updateLastSyncTime();     // 更新最后同步时间
    bool canPerformSync() const;   // 检查是否可以执行同步
    void startAutoSyncTimer();     // 启动自动同步定时器
    void stopAutoSyncTimer();      // 停止自动同步定时器
    bool isValidCategoryName(const QString &name) const; // 验证类别名称
    void emitOperationCompleted(const QString &operation, bool success, const QString &message); // 发射操作完成信号

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
    QString m_serverBaseUrl;        ///< 服务器基础URL
    QString m_categoriesApiEndpoint; ///< 类别API端点

    // 数据管理
    std::vector<CategorieItem *> m_categoryItems;            ///< 待同步的类别列表
    std::vector<CategorieItem *> m_pendingUnsyncedItems;     ///< 等待推送的未同步项目
    std::vector<CategorieItem *> m_allUnsyncedItems;         ///< 存储所有待同步项目
    int m_currentPushIndex;                                  ///< 当前推送的项目索引
    int m_currentBatchIndex;                                 ///< 当前批次索引
    int m_totalBatches;                                      ///< 总批次数
    static const int m_maxBatchSize = 50;                    ///< 最大批次大小（类别数量相对较少）

    // 当前操作状态
    QString m_currentOperationName;    ///< 当前操作名称（用于类别CRUD操作）
    QString m_currentOperationNewName; ///< 当前操作的新名称（用于更新操作）
};