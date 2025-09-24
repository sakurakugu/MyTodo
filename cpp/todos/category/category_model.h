/**
 * @file category_model.h
 * @brief CategoryModel类的头文件
 *
 * 该文件定义了CategoryModel类，专门负责类别的数据模型显示。
 * 从CategoryManager中拆分出来，专门负责QAbstractListModel的实现。
 *
 * @author Sakurakugu
 * @date 2025-09-23 16:11:01(UTC+8) 周二
 * @change 2025-09-24 03:45:31(UTC+8) 周三
 */

#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariant>

#include "categorie_item.h"
#include "category_data_storage.h"
#include "category_sync_server.h"

class CategoryManager; // 前向声明

/**
 * @class CategoryModel
 * @brief 类别数据模型，专门负责QML界面显示
 *
 * CategoryModel类继承自QAbstractListModel，专门负责类别数据的视图层：
 *
 * **核心功能：**
 * - 为QML ListView提供数据模型
 * - 支持类别数据的增删改查显示
 * - 处理模型数据的角色映射
 * - 响应数据变化并通知视图更新
 *
 * **架构特点：**
 * - 继承自QAbstractListModel，与Qt视图系统完美集成
 * - 支持QML属性绑定和信号槽机制
 * - 与CategoryManager协作，实现数据与视图的分离
 * - 支持动态数据更新和通知
 *
 * **使用场景：**
 * - 在QML中作为ListView的model
 * - 为类别选择器提供数据源
 * - 支持类别列表的实时更新显示
 *
 * @note 该类专注于视图层，业务逻辑由CategoryManager处理
 * @see CategoryManager, CategorieItem
 */
class CategoryModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY categoriesChanged)

  public:
    // 定义类别模型中的数据角色
    enum CategoryRoles {
        IdRole = Qt::UserRole + 1, // 类别ID
        UuidRole,                  // 类别UUID
        NameRole,                  // 类别名称
        UserUuidRole,              // 用户UUID
        CreatedAtRole,             // 创建时间
        UpdatedAtRole,             // 更新时间
        SyncedRole,                // 是否已同步
    };
    Q_ENUM(CategoryRoles)

    explicit CategoryModel(CategoryDataStorage &dataStorage, CategorySyncServer &syncServer, QObject *parent = nullptr);
    ~CategoryModel();

    // QAbstractListModel 必要的实现方法
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    // 数据访问方法
    QStringList 获取类别() const;
    bool 加载类别(const QUuid &userUuid); // 从存储加载类别数据

    CategorieItem *寻找类别(const QString &name) const;
    CategorieItem *寻找类别(int id) const;
    CategorieItem *寻找类别(const QUuid &uuid) const;

    bool 新增类别(const QString &name, const QUuid &userUuid);
    bool 更新类别(const QString &name, const QString &newName);
    bool 删除类别(const QString &name);

    void 与服务器同步();
    void 更新同步成功状态(const std::vector<CategorieItem *> &categories);
    bool 导入类别从JSON(const QJsonArray &jsonArray,
                        CategoryDataStorage::ImportSource source = CategoryDataStorage::Server);

  signals:
    void categoriesChanged(); ///< 类别列表变化信号

  public slots:
    void onCategoriesChanged(); ///< 处理类别数据变化

  private:
    // 辅助方法
    QVariant 获取项目数据(const CategorieItem *item, int role) const; ///< 根据角色获取项目数据
    bool 是否是有效名称(const QString &name) const;
    void 开始更新模型(); ///< 开始模型更新
    void 结束更新模型(); ///< 结束模型更新

    // 成员变量
    std::vector<std::unique_ptr<CategorieItem>> m_categoryItems; ///< 类别项目列表

    CategoryDataStorage &m_dataStorage; ///< 数据存储引用
    CategorySyncServer &m_syncServer;   ///< 同步服务器引用
};