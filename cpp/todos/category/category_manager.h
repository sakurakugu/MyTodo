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

#include <QAbstractListModel>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <memory>
#include <vector>

#include "../items/categorie_item.h"
#include "category_data_storage.h"
#include "category_sync_server.h"
#include "setting.h"
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
 * - 支持QML ListView模型绑定
 *
 * **架构特点：**
 * - 继承自QAbstractListModel，与Qt视图系统完美集成
 * - 支持QML属性绑定和信号槽机制
 * - 使用智能指针管理内存，确保安全性
 * - 实现了缓存机制，提升性能
 * - 统一的错误处理和异常安全机制
 *
 * **使用场景：**
 * - 在QML中作为ListView的model
 * - 在C++中作为类别数据访问层
 * - 支持在线/离线模式切换
 *
 * @note 该类是线程安全的，所有网络操作都在后台线程执行
 * @see CategorieItem, NetworkRequest, TodoSyncServer
 */
class CategoryManager : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QStringList categories READ getCategories NOTIFY categoriesChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY categoriesChanged)

  public:
    /**
     * @enum CategoryRoles
     * @brief 定义类别模型中的数据角色
     */
    enum CategoryRoles {
        IdRole = Qt::UserRole + 1, // 类别ID
        UuidRole,                  // 类别UUID
        NameRole,                  // 类别名称
        UserUuidRole,              // 用户UUID
        CreatedAtRole,             // 创建时间
        SyncedRole,                // 是否已同步
    };
    Q_ENUM(CategoryRoles)

    // 单例模式
    static CategoryManager &GetInstance() {
        static CategoryManager instance;
        return instance;
    }
    // 禁用拷贝构造和赋值操作
    CategoryManager(const CategoryManager &) = delete;
    CategoryManager &operator=(const CategoryManager &) = delete;
    CategoryManager(CategoryManager &&) = delete;
    CategoryManager &operator=(CategoryManager &&) = delete;



    // QAbstractListModel 必要的实现方法
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    // 类别管理相关方法
    Q_INVOKABLE QStringList getCategories() const;                                ///< 获取类别列表
    Q_INVOKABLE void createCategory(const QString &name);                         ///< 创建新类别
    Q_INVOKABLE void updateCategory(const QString &name, const QString &newName); ///< 更新类别名称
    Q_INVOKABLE void deleteCategory(const QString &name);                         ///< 删除类别

    // 同步相关方法
    Q_INVOKABLE void syncWithServer();            ///< 与服务器同步类别
    Q_INVOKABLE void fetchCategoriesFromServer(); ///< 从服务器获取类别
    Q_INVOKABLE void pushCategoriesToServer();    ///< 推送类别到服务器
    Q_INVOKABLE bool isSyncing() const;           ///< 检查是否正在同步

    // 属性访问器
    CategorySyncServer *getSyncServer() const {
        return m_syncServer;
    } ///< 获取同步服务器实例
    CategoryDataStorage *getDataStorage() const {
        return m_dataStorage;
    } ///< 获取数据存储实例

    const std::vector<std::unique_ptr<CategorieItem>> &getCategoryItems() const; ///< 获取类别项目列表
    Q_INVOKABLE CategorieItem *findCategoryByName(const QString &name) const;    ///< 根据名称查找类别项目
    CategorieItem *findCategoryById(int id) const;                               ///< 根据ID查找类别项目
    CategorieItem *findCategoryByUuid(const QUuid &uuid) const;                  ///< 根据UUID查找类别项目
    Q_INVOKABLE bool categoryExists(const QString &name) const;                  ///< 检查类别名称是否已存在
    Q_INVOKABLE CategorieItem *getCategoryAt(int index) const;                   ///< 根据索引获取类别项目
    void addDefaultCategories();                                                 ///< 添加默认类别
    void clearCategories();                                                      ///< 清空所有类别

    // 数据存储相关方法
    void loadCategoriesFromStorage();                                                       ///< 从存储加载类别
    void saveCategoriesStorage();                                                           ///< 保存类别到存储
    void importCategories(const QString &filePath, CategoryDataStorage::FileFormat format); ///< 导入类别数据
    void exportCategories(const QString &filePath, CategoryDataStorage::FileFormat format); ///< 导出类别数据

  signals:
    void categoriesChanged();                 ///< 类别列表变化信号
    void loadingStateChanged();               ///< 加载状态变化信号
    void errorOccurred(const QString &error); ///< 错误发生信号

    // 统一的操作完成信号
    void operationCompleted(const QString &operation, bool success, const QString &message); ///< 操作完成信号

  public slots:
    void onCategoriesUpdatedFromServer(const QJsonArray &categoriesArray); ///< 处理从服务器更新的类别数据

  private:
      explicit CategoryManager(QObject *parent = nullptr);
    ~CategoryManager();

    // 辅助方法
    void updateCategoriesFromJson(const QJsonArray &categoriesArray); ///< 从JSON数组更新类别列表
    bool isValidCategoryName(const QString &name) const;              ///< 验证类别名称

    // 模型相关辅助方法
    QVariant getItemData(const CategorieItem *item, int role) const; ///< 根据角色获取项目数据
    QModelIndex indexFromItem(CategorieItem *categoryItem) const;    ///< 获取指定CategorieItem的模型索引
    CategoryRoles roleFromName(const QString &name) const;           ///< 从名称获取角色

    // 状态管理
    void emitOperationCompleted(const QString &operation, bool success, const QString &message); ///< 发射操作完成信号

    // 性能优化
    void beginModelUpdate(); ///< 开始模型更新
    void endModelUpdate();   ///< 结束模型更新

    // 信号槽连接
    void setupConnections(); ///< 设置信号槽连接

    // 成员变量
    std::vector<std::unique_ptr<CategorieItem>> m_categoryItems; ///< 类别项目列表
    QStringList m_categories;                                    ///< 类别名称列表（缓存）
    QString m_categoriesApiEndpoint;                             ///< 类别API端点

    // 状态变量
    bool m_isLoading;        ///< 是否正在加载
    QString m_lastError;     ///< 最后的错误信息
    QTimer *m_debounceTimer; ///< 防抖定时器

    CategorySyncServer *m_syncServer;   ///< 类别同步服务器对象
    CategoryDataStorage *m_dataStorage; ///< 类别数据存储对象
    Setting &m_setting;                 ///< 配置管理
    UserAuth &m_userAuth;               ///< 用户认证管理
};