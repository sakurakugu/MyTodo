/**
 * @file categorie_item.h
 * @brief CategorieItem类的头文件
 *
 * 该文件定义了CategorieItem类，用于表示分类的数据模型。
 *
 * @author Sakurakugu
 * @date 2025-08-22 12:06:17(UTC+8) 周五
 * @version 2025-08-22 14:55:36(UTC+8) 周五
 */

#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <concepts>
#include <type_traits>

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
 * @see TodoModel
 */
class CategorieItem : public QObject {
    Q_OBJECT
    Q_PROPERTY(int id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(int userId READ userId WRITE setUserId NOTIFY userIdChanged)
    Q_PROPERTY(bool isDefault READ isDefault WRITE setIsDefault NOTIFY isDefaultChanged)
    Q_PROPERTY(QDateTime createdAt READ createdAt WRITE setCreatedAt NOTIFY createdAtChanged)
    Q_PROPERTY(bool synced READ synced WRITE setSynced NOTIFY syncedChanged)

  public:
    CategorieItem(const CategorieItem &) = delete;
    CategorieItem &operator=(const CategorieItem &) = delete;
    explicit CategorieItem(QObject *parent = nullptr);
    CategorieItem(int id,                    ///< 分类唯一标识符
                  const QString &name,       ///< 分类名称
                  int userId,                ///< 用户ID
                  bool isDefault,            ///< 是否为默认分类
                  const QDateTime &createdAt, ///< 创建时间
                  bool synced,               ///< 是否已与服务器同步
                  QObject *parent = nullptr);

    int id() const noexcept { return m_id; } // 获取ID
    void setId(int id);                      // 设置ID

    QString name() const noexcept { return m_name; } // 获取分类名称
    void setName(const QString &name);               // 设置分类名称

    int userId() const noexcept { return m_userId; } // 获取用户ID
    void setUserId(int userId);                      // 设置用户ID

    bool isDefault() const noexcept { return m_isDefault; } // 获取是否为默认分类
    void setIsDefault(bool isDefault);                      // 设置是否为默认分类

    QDateTime createdAt() const noexcept { return m_createdAt; } // 获取创建时间
    void setCreatedAt(const QDateTime &createdAt);               // 设置创建时间

    bool synced() const noexcept { return m_synced; } // 获取是否已同步
    void setSynced(bool synced);                      // 设置是否已同步

    // 便利方法
    bool isValidName() const noexcept;                                    // 检查分类名称是否有效
    bool isSystemDefault() const noexcept;                                // 检查是否为系统默认分类
    QString displayName() const noexcept;                                 // 获取显示名称
    bool canBeDeleted() const noexcept;                                   // 检查是否可以被删除

    // 比较操作符
    bool operator==(const CategorieItem &other) const;
    bool operator!=(const CategorieItem &other) const;

  signals:
    void idChanged();        ///< ID改变信号
    void nameChanged();      ///< 分类名称改变信号
    void userIdChanged();    ///< 用户ID改变信号
    void isDefaultChanged(); ///< 默认分类状态改变信号
    void createdAtChanged(); ///< 创建时间改变信号
    void syncedChanged();    ///< 同步状态改变信号

  private:
    /**
     * @brief 通用属性设置方法
     * @tparam T 属性类型，必须支持比较和赋值操作
     * @param member 成员变量引用
     * @param value 新值
     * @param signal 信号指针
     */
    template <typename T>
    constexpr void setProperty(T& member, const T& value, void (CategorieItem::*signal)()) {
        if (member != value) {
            member = value;
            emit(this->*signal)();
        }
    }

    // 成员变量
    int m_id;                // 分类ID
    QString m_name;          // 分类名称
    int m_userId;            // 用户ID
    bool m_isDefault;        // 是否为默认分类
    QDateTime m_createdAt;   // 分类创建时间
    bool m_synced;           // 分类是否已同步
};