/**
 * @file category_item.cpp
 * @brief CategorieItem类的实现文件
 *
 * 该文件实现了CategorieItem类，用于表示分类的数据模型。
 * CategorieItem包含了分类的所有属性，如ID、名称、用户ID、是否为默认分类等，
 * 并提供了相应的getter和setter方法，支持Qt的属性系统和信号槽机制。
 *
 * @author Sakurakugu
 * @date 2025-08-22 12:06:17(UTC+8) 周五
 * @change 2025-09-22 19:36:18(UTC+8) 周一
 */

#include "category_item.h"

/**
 * @brief 默认构造函数
 *
 * 创建一个空的CategorieItem对象，所有字符串属性为空，
 * 时间属性为当前时间，同步状态为false。
 *
 * @param parent 父对象指针，用于Qt的对象树管理
 */
CategorieItem::CategorieItem()
    : m_id(0),                                   // 初始化分类ID为0
      m_uuid(QUuid()),                           // 初始化分类UUID为空UUID
      m_name(""),                                // 初始化分类名称为空字符串
      m_userUuid(QUuid()),                       // 初始化用户UUID为空UUID
      m_createdAt(QDateTime::currentDateTime()), // 初始化创建时间为当前时间
      m_updatedAt(QDateTime::currentDateTime()), // 初始化更新时间为当前时间
      m_synced(1)                                // 初始化是否已同步为false
{}

/**
 * @brief 带参数的构造函数
 *
 * 使用指定的参数创建CategorieItem对象。这个构造函数通常用于
 * 从数据库或网络加载已存在的分类数据。
 */
CategorieItem::CategorieItem(   //
    int id,                     ///< 分类唯一标识符
    const QUuid &uuid,          ///< 分类唯一标识符（UUID)
    const QString &name,        ///< 分类名称
    const QUuid &userUuid,      ///< 用户UUID
    const QDateTime &createdAt, ///< 创建时间
    const QDateTime &updatedAt, ///< 更新时间
    int synced)                 ///< 是否已与服务器同步
    : m_id(id),                 ///< 初始化分类ID
      m_uuid(uuid),             ///< 初始化分类UUID
      m_name(name),             ///< 初始化分类名称
      m_userUuid(userUuid),     ///< 初始化用户UUID
      m_createdAt(createdAt),   ///< 初始化创建时间
      m_updatedAt(updatedAt),   ///< 初始化更新时间
      m_synced(synced)          ///< 初始化同步状态
{}

/**
 * @brief 设置分类的唯一标识符
 * @param id 新的分类ID
 */
void CategorieItem::setId(int id) {
    if (m_id != id)
        m_id = id;
}

/**
 * @brief 设置分类的唯一标识符
 * @param uuid 新的分类UUID
 */
void CategorieItem::setUuid(const QUuid &uuid) {
    if (m_uuid != uuid)
        m_uuid = uuid;
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
    if (m_name != name_) {
        m_name = name_;
    }
}

/**
 * @brief 设置用户UUID
 * @param userUuid 新的用户UUID
 */
void CategorieItem::setUserUuid(const QUuid &userUuid) {
    if (m_userUuid != userUuid)
        m_userUuid = userUuid;
}

/**
 * @brief 设置分类创建时间
 * @param createdAt 新的创建时间
 */
void CategorieItem::setCreatedAt(const QDateTime &createdAt) {
    if (m_createdAt != createdAt)
        m_createdAt = createdAt;
}

/**
 * @brief 设置分类更新时间
 * @param updatedAt 新的更新时间
 */
void CategorieItem::setUpdatedAt(const QDateTime &updatedAt) {
    if (m_updatedAt != updatedAt)
        m_updatedAt = updatedAt;
}

/**
 * @brief 设置分类同步状态
 * @param synced 新的同步状态
 */
void CategorieItem::setSynced(int synced) {
    if (m_synced == synced)
        return;

    // 如果之前是新建(1)，现在要改为更新(2)，保持不变
    if (m_synced == 1 && synced == 2)
        return;

    m_synced = synced;
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
 * @brief 移动赋值运算符
 * @param other 要移动的CategorieItem对象
 * @return 当前对象的引用
 */
CategorieItem &CategorieItem::operator=(CategorieItem &&other) noexcept {
    if (this != &other) {
        m_id = std::move(other.m_id);
        m_uuid = std::move(other.m_uuid);
        m_name = std::move(other.m_name);
        m_userUuid = std::move(other.m_userUuid);
        m_createdAt = std::move(other.m_createdAt);
        m_updatedAt = std::move(other.m_updatedAt);
        m_synced = std::move(other.m_synced);
    }
    return *this;
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