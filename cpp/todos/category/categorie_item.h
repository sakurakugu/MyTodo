/**
 * @file categorie_item.h
 * @brief CategorieItem类的头文件
 *
 * 该文件定义了CategorieItem类，用于表示分类的数据模型。
 *
 * @author Sakurakugu
 * @date 2025-08-22 12:06:17(UTC+8) 周五
 * @change 2025-09-23 18:45:36(UTC+8) 周二
 */

#pragma once

#include <QDateTime>
#include <QString>
#include <QUuid>

/**
 * @class CategorieItem
 * @brief 表示单个分类的数据模型类
 *
 * CategorieItem类封装了一个分类的所有属性，包括：
 * - 基本信息：ID、名称、用户ID
 * - 状态信息：是否为默认分类、创建时间
 * - 同步信息：是否已与服务器同步
 *
 * 该类继承自QObject，支持Qt的属性系统和信号槽机制，
 * 可以直接在QML中使用，实现数据绑定和自动更新UI。
 *
 * @note 所有属性都有对应的getter、setter方法和change信号
 * @see TodoManager
 */
class CategorieItem {
  public:
    CategorieItem(const CategorieItem &) = delete;
    CategorieItem &operator=(const CategorieItem &) = delete;
    explicit CategorieItem();
    CategorieItem(int id,                     ///< 分类唯一标识符
                  const QUuid &uuid,          ///< 分类唯一标识符（UUID）
                  const QString &name,        ///< 分类名称
                  const QUuid &userUuid,      ///< 用户UUID
                  const QDateTime &createdAt, ///< 创建时间
                  const QDateTime &updatedAt, ///< 更新时间
                  int synced                  ///< 是否已与服务器同步（是否要上传）
                                              ///< 0表示已同步，1表示插入，2表示更新，3表示删除
    );

    int id() const noexcept { return m_id; } // 获取ID
    void setId(int id);                      // 设置ID

    QUuid uuid() const { return m_uuid; } // 获取UUID
    void setUuid(const QUuid &uuid);      // 设置UUID

    QString name() const noexcept { return m_name; } // 获取分类名称
    void setName(const QString &name);               // 设置分类名称

    QUuid userUuid() const noexcept { return m_userUuid; } // 获取用户UUID
    void setUserUuid(const QUuid &userUuid);               // 设置用户UUID

    QDateTime createdAt() const noexcept { return m_createdAt; } // 获取创建时间
    void setCreatedAt(const QDateTime &createdAt);               // 设置创建时间

    QDateTime updatedAt() const noexcept { return m_updatedAt; } // 获取更新时间
    void setUpdatedAt(const QDateTime &updatedAt);               // 设置更新时间

    int synced() const noexcept { return m_synced; } // 获取是否已同步
    void setSynced(int synced);                      // 设置是否已同步

    // 便利方法
    bool isValidName() const noexcept;     // 检查分类名称是否有效
    bool isSystemDefault() const noexcept; // 检查是否为系统默认分类
    QString displayName() const noexcept;  // 获取显示名称
    bool canBeDeleted() const noexcept;    // 检查是否可以被删除

    // 比较操作符
    bool operator==(const CategorieItem &other) const;
    bool operator!=(const CategorieItem &other) const;

  private:
    // 成员变量
    int m_id;              // 分类ID
    QUuid m_uuid;          // 分类UUID
    QString m_name;        // 分类名称
    QUuid m_userUuid;      // 用户UUID
    QDateTime m_createdAt; // 分类创建时间
    QDateTime m_updatedAt; // 分类更新时间
    int m_synced;          // 分类是否已同步
};
