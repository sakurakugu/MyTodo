/**
 * @file base_sync_server.h
 * @brief BaseSyncServer 抽象基类头文件
 *
 * 抽象出分类与待办同步逻辑的通用部分，统一自动/手动同步调度、状态管理、错误处理与网络交互入口，
 * 降低 CategorySyncServer / TodoSyncServer 的重复实现。与 UserAuth 协作，在首次认证完成后触发初始同步。
 *
 * @author Sakurakugu
 * @date 2025-09-10 22:45:52(UTC+8) 周三
 * @change 2025-10-06 01:32:19(UTC+8) 周一
 */

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QTimer>

#include "network_request.h"

class Config;
class UserAuth;

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
 * @see CategorySyncServer, TodoSyncServer, NetworkRequest, config, UserAuth
 */
class BaseSyncServer : public QObject {
    Q_OBJECT
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

    /**
     * @enum SyncDirection
     * @brief 同步方向枚举
     */
    enum SyncDirection {
        Bidirectional = 0, // 双向同步（默认）
        UploadOnly = 1,    // 仅上传
        DownloadOnly = 2   // 仅下载
    };

    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit BaseSyncServer(UserAuth &userAuth, QObject *parent = nullptr);
    virtual ~BaseSyncServer();

    // 属性访问器
    bool isSyncing() const; // 获取当前是否正在同步
    void setIsSyncing(bool syncing);  // 设置当前同步状态

    // 同步操作（纯虚函数，由子类实现）
    virtual void 与服务器同步(SyncDirection direction = Bidirectional) = 0;
    virtual void 重置同步状态();
    virtual void 取消同步();

  signals:
    // 同步状态信号
    void syncStarted();                                                        // 同步开始
    void syncCompleted(SyncResult result, const QString &message = QString()); // 同步完成
    void syncProgress(int percentage, const QString &status);                  // 同步进度
    void syncingChanged();                                                     // 同步状态变化

    // 配置和状态信号
    void autoSyncEnabledChanged();  // 自动同步启用状态变化
    void autoSyncIntervalChanged(); // 自动同步间隔变化

  protected slots:
    virtual void onNetworkRequestCompleted(Network::RequestType type,
                                           const QJsonObject &response); // 网络请求完成
    virtual void onNetworkRequestFailed(Network::RequestType type, NetworkRequest::NetworkError error,
                                        const QString &message); // 网络请求失败
    void onAutoSyncTimer();                                      // 自动同步定时器触发

  protected:
    // 同步操作实现（由子类重写）
    virtual void 执行同步(SyncDirection direction) = 0; // 执行同步操作

    // 辅助方法
    void 更新最后同步时间();
    bool 是否可以执行同步() const;
    void 开启自动同步计时器();
    void 停止自动同步计时器();
    void 检查同步前置条件(bool allowOngoingPhase = false);

    // 成员变量
    NetworkRequest &m_networkRequest; ///< 网络请求管理器
    Config &m_config;                 ///< 应用配置管理器
    UserAuth &m_userAuth;             ///< 用户认证管理器
    QTimer *m_autoSyncTimer;          ///< 自动同步定时器

    // 同步状态
    bool m_isSyncing;                     ///< 当前是否正在同步
    QString m_lastSyncTime;               ///< 最后同步时间
    int m_autoSyncInterval;               ///< 自动同步间隔（分钟）
    SyncDirection m_currentSyncDirection; ///< 当前同步方向

    // 策略标志：在双向同步且存在本地未同步更改时，先推送再拉取，避免先拉取造成改名被当作新增
    bool m_pushFirstInBidirectional = false;

    // 服务器配置
    QString m_apiEndpoint; ///< API端点
};