/**
 * @file category_manager.h
 * @brief CategoryManager类的头文件
 *
 * 该文件定义了CategoryManager类，用于管理待办事项的类别。
 * 从TodoManager中拆分出来，专门负责类别的CRUD操作和服务器同步。
 *
 * @author Sakurakugu
 * @date 2025-01-24
 * @version 1.0.0
 */

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QStringList>
#include <memory>
#include <vector>

#include "foundation/network_request.h"
#include "items/categorie_item.h"
#include "setting.h"
#include "todo_sync_server.h"
#include "user_auth.h"

/**
 * @class CategoryManager
 * @brief 类别管理器，负责管理所有类别相关的操作
 *
 * CategoryManager类专门负责类别的管理功能，包括：
 *
 * **核心功能：**
 * - 类别的CRUD操作（创建、读取、更新、删除）
 * - 与服务器的同步操作
 * - 本地类别数据的缓存和管理
 *
 * **架构特点：**
 * - 继承自QObject，支持Qt的信号槽机制
 * - 支持QML属性绑定
 * - 使用智能指针管理内存
 * - 提供异步操作接口
 *
 * **使用场景：**
 * - 在QML中进行类别管理
 * - 在C++中作为类别数据访问层
 * - 支持在线/离线模式
 *
 * @note 该类是线程安全的，所有网络操作都在后台线程执行
 * @see CategorieItem, NetworkRequest, TodoSyncServer
 */
class CategoryManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList categories READ getCategories NOTIFY categoriesChanged)

  public:
    /**
     * @brief 构造函数
     *
     * 创建CategoryManager实例，初始化网络管理器和同步管理器。
     *
     * @param syncManager 同步管理器指针
     * @param parent 父对象指针
     */
    explicit CategoryManager(TodoSyncServer *syncManager, QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~CategoryManager();

    // 类别管理相关方法
    Q_INVOKABLE QStringList getCategories() const;                ///< 获取类别列表
    Q_INVOKABLE void fetchCategories();                           ///< 从服务器获取类别列表
    Q_INVOKABLE void createCategory(const QString &name);         ///< 创建新类别
    Q_INVOKABLE void updateCategory(int id, const QString &name); ///< 更新类别名称
    Q_INVOKABLE void deleteCategory(int id);                      ///< 删除类别

    const std::vector<std::unique_ptr<CategorieItem>> &getCategoryItems() const; ///< 获取类别项目列表
    CategorieItem *findCategoryByName(const QString &name) const;                ///< 根据名称查找类别项目
    CategorieItem *findCategoryById(int id) const;                               ///< 根据ID查找类别项目
    Q_INVOKABLE bool categoryExists(const QString &name) const;                  ///< 检查类别名称是否已存在
    void addDefaultCategories();                                                 ///< 添加默认类别
    void clearCategories();                                                      ///< 清空所有类别

  signals:
    void categoriesChanged();                                              ///< 类别列表变化信号
    void categoryOperationCompleted(bool success, const QString &message); ///< 类别操作完成信号
    void fetchCategoriesCompleted(bool success, const QString &message);   ///< 获取类别完成信号
    void createCategoryCompleted(bool success, const QString &message);    ///< 创建类别完成信号
    void updateCategoryCompleted(bool success, const QString &message);    ///< 更新类别完成信号
    void deleteCategoryCompleted(bool success, const QString &message);    ///< 删除类别完成信号

  public slots:
    void onNetworkRequestCompleted(NetworkRequest::RequestType type, const QJsonObject &response); ///< 处理网络请求完成
    void onNetworkRequestFailed(NetworkRequest::RequestType type, NetworkRequest::NetworkError error,
                                const QString &message); ///< 处理网络请求失败

  private:
    void handleFetchCategoriesSuccess(const QJsonObject &response);   ///< 处理获取类别列表成功响应
    void handleCategoryOperationSuccess(const QJsonObject &response); ///< 处理类别操作成功响应
    void updateCategoriesFromJson(const QJsonArray &categoriesArray); ///< 从JSON数组更新类别列表
    bool isValidCategoryName(const QString &name) const;              ///< 验证类别名称
    void updateServerConfig(const QString &apiEndpoint);              ///< 更新服务器配置

    // 成员变量
    std::vector<std::unique_ptr<CategorieItem>> m_categoryItems; ///< 类别项目列表
    QStringList m_categories;                                    ///< 类别名称列表（缓存）
    QString m_categoriesApiEndpoint;                             ///< 类别API端点

    NetworkRequest &m_networkRequest; ///< 网络管理器引用
    TodoSyncServer *m_syncManager;    ///< 同步管理器指针
    UserAuth &m_userAuth;             ///< 用户认证引用
    Setting &m_setting;               ///< 配置管理
};