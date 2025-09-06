/**
 * @file categorie_item.cpp
 * @brief CategorieItem类的实现文件
 *
 * 该文件实现了CategorieItem类，用于表示分类的数据模型。
 * CategorieItem包含了分类的所有属性，如ID、名称、用户ID、是否为默认分类等，
 * 并提供了相应的getter和setter方法，支持Qt的属性系统和信号槽机制。
 *
 * @author Sakurakugu
 * @date 2025-08-22 12:06:17(UTC+8) 周五
 * @change 2025-09-03 00:37:33(UTC+8) 周三
 * @version 0.4.0
 */

#include "categorie_item.h"

/**
 * @brief 默认构造函数
 *
 * 创建一个空的CategorieItem对象，所有字符串属性为空，
 * 时间属性为当前时间，同步状态为false。
 *
 * @param parent 父对象指针，用于Qt的对象树管理
 */
CategorieItem::CategorieItem(QObject *parent)
    : QObject(parent),                           // 初始化父对象
      m_id(0),                                   // 初始化分类ID为0
      m_uuid(QUuid()),                           // 初始化分类UUID为空UUID
      m_name(""),                                // 初始化分类名称为空字符串
      m_userUuid(QUuid()),                       // 初始化用户UUID为空UUID
      m_createdAt(QDateTime::currentDateTime()), // 初始化创建时间为当前时间
      m_synced(false)                            // 初始化是否已同步为false
{
}

/**
 * @brief 带参数的构造函数
 *
 * 使用指定的参数创建CategorieItem对象。这个构造函数通常用于
 * 从数据库或网络加载已存在的分类数据。
 */
CategorieItem::CategorieItem(int id,                     ///< 分类唯一标识符
                             const QUuid &uuid,          ///< 分类唯一标识符（UUID)
                             const QString &name,        ///< 分类名称
                             const QUuid &userUuid,      ///< 用户UUID
                             const QDateTime &createdAt, ///< 创建时间
                             bool synced,                ///< 是否已与服务器同步
                             QObject *parent)            ///< 父对象指针
    : QObject(parent),                                   ///< 初始化父对象
      m_id(id),                                          ///< 初始化分类ID
      m_uuid(uuid),                                      ///< 初始化分类UUID
      m_name(name),                                      ///< 初始化分类名称
      m_userUuid(userUuid),                              ///< 初始化用户UUID
      m_createdAt(createdAt),                            ///< 初始化创建时间
      m_synced(synced)                                   ///< 初始化同步状态
{
}

/**
 * @brief 设置分类的唯一标识符
 * @param id 新的分类ID
 */
void CategorieItem::setId(int id) {
    setProperty(m_id, id, &CategorieItem::idChanged);
}

/**
 * @brief 设置分类的唯一标识符
 * @param uuid 新的分类UUID
 */
void CategorieItem::setUuid(const QUuid &uuid) {
    setProperty(m_uuid, uuid, &CategorieItem::uuidChanged);
}

/**
 * @brief 设置分类名称
 * @param name 新的分类名称
 */
void CategorieItem::setName(const QString &name) {
    QString name_;
    if (name.length() > 50) {
        name_ = name.left(40) + "......";
    } else [[likely]] {
        name_ = name;
    }
    setProperty(m_name, name_, &CategorieItem::nameChanged);
}

/**
 * @brief 设置用户UUID
 * @param userUuid 新的用户UUID
 */
void CategorieItem::setUserUuid(const QUuid &userUuid) {
    setProperty(m_userUuid, userUuid, &CategorieItem::userUuidChanged);
}

/**
 * @brief 设置分类创建时间
 * @param createdAt 新的创建时间
 */
void CategorieItem::setCreatedAt(const QDateTime &createdAt) {
    setProperty(m_createdAt, createdAt, &CategorieItem::createdAtChanged);
}

/**
 * @brief 设置分类同步状态
 * @param synced 新的同步状态
 */
void CategorieItem::setSynced(bool synced) {
    setProperty(m_synced, synced, &CategorieItem::syncedChanged);
}

// 便利方法实现

/**
 * @brief 检查分类名称是否有效
 * @return 如果名称有效返回true，否则返回false
 */
bool CategorieItem::isValidName() const noexcept {
    return !m_name.isEmpty() && !m_name.trimmed().isEmpty() && m_name.length() <= 50;
}

/**
 * @brief 检查是否为系统默认分类
 * @return 如果是系统默认分类返回true，否则返回false
 */
bool CategorieItem::isSystemDefault() const noexcept {
    return m_id == 1;
}

/**
 * @brief 获取显示名称
 * @return 用于显示的分类名称
 */
QString CategorieItem::displayName() const noexcept {
    if (m_name.isEmpty()) {
        return "未命名分类";
    }
    return m_name;
}

/**
 * @brief 检查是否可以被删除
 * @return 如果可以被删除返回true，否则返回false
 */
bool CategorieItem::canBeDeleted() const noexcept {
    // 系统默认分类不能被删除
    return !isSystemDefault();
}

/**
 * @brief 比较两个CategorieItem是否相等
 * @param other 另一个CategorieItem对象
 * @return 如果相等返回true，否则返回false
 */
bool CategorieItem::operator==(const CategorieItem &other) const {
    return m_id == other.m_id &&               // 主键ID
           m_name == other.m_name &&           // 分类名称
           m_userUuid == other.m_userUuid &&   // 用户ID
           m_createdAt == other.m_createdAt && // 创建时间
           m_synced == other.m_synced;         // 同步状态
}

/**
 * @brief 比较两个CategorieItem是否不相等
 * @param other 另一个CategorieItem对象
 * @return 如果不相等返回true，否则返回false
 */
bool CategorieItem::operator!=(const CategorieItem &other) const {
    return !(*this == other);
}