/**
 * @file category_manager.h
 * @brief CategoryManager类的头文件
 *
 * 该文件定义了CategoryManager类，用于管理待办事项的类别。
 *
 * @author Sakurakugu
 * @date 2025-08-25 01:28:49(UTC+8) 周一
 * @change 2025-09-03 00:37:33(UTC+8) 周三
 * @version 0.4.0
 */

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <memory>
#include <vector>

#include "categorie_item.h"
#include "category_data_storage.h"
#include "category_model.h"
#include "category_sync_server.h"
#include "setting.h"
#include "user_auth.h"

/**
 * @class CategoryManager
 * @brief 类别管理器，负责管理所有类别相关的业务逻辑，暴露给QML
 *
 * CategoryManager类专门负责类别的管理功能，包括：
 *
 * **核心功能：**
 * - 类别的CRUD操作（创建、读取、更新、删除）
 * - 与服务器的同步操作
 * - 本地类别数据的缓存和管理
 * - 为视图模型提供数据访问接口
 *
 * **架构特点：**
 * - 专注于业务逻辑处理，不直接处理视图层
 * - 使用智能指针管理内存，确保安全性
 * - 实现了缓存机制，提升性能
 * - 统一的错误处理和异常安全机制
 * - 通过信号槽机制通知数据变化
 *
 * **使用场景：**
 * - 作为CategoryModel的数据源
 * - 在C++中作为类别数据访问层
 * - 支持在线/离线模式切换
 *
 * @note 该类是线程安全的，所有网络操作都在后台线程执行
 * @see CategorieItem, NetworkRequest, CategorySyncServer, CategoryModel
 */
class CategoryManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList categories READ getCategories NOTIFY categoriesChanged)

  public:
    explicit CategoryManager(UserAuth &userAuth, QObject *parent = nullptr);
    ~CategoryManager();

    // 类别管理相关方法
    QStringList getCategories() const;                                            ///< 获取类别列表
    Q_INVOKABLE void createCategory(const QString &name);                         ///< 创建新类别
    Q_INVOKABLE void updateCategory(const QString &name, const QString &newName); ///< 更新类别名称
    Q_INVOKABLE void deleteCategory(const QString &name);                         ///<
    void loadCategories();                                                        ///< 加载类别

    // 同步相关方法
    Q_INVOKABLE void syncWithServer();  ///< 与服务器同步类别
    Q_INVOKABLE bool isSyncing() const; ///< 检查是否正在同步

    Q_INVOKABLE bool categoryExists(const QString &name) const; ///< 检查类别名称是否已存在

  signals:
    void categoriesChanged();                 ///< 类别列表变化信号
    void loadingStateChanged();               ///< 加载状态变化信号
    void errorOccurred(const QString &error); ///< 错误发生信号

  public slots:
    void onCategoriesUpdatedFromServer(const QJsonArray &categoriesArray);            ///< 处理从服务器更新的类别数据
    void onLocalChangesUploaded(const std::vector<CategorieItem *> &m_unsyncedItems); ///< 处理本地更改已上传
  private:
    // 成员变量
    CategorySyncServer *m_syncServer;   ///< 类别同步服务器对象
    CategoryDataStorage *m_dataStorage; ///< 类别数据存储对象
    CategoryModel *m_categoryModel;     ///< 类别模型对象
    UserAuth &m_userAuth;               ///< 用户认证管理
};