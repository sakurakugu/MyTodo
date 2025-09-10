/**
 * @file base_sync_server.h
 * @brief BaseSyncServer基类的头文件
 *
 * 该文件定义了BaseSyncServer基类，提取CategorySyncServer和TodoSyncServer的共同功能。
 * 实现代码复用，减少重复代码，提高可维护性。
 *
 * @author Sakurakugu
 * @date 2025-01-27 15:00:00(UTC+8) 周一
 * @version 0.4.0
 */

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QTimer>

class NetworkRequest;
class Setting;

#include "foundation/network_request.h"
#include "setting.h"

/**
 * @class BaseSyncServer
 * @brief 同步服务器基类，提供通用的同步功能
 *
 * BaseSyncServer类提供了同步服务器的通用功能，包括：
 *
 * **核心功能：**
 * - 自动同步和手动同步管理
 * - 同步状态管理
 * - 网络请求处理
 * - 定时器管理
 * - 服务器配置管理
 *
 * **设计原则：**
 * - 代码复用：提取共同功能到基类
 * - 单一职责：专注于通用同步功能
 * - 松耦合：通过信号槽与其他组件通信
 * - 可扩展：子类可以扩展特定功能
 *
 * **使用场景：**
 * - 作为CategorySyncServer和TodoSyncServer的基类
 * - 提供通用的同步管理功能
 * - 统一同步行为和接口
 *
 * @note 该类是抽象基类，不能直接实例化
 * @see CategorySyncServer, TodoSyncServer, NetworkRequest, Setting
 */
class BaseSyncServer : public QObject {
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
    explicit BaseSyncServer(NetworkRequest *networkRequest, Setting *setting, QObject *parent = nullptr);
    virtual ~BaseSyncServer();

    // 属性访问器
    Q_INVOKABLE bool isAutoSyncEnabled() const;        // 获取自动同步是否启用
    Q_INVOKABLE void setAutoSyncEnabled(bool enabled); // 设置自动同步启用状态
    bool isSyncing() const;                            // 获取当前是否正在同步
    QString lastSyncTime() const;                      // 获取最后同步时间
    int autoSyncInterval() const;                      // 获取自动同步间隔（分钟）
    void setAutoSyncInterval(int minutes);             // 设置自动同步间隔

    // 同步操作（纯虚函数，由子类实现）
    Q_INVOKABLE virtual void syncWithServer(SyncDirection direction = Bidirectional) = 0; // 与服务器同步
    Q_INVOKABLE virtual void cancelSync();                                                 // 取消当前同步操作
    Q_INVOKABLE virtual void resetSyncState();                                             // 重置同步状态

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

    // 配置和状态信号
    void autoSyncEnabledChanged();  // 自动同步启用状态变化
    void lastSyncTimeChanged();     // 最后同步时间变化
    void autoSyncIntervalChanged(); // 自动同步间隔变化
    void serverConfigChanged();     // 服务器配置变化

  protected slots:
    virtual void onNetworkRequestCompleted(NetworkRequest::RequestType type, const QJsonObject &response); // 网络请求完成
    virtual void onNetworkRequestFailed(NetworkRequest::RequestType type, NetworkRequest::NetworkError error,
                                         const QString &message); // 网络请求失败
    void onAutoSyncTimer();                                       // 自动同步定时器触发
    void onBaseUrlChanged(const QString &newBaseUrl);             // 服务器URL变化

  protected:
    // 同步操作实现（由子类重写）
    virtual void performSync(SyncDirection direction) = 0; // 执行同步操作

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
    QString m_serverBaseUrl; ///< 服务器基础URL
    QString m_apiEndpoint;   ///< API端点
};