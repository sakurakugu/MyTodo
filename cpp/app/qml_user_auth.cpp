/**
 * @file qml_user_auth.cpp
 * @brief QmlUserAuth 实现
 *
 * @author Sakurakugu
 * @date 2025-10-06 01:02:00 (UTC+8) 周一
 * @change 2025-10-06 01:32:19(UTC+8) 周一
 */

#include "qml_user_auth.h"
#include "user_auth.h"

QmlUserAuth::QmlUserAuth(UserAuth &auth, QObject *parent) //
    : QObject(parent),                                            //
      m_auth(auth)                                                //
{
    connectSignals();
}

// ============== 属性访问 ==============
QString QmlUserAuth::username() const {
    return m_auth.获取用户名();
}
QString QmlUserAuth::email() const {
    return m_auth.获取邮箱();
}
QUuid QmlUserAuth::uuid() const {
    return m_auth.获取UUID();
}
bool QmlUserAuth::isLoggedIn() const {
    return m_auth. 是否已登录();
}

// ============== 方法转发 ==============
void QmlUserAuth::login(const QString &account, const QString &password) {
    m_auth.登录(account, password);
}
void QmlUserAuth::logout() {
    m_auth.注销();
}

// ============== 信号桥接 ==============
void QmlUserAuth::connectSignals() {
    // 使用 lambda 直接转发，避免多继承/友元侵入
    QObject::connect(&m_auth, &UserAuth::usernameChanged, this, &QmlUserAuth::usernameChanged);
    QObject::connect(&m_auth, &UserAuth::emailChanged, this, &QmlUserAuth::emailChanged);
    QObject::connect(&m_auth, &UserAuth::uuidChanged, this, &QmlUserAuth::uuidChanged);
    QObject::connect(&m_auth, &UserAuth::isLoggedInChanged, this, &QmlUserAuth::isLoggedInChanged);
    QObject::connect(&m_auth, &UserAuth::loginSuccessful, this, &QmlUserAuth::loginSuccessful);
    QObject::connect(&m_auth, &UserAuth::loginFailed, this, &QmlUserAuth::loginFailed);
    QObject::connect(&m_auth, &UserAuth::loginRequired, this, &QmlUserAuth::loginRequired);
    QObject::connect(&m_auth, &UserAuth::logoutSuccessful, this, &QmlUserAuth::logoutSuccessful);
    QObject::connect(&m_auth, &UserAuth::authTokenExpired, this, &QmlUserAuth::authTokenExpired);
    QObject::connect(&m_auth, &UserAuth::tokenRefreshStarted, this, &QmlUserAuth::tokenRefreshStarted);
    QObject::connect(&m_auth, &UserAuth::tokenRefreshSuccessful, this, &QmlUserAuth::tokenRefreshSuccessful);
    QObject::connect(&m_auth, &UserAuth::tokenRefreshFailed, this, &QmlUserAuth::tokenRefreshFailed);
    QObject::connect(&m_auth, &UserAuth::firstAuthCompleted, this, &QmlUserAuth::firstAuthCompleted);
}
