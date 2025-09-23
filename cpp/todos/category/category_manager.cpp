/**
 * @file category_manager.cpp
 * @brief CategoryManager类的实现文件
 *
 * 该文件实现了CategoryManager类，用于管理待办事项的类别。
 * 专门负责类别的CRUD操作和服务器同步。
 *
 * @author Sakurakugu
 * @date 2025-08-25 01:28:49(UTC+8) 周一
 * @change 2025-09-03 00:37:33(UTC+8) 周三
 * @version 0.4.0
 */

#include "category_manager.h"
#include "default_value.h"
#include <QDebug>
#include <QJsonDocument>
#include <algorithm>

CategoryManager::CategoryManager(UserAuth &userAuth, QObject *parent)
    : QObject(parent),                                                         //
      m_syncServer(new CategorySyncServer(userAuth, this)),                    //
      m_dataStorage(new CategoryDataStorage(this)),                            //
      m_categoryModel(new CategoryModel(*m_dataStorage, *m_syncServer, this)), //
      m_userAuth(userAuth)                                                     //
{

    // 连接同步服务器信号
    connect(m_syncServer, &CategorySyncServer::categoriesUpdatedFromServer, this,
            &CategoryManager::onCategoriesUpdatedFromServer);

    // 从存储加载类别数据
    loadCategories();
}

CategoryManager::~CategoryManager() {
}

QStringList CategoryManager::getCategories() const {
    return m_categoryModel->获取类别();
}

/**
 * @brief 检查类别名称是否已存在
 * @param name 类别名称
 * @return 如果存在返回true，否则返回false
 */
bool CategoryManager::categoryExists(const QString &name) const {
    return m_categoryModel->寻找类别(name) != nullptr;
}

void CategoryManager::createCategory(const QString &name) {
    m_categoryModel->新增类别(name, m_userAuth.getUuid());
}

void CategoryManager::updateCategory(const QString &name, const QString &newName) {
    m_categoryModel->更新类别(name, newName);
}

void CategoryManager::deleteCategory(const QString &name) {
    m_categoryModel->删除类别(name);
}

void CategoryManager::loadCategories() {
    m_categoryModel->加载类别(m_userAuth.getUuid());
}

// 同步相关方法实现
void CategoryManager::syncWithServer() {
    m_categoryModel->与服务器同步();
}

bool CategoryManager::isSyncing() const {
    return m_syncServer ? m_syncServer->isSyncing() : false;
}

/**
 * @brief 处理从服务器更新的类别数据
 * @param categoriesArray 类别数据数组
 */
void CategoryManager::onCategoriesUpdatedFromServer(const QJsonArray &categoriesArray) {
    m_categoryModel->导入类别从JSON(categoriesArray);
}

void CategoryManager::onLocalChangesUploaded(const std::vector<CategorieItem *> &m_succeedSyncedItems) {
    m_categoryModel->更新同步成功状态(m_succeedSyncedItems);
}
