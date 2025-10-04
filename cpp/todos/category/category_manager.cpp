/**
 * @file category_manager.cpp
 * @brief CategoryManager类的实现文件
 *
 * 该文件实现了CategoryManager类，用于管理待办事项的类别。
 * 专门负责类别的CRUD操作和服务器同步。
 *
 * @author Sakurakugu
 * @date 2025-08-25 01:28:49(UTC+8) 周一
 * @change 2025-09-24 03:45:31(UTC+8) 周三
 */

#include "category_manager.h"
#include "category_item.h"
#include "category_data_storage.h"
#include "category_model.h"
#include "category_sync_server.h"
#include "default_value.h"
#include "cpp/app/user_auth.h"

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
    // 关键：同步成功后需要写回数据库（更新 synced=0），之前缺少该连接导致只改内存未持久化
    connect(m_syncServer, &CategorySyncServer::localChangesUploaded, this,
            &CategoryManager::onLocalChangesUploaded);

    // 桥接模型变化到管理器的属性通知：确保 QML 对 categoryManager.categories 的绑定会更新
    connect(m_categoryModel, &CategoryModel::categoriesChanged, this, &CategoryManager::categoriesChanged);

    // 从存储加载类别数据（本地缓存）
    loadCategories();

    // 自动同步逻辑：
    // 1. 监听首次认证完成（包含自动 refresh token 成功 或 手动登录成功后第一次）
    // 2. 监听显式 loginSuccessful（保证如果 firstAuthCompleted 因某种异常未发出仍可触发）
    // 3. 仅当当前未在同步、且用户处于登录状态时触发一次双向同步
    // auto triggerInitialSync = [this]() {
    //     if (!m_userAuth.isLoggedIn()) {
    //         return; // 未登录不触发
    //     }
    //     if (m_syncServer && !m_syncServer->isSyncing()) {
    //         // 双向同步：服务器数据优先拉取，再推送本地未同步更改
    //         m_categoryModel->与服务器同步();
    //     }
    // };

    // connect(&m_userAuth, &UserAuth::firstAuthCompleted, this, 与服务器同步());
    // // connect(&m_userAuth, &UserAuth::loginSuccessful, this, triggerInitialSync);

    // // 如果应用启动时已经有有效 session（例如刷新令牌成功并立即拿到 access token），也做一次延迟触发
    // if (m_userAuth.isLoggedIn()) {
    //     QMetaObject::invokeMethod(this, [triggerInitialSync]() { triggerInitialSync(); }, Qt::QueuedConnection);
    // }
}

CategoryManager::~CategoryManager() {}

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

void CategoryManager::onLocalChangesUploaded(const std::vector<CategorieItem *> &succeedSyncedItems) {
    m_categoryModel->更新同步成功状态(succeedSyncedItems);
}
