/**
 * @brief 构造函数
 *
 * 初始化TodoManager对象，设置父对象为parent。
 *
 * @param parent 父对象指针，默认值为nullptr。
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @change 2025-09-06 01:29:53(UTC+8) 周六
 * @version 0.4.0
 */
#include "todo_manager.h"
#include "../category/category_data_storage.h"
#include "../category/category_manager.h"
#include "../category/category_sync_server.h"
#include "foundation/network_request.h"
#include "global_state.h"
#include "todo_queryer.h"
#include "user_auth.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkProxy>
#include <QProcess>
#include <QUuid>

TodoManager::TodoManager(UserAuth &userAuth, CategoryManager &categoryManager,
                         QObject *parent)
    : QObject(parent),                                                             //
      m_networkRequest(NetworkRequest::GetInstance()),                             //
      m_userAuth(userAuth),                                                        //
      m_globalState(GlobalState::GetInstance()),                                   //
      m_dataManager(new TodoDataStorage(this)),                                    //
      m_syncManager(new TodoSyncServer(userAuth, this)),                           //
      m_categoryManager(&categoryManager),                                         //
      m_queryer(new TodoQueryer(this)),                                            //
      m_todoModel(new TodoModel(*m_dataManager, *m_syncManager, *m_queryer, this)) //
{
    loadTodo();
}

/**
 * @brief 析构函数
 *
 * 清理资源，保存未同步的数据到本地存储。
 */
TodoManager::~TodoManager() {}

void TodoManager::loadTodo() {
    m_todoModel->加载待办();
}

/**
 * @brief 添加新的待办事项
 * @param title 任务标题（必填）
 * @param category 任务分类（默认为"未分类"）
 */
void TodoManager::addTodo(const QString &title, const QString &description, const QString &category, bool important,
                          const QString &deadline, int recurrenceInterval, int recurrenceCount,
                          const QDate &recurrenceStartDate) {
    m_todoModel->新增待办(title, m_userAuth.getUuid(), description, category, important, //
                          QDateTime::fromString(deadline, Qt::ISODate),                  //
                          recurrenceInterval, recurrenceCount, recurrenceStartDate);
}

/**
 * @brief 更新现有待办事项
 * @param index 待更新事项的索引
 * @param todoData 包含要更新字段的映射
 * @return 更新是否成功
 */
bool TodoManager::updateTodo(int index, const QString &roleName, const QVariant &value) {
    QVariantMap todoData;
    todoData[roleName] = value;
    return m_todoModel->更新待办(index, todoData);
}

/**
 * @brief 删除或从回收站恢复待办事项
 * @param index 待删除事项的索引
 * @return 操作是否成功
 */
bool TodoManager::markAsRemove(int index, bool remove) {
    return m_todoModel->标记删除(index, remove);
}

bool TodoManager::permanentlyDeleteTodo(int index) {
    return m_todoModel->删除待办(index);
}

/**
 * @brief 删除所有待办事项
 * @param deleteLocal 是否删除本地数据
 */
void TodoManager::deleteAllTodos(bool deleteLocal) {
    m_todoModel->删除所有待办(deleteLocal);
}

void TodoManager::syncWithServer() {
    m_todoModel->与服务器同步();
}

/**
 * @brief 处理首次认证完成信号
 * 当用户首次认证完成时，触发此槽函数，用于同步待办事项。
 */
void TodoManager::onFirstAuthCompleted() {
    syncWithServer();
}

/**
 * @brief 将待办事项标记为已完成
 * @param index 待办事项的索引
 * @return 操作是否成功
 */
bool TodoManager::markAsDone(int index, bool remove) {
    return m_todoModel->标记完成(index, remove);
}

// 访问器方法
TodoQueryer *TodoManager::queryer() const {
    return m_queryer;
}

TodoModel *TodoManager::todoModel() const {
    return m_todoModel;
}
