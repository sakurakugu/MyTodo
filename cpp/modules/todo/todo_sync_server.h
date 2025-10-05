/**
 * @file todo_sync_server.h
 * @brief TodoSyncServer类的头文件
 *
 * 该文件定义了TodoSyncServer类，专门负责待办事项的服务器同步功能。
 * 将同步逻辑从TodoManager中分离，实现单一职责原则。
 *
 * @author Sakurakugu
 * @date 2025-08-24 23:07:18(UTC+8) 周日
 * @change 2025-10-06 02:13:23(UTC+8) 周一
 */

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QList>

#include "base_sync_server.h"

class TodoItem;

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
class TodoSyncServer : public BaseSyncServer {
    Q_OBJECT

  public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit TodoSyncServer(UserAuth &userAuth, QObject *parent = nullptr);
    ~TodoSyncServer();

    // 同步操作（重写基类方法）
    void 与服务器同步(SyncDirection direction = Bidirectional) override; // 与服务器同步
    void 重置同步状态() override;
    void 取消同步() override;

    // 数据操作接口
    void setTodoItems(const QList<TodoItem *> &items); // 设置待同步的待办事项列表
    QList<TodoItem *> getUnsyncedItems() const;        // 获取未同步的项目

  signals:
    // 数据更新信号（待办事项特有）
    void todosUpdatedFromServer(const QJsonArray &todosArray);  // 从服务器更新的待办事项
    void localChangesUploaded(const QList<TodoItem *> &items);  // 本地更改已上传
    void syncConflictDetected(const QJsonArray &conflictItems); // 检测到同步冲突

  protected slots:
    void onNetworkRequestCompleted(Network::RequestType type,
                                   const QJsonObject &response) override; // 网络请求完成
    void onNetworkRequestFailed(Network::RequestType type, NetworkRequest::NetworkError error,
                                const QString &message) override; // 网络请求失败

  protected:
    // 同步操作实现（重写基类方法）
    void 执行同步(SyncDirection direction) override;           // 执行同步操作
    void 拉取待办();                                           // 从服务器获取待办事项
    void 推送待办();                                           // 推送本地更改到服务器
    void 推送单个项目(TodoItem *item);                         // 推送单个待办事项
    void 推送下个项目();                                       // 推送下一个待办事项
    void handleSingleItemPushSuccess();                        // 处理单个项目推送成功
    void handleSyncSuccess(const QJsonObject &response);       // 处理同步成功
    void handleFetchTodosSuccess(const QJsonObject &response); // 处理获取待办事项成功
    void 处理推送更改成功(const QJsonObject &response);        // 处理推送更改成功
    void pushBatchToServer(const QList<TodoItem *> &batch);    // 推送批次到服务器
    void pushNextBatch();                                      // 推送下一个批次

  private:
    // 数据管理
    QList<TodoItem *> m_todoItems;            ///< 待同步的待办事项列表
    QList<TodoItem *> m_pendingUnsyncedItems; ///< 等待推送的未同步项目
    QList<TodoItem *> m_allUnsyncedItems;     ///< 存储所有待同步项目
    int m_currentPushIndex;                   ///< 当前推送的项目索引
    int m_currentBatchIndex;                  ///< 当前批次索引
    int m_totalBatches;                       ///< 总批次数
    static const int m_maxBatchSize = 100;    ///< 最大批次大小
};