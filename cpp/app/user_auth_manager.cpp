/**
 * @file user_auth_manager.cpp
 * @brief UserAuthManager 实现
 *
 * @author Sakurakugu
 * @date 2025-10-06 01:02:00 (UTC+8) 周一
 * @change 2025-10-06 01:32:19(UTC+8) 周一
 */

#include "user_auth_manager.h"
#include "user_auth.h"

UserAuthManager::UserAuthManager(UserAuth &auth, QObject *parent) //
    : QObject(parent),                                            //
      m_auth(auth)                                                //
{
    connectSignals();
}

// ============== 属性访问 ==============
QString UserAuthManager::username() const {
    return m_auth.获取用户名();
}
QString UserAuthManager::email() const {
    return m_auth.获取邮箱();
}
QUuid UserAuthManager::uuid() const {
    return m_auth.获取UUID();
}
bool UserAuthManager::isLoggedIn() const {
    return m_auth. 是否已登录();
}

// ============== 方法转发 ==============
void UserAuthManager::login(const QString &account, const QString &password) {
    m_auth.登录(account, password);
}
void UserAuthManager::logout() {
    m_auth.注销();
}

// ============== 信号桥接 ==============
void UserAuthManager::connectSignals() {
    // 使用 lambda 直接转发，避免多继承/友元侵入
    QObject::connect(&m_auth, &UserAuth::usernameChanged, this, &UserAuthManager::usernameChanged);
    QObject::connect(&m_auth, &UserAuth::emailChanged, this, &UserAuthManager::emailChanged);
    QObject::connect(&m_auth, &UserAuth::uuidChanged, this, &UserAuthManager::uuidChanged);
    QObject::connect(&m_auth, &UserAuth::isLoggedInChanged, this, &UserAuthManager::isLoggedInChanged);
    QObject::connect(&m_auth, &UserAuth::loginSuccessful, this, &UserAuthManager::loginSuccessful);
    QObject::connect(&m_auth, &UserAuth::loginFailed, this, &UserAuthManager::loginFailed);
    QObject::connect(&m_auth, &UserAuth::loginRequired, this, &UserAuthManager::loginRequired);
    QObject::connect(&m_auth, &UserAuth::logoutSuccessful, this, &UserAuthManager::logoutSuccessful);
    QObject::connect(&m_auth, &UserAuth::authTokenExpired, this, &UserAuthManager::authTokenExpired);
    QObject::connect(&m_auth, &UserAuth::tokenRefreshStarted, this, &UserAuthManager::tokenRefreshStarted);
    QObject::connect(&m_auth, &UserAuth::tokenRefreshSuccessful, this, &UserAuthManager::tokenRefreshSuccessful);
    QObject::connect(&m_auth, &UserAuth::tokenRefreshFailed, this, &UserAuthManager::tokenRefreshFailed);
    QObject::connect(&m_auth, &UserAuth::firstAuthCompleted, this, &UserAuthManager::firstAuthCompleted);
}
