/**
 * @file qml_todo.cpp
 * @brief QmlTodoManager类的实现文件
 *
 * 该文件实现了QmlTodoManager类，用于管理待办事项的增删改查和同步操作。
 *
 * @author Sakurakugu
 * @date 2025-08-16 20:05:55(UTC+8) 周六
 * @change 2025-10-06 02:13:23(UTC+8) 周一
 */
#include "qml_todo.h"
#include "modules/todo/todo_data_storage.h" // 数据管理器
#include "modules/todo/todo_item.h"         // 待办事项数据结构
#include "modules/todo/todo_sync_server.h"  // 服务器同步管理器
#include "network_request.h"                // 网络请求
#include "qml_category.h"                   // 类别管理器
#include "qml_global_data.h"
#include "user_auth.h"

#include <QDateTime>

QmlTodoManager::QmlTodoManager(UserAuth &userAuth, QObject *parent)
    : QObject(parent),                                                             //
      m_networkRequest(NetworkRequest::GetInstance()),                             //
      m_userAuth(userAuth),                                                        //
      m_dataManager(new TodoDataStorage(this)),                                    //
      m_syncManager(new TodoSyncServer(userAuth, this)),                           //
      m_queryer(new TodoQueryer(this)),                                            //
      m_todoModel(new TodoModel(*m_dataManager, *m_syncManager, *m_queryer, this)) //
{
    // 转发底层同步信号到自身，供 QML 的 Connections 使用
    connect(m_syncManager, &BaseSyncServer::syncStarted, this, &QmlTodoManager::syncStarted);
    connect(m_syncManager, &BaseSyncServer::syncCompleted, this, &QmlTodoManager::syncCompleted);
    loadTodo();
}

/**
 * @brief 析构函数
 *
 * 清理资源，保存未同步的数据到本地存储。
 */
QmlTodoManager::~QmlTodoManager() {}

void QmlTodoManager::loadTodo() {
    m_todoModel->加载待办();
}

/**
 * @brief 添加新的待办事项
 * @param title 任务标题（必填）
 * @param category 任务分类（默认为"未分类"）
 */
void QmlTodoManager::addTodo(const QString &title, const QString &description, const QString &category, bool important,
                             const QString &deadline, int recurrenceInterval, int recurrenceCount,
                             const QDate &recurrenceStartDate) {
    m_todoModel->新增待办(title, m_userAuth.获取UUID(), description, category, important, //
                          QDateTime::fromString(deadline, Qt::ISODate),                   //
                          recurrenceInterval, recurrenceCount, recurrenceStartDate);
}

/**
 * @brief 更新现有待办事项
 * @param index 待更新事项的索引
 * @param todoData 包含要更新字段的映射
 * @return 更新是否成功
 */
bool QmlTodoManager::updateTodo(int index, const QString &roleName, const QVariant &value) {
    QVariantMap todoData;
    todoData[roleName] = value;
    return m_todoModel->更新待办(index, todoData);
}

/**
 * @brief 更新现有待办事项（使用全局状态中的选中待办）
 * @return 更新是否成功
 */
bool QmlTodoManager::updateTodo() {
    const QVariant selectedTodo = QmlGlobalData::GetInstance().selectedTodo();
    if (!selectedTodo.isValid()) {
        return false;
    }

    const QVariantMap todoMap = selectedTodo.toMap();
    const int index = todoMap.value("index", -1).toInt();
    if (index < 0 || index >= m_todoModel->rowCount()) {
        return false;
    }

    return m_todoModel->更新待办(index, std::move(todoMap));
}

/**
 * @brief 删除或从回收站恢复待办事项
 * @param index 待删除事项的索引
 * @return 操作是否成功
 */
bool QmlTodoManager::markAsRemove(int index, bool remove) {
    return m_todoModel->标记删除(index, remove);
}

bool QmlTodoManager::permanentlyDeleteTodo(int index) {
    return m_todoModel->删除待办(index);
}

/**
 * @brief 删除所有待办事项
 * @param deleteLocal 是否删除本地数据
 */
void QmlTodoManager::deleteAllTodos(bool deleteLocal) {
    m_todoModel->删除所有待办(deleteLocal, m_userAuth.获取UUID());
}

void QmlTodoManager::syncWithServer() {
    m_todoModel->与服务器同步();
}

bool QmlTodoManager::isSyncing() const {
    return m_syncManager->isSyncing();
}

/**
 * @brief 处理首次认证完成信号
 * 当用户首次认证完成时，触发此槽函数，用于同步待办事项。
 */
void QmlTodoManager::onFirstAuthCompleted() {
    syncWithServer();
}

/**
 * @brief 将待办事项标记为已完成
 * @param index 待办事项的索引
 * @return 操作是否成功
 */
bool QmlTodoManager::markAsDone(int index, bool remove) {
    return m_todoModel->标记完成(index, remove);
}

// 访问器方法
TodoQueryer *QmlTodoManager::queryer() const {
    return m_queryer;
}

TodoModel *QmlTodoManager::todoModel() const {
    return m_todoModel;
}
