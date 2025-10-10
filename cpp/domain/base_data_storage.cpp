/**
 * @file base_data_storage.cpp
 * @brief BaseDataStorage基类的实现文件
 *
 * 该文件实现了BaseDataStorage基类的通用功能，为子类提供
 * 数据库操作、导出器管理等基础服务。
 *
 * @author Sakurakugu
 * @date 2025-10-05 01:07:45(UTC+8) 周日
 * @change 2025-10-06 01:32:19(UTC+8) 周一
 */

#include "base_data_storage.h"
#include <QDebug>

/**
 * @brief 构造函数
 * @param exporterName 导出器名称，用于在数据库中注册
 * @param parent 父对象
 */
BaseDataStorage::BaseDataStorage(const QString &exporterName, QObject *parent)
    : QObject(parent), m_database(Database::GetInstance()), m_exporterName(exporterName) {
    // 注册到数据库导出器
    m_database.registerDataExporter(m_exporterName.toStdString(), this);
}

/**
 * @brief 析构函数
 */
BaseDataStorage::~BaseDataStorage() {
    // 从数据库导出器中注销
    m_database.unregisterDataExporter(m_exporterName.toStdString());
}

/**
 * @brief 初始化数据存储
 * @return 初始化是否成功
 */
bool BaseDataStorage::初始化() {
    if (!m_database.initialize()) {
        qCritical() << m_exporterName << "数据库初始化失败";
        return false;
    }
    // 初始化数据表
    if (!初始化数据表()) {
        qCritical() << m_exporterName << "数据表初始化失败";
        return false;
    }
    return true;
}

/**
 * @brief 执行SQL查询（无参数）
 * @param queryString SQL查询语句
 * @return 执行是否成功
 */
bool BaseDataStorage::执行SQL查询(const std::string &queryString) {
    auto query = m_database.createQuery();
    if (!query->exec(queryString)) {
        qCritical() << "SQL查询执行失败:" << QString::fromStdString(m_database.lastError());
        qCritical() << "查询语句:" << QString::fromStdString(queryString);
        return false;
    }
    return true;
}

/**
 * @brief 执行SQL查询（带参数）
 * @param queryString SQL查询语句
 * @param params 查询参数
 * @return 执行是否成功
 */
bool BaseDataStorage::执行SQL查询(const std::string &queryString, const std::vector<SqlValue> &params) {
    auto query = m_database.createQuery();
    query->bindValues(params);
    if (!query->exec(queryString)) {
        qCritical() << "SQL查询执行失败:" << QString::fromStdString(m_database.lastError());
        qCritical() << "查询语句:" << QString::fromStdString(queryString);
        return false;
    }
    return true;
}
