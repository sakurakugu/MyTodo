/**
 * @file category_sync_server.h
 * @brief CategorySyncServer类的头文件
 *
 * 该文件定义了CategorySyncServer类，专门负责类别的服务器同步功能。
 * 将同步逻辑从CategoryManager中分离，实现单一职责原则。
 *
 * @author Sakurakugu
 * @date 2025-09-10 22:00:00(UTC+8) 周三
 * @change 2025-09-20 23:17:33(UTC+8) 周六
 */

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <memory>
#include <vector>

#include "domain_base/base_sync_server.h"

class CategorieItem;

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
class CategorySyncServer : public BaseSyncServer {
    Q_OBJECT

  public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit CategorySyncServer(UserAuth &userAuth, QObject *parent = nullptr);
    ~CategorySyncServer();

    // 同步操作（重写基类方法）
    void 与服务器同步(SyncDirection direction = Bidirectional) override; // 与服务器同步
    void 取消同步() override;                                            // 取消当前同步操作
    void 重置同步状态() override;                                        // 重置同步状态

    // 类别操作接口
    void 新增类别(const QString &name);
    void 更新类别(const QString &name, const QString &newName);
    void 删除类别(const QString &name);

    // 数据操作接口
    void 设置未同步的对象(const std::vector<std::unique_ptr<CategorieItem>> &categoryItems);

  signals:
    // 数据更新信号（类别特有）
    void categoriesUpdatedFromServer(const QJsonArray &categoriesArray);            // 从服务器更新的类别
    void localChangesUploaded(const std::vector<CategorieItem *> &m_unsyncedItems); // 本地更改已上传
    void syncConflictDetected(const QJsonArray &conflictItems);                     // 检测到同步冲突

  protected slots:
    void onNetworkRequestCompleted(Network::RequestType type,
                                   const QJsonObject &response) override; // 网络请求完成
    void onNetworkRequestFailed(Network::RequestType type, NetworkRequest::NetworkError error,
                                const QString &message) override; // 网络请求失败

  protected:
    // 同步操作实现（重写基类方法）
    void 执行同步(SyncDirection direction) override;
    void 拉取类别();
    void 推送类别();
    void 处理获取类别成功(const QJsonObject &response); // 处理获取类别成功
    void 处理推送更改成功(const QJsonObject &response); // 处理推送更改成功

    // 类别操作实现
    void 处理创建类别成功(const QJsonObject &response); // 处理创建类别成功
    void 处理更新类别成功(const QJsonObject &response); // 处理更新类别成功
    void 处理删除类别成功(const QJsonObject &response); // 处理删除类别成功

    // 辅助方法

  private:
    // 数据管理
    std::vector<CategorieItem *> m_unsyncedItems; ///< 待同步的类别列表

    // 当前操作状态
    QString m_currentOperationName;    ///< 当前操作的对象的名称（用于类别CRUD操作）
    QString m_currentOperationNewName; ///< 当前操作的对象的新名称（用于更新操作）
};