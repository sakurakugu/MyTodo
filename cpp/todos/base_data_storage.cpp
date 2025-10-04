/**
 * @file base_data_storage.cpp
 * @brief BaseDataStorage基类的实现文件
 *
 * 该文件实现了BaseDataStorage基类的通用功能，为子类提供
 * 数据库操作、导出器管理等基础服务。
 *
 * @author Sakurakugu
 * @date 2025-10-05 01:07 (UTC+8)
 */

#include "base_data_storage.h"
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>

/**
 * @brief 构造函数
 * @param exporterName 导出器名称，用于在数据库中注册
 * @param parent 父对象
 */
BaseDataStorage::BaseDataStorage(const QString &exporterName, QObject *parent)
    : QObject(parent),
      m_database(Database::GetInstance()),
      m_exporterName(exporterName)
{
    // 注册到数据库导出器
    m_database.registerDataExporter(m_exporterName, this);
}

/**
 * @brief 析构函数
 */
BaseDataStorage::~BaseDataStorage() {
    // 从数据库导出器中注销
    m_database.unregisterDataExporter(m_exporterName);
}

/**
 * @brief 初始化数据存储
 * @return 初始化是否成功
 */
bool BaseDataStorage::初始化() {
    // 初始化数据表
    if (!初始化数据表()) {
        qCritical() << m_exporterName << "数据表初始化失败";
        return false;
    }
    return true;
}

/**
 * @brief 获取最后插入行的ID
 * @param db 数据库连接
 * @return 最后插入行的ID，失败返回-1
 */
int BaseDataStorage::获取最后插入行ID(QSqlDatabase &db) const {
    QSqlQuery idQuery(db);
    int newId = -1;
    
    if (idQuery.exec("SELECT last_insert_rowid()") && idQuery.next()) {
        newId = idQuery.value(0).toInt();
    }
    
    if (newId <= 0) {
        qWarning() << "获取自增ID失败，使用临时ID -1";
        return -1;
    }
    
    return newId;
}

/**
 * @brief 执行SQL查询（带返回结果）
 * @param queryString SQL查询语句
 * @param query 查询对象引用
 * @return 执行是否成功
 */
bool BaseDataStorage::执行SQL查询(const QString &queryString, QSqlQuery &query) {
    if (!query.exec(queryString)) {
        qCritical() << "SQL查询执行失败:" << query.lastError().text();
        qCritical() << "查询语句:" << queryString;
        return false;
    }
    return true;
}

/**
 * @brief 执行SQL查询（无返回结果）
 * @param queryString SQL查询语句
 * @return 执行是否成功
 */
bool BaseDataStorage::执行SQL查询(const QString &queryString) {
    QSqlDatabase db = m_database.getDatabase();
    if (!db.isOpen()) {
        qCritical() << "数据库未打开，无法执行查询";
        return false;
    }
    
    QSqlQuery query(db);
    return 执行SQL查询(queryString, query);
}