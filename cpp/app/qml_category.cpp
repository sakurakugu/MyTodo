/**
 * @file qml_category.cpp
 * @brief QmlCategoryManager类的实现文件
 *
 * 该文件实现了QmlCategoryManager类，用于管理待办事项的类别。
 * 专门负责类别的CRUD操作和服务器同步。
 *
 * @author Sakurakugu
 * @date 2025-08-25 01:28:49(UTC+8) 周一
 * @change 2025-10-06 02:13:23(UTC+8) 周一
 */

#include "qml_category.h"
#include "modules/category/category_item.h"
#include "modules/category/category_data_storage.h"
#include "modules/category/category_model.h"
#include "modules/category/category_sync_server.h"
#include "default_value.h"
#include "user_auth.h"

#include <QJsonDocument>
#include <algorithm>

QmlCategoryManager::QmlCategoryManager(UserAuth &userAuth, QObject *parent)
    : QObject(parent),                                                         //
      m_syncServer(new CategorySyncServer(userAuth, this)),                    //
      m_dataStorage(new CategoryDataStorage(this)),                            //
      m_categoryModel(new CategoryModel(*m_dataStorage, *m_syncServer, this)), //
      m_userAuth(userAuth)                                                     //
{

    // 连接同步服务器信号
    connect(m_syncServer, &CategorySyncServer::categoriesUpdatedFromServer, this,
            &QmlCategoryManager::onCategoriesUpdatedFromServer);
    connect(m_syncServer, &CategorySyncServer::localChangesUploaded, this,
            &QmlCategoryManager::onLocalChangesUploaded);

    // 桥接模型变化到管理器的属性通知：确保 QML 对 QmlCategoryManager.categories 的绑定会更新
    connect(m_categoryModel, &CategoryModel::categoriesChanged, this, &QmlCategoryManager::categoriesChanged);

    // 从存储加载类别数据（本地缓存）
    loadCategories();
}

QmlCategoryManager::~QmlCategoryManager() {}

QStringList QmlCategoryManager::getCategories() const {
    return m_categoryModel->获取类别();
}

/**
 * @brief 检查类别名称是否已存在
 * @param name 类别名称
 * @return 如果存在返回true，否则返回false
 */
bool QmlCategoryManager::categoryExists(const QString &name) const {
    return m_categoryModel->寻找类别(name) != nullptr;
}

void QmlCategoryManager::createCategory(const QString &name) {
    m_categoryModel->新增类别(name, m_userAuth.获取UUID());
}

void QmlCategoryManager::updateCategory(const QString &name, const QString &newName) {
    m_categoryModel->更新类别(name, newName);
}

void QmlCategoryManager::deleteCategory(const QString &name) {
    m_categoryModel->删除类别(name);
}

void QmlCategoryManager::loadCategories() {
    m_categoryModel->加载类别(m_userAuth.获取UUID());
}

// 同步相关方法实现
void QmlCategoryManager::syncWithServer() {
    m_categoryModel->与服务器同步();
}

bool QmlCategoryManager::isSyncing() const {
    return m_syncServer->isSyncing();
}

/**
 * @brief 处理从服务器更新的类别数据
 * @param categoriesArray 类别数据数组
 */
void QmlCategoryManager::onCategoriesUpdatedFromServer(const QJsonArray &categoriesArray) {
    m_categoryModel->导入类别从JSON(categoriesArray);
}

void QmlCategoryManager::onLocalChangesUploaded(const std::vector<CategorieItem *> &succeedSyncedItems) {
    m_categoryModel->更新同步成功状态(succeedSyncedItems);
}
