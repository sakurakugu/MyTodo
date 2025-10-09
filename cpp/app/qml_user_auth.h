/**
 * @file qml_user_auth.h
 * @brief QmlUserAuth QML 暴露层头文件
 *
 * 将底层 `UserAuth` 逻辑与 QML 解耦：
 * - 仅转发 UI 所需的属性 / 方法 / 信号
 * - 便于后续在不影响 QML 的情况下重构 UserAuth 内部实现
 * - 充当 Facade / Adapter
 *
 * @author Sakurakugu
 * @date 2025-10-06 01:02:00 (UTC+8) 周一
 * @change 2025-10-06 01:32:19(UTC+8) 周一
 */
#pragma once

#include <QObject>
#include <QString>
#include <QUuid>

class UserAuth; // 前向声明底层认证类

/**
 * @class QmlUserAuth
 * @brief QML 层用户认证门面（轻量包装转发）
 *
 * 设计目标：
 * - 与业务逻辑 UserAuth 分离，降低 QML 对内部实现的耦合
 * - 只暴露 QML 当前使用的最小集合；若 UI 新增需求，再增量扩展
 * - 可在此添加 UI 相关状态（例如 loading 状态聚合）而不污染核心类
 */
class QmlUserAuth : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString username READ username NOTIFY usernameChanged)
    Q_PROPERTY(QString email READ email NOTIFY emailChanged)
    Q_PROPERTY(QUuid uuid READ uuid NOTIFY uuidChanged)
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY isLoggedInChanged)
  public:
    explicit QmlUserAuth(UserAuth &auth, QObject *parent = nullptr);

    // 基本属性访问（转发）
    QString username() const;
    QString email() const;
    QUuid uuid() const;
    bool isLoggedIn() const;

    // QML 可调用方法
    Q_INVOKABLE void login(const QString &account, const QString &password);
    Q_INVOKABLE void logout();

  signals: // 直接转发底层信号
    void usernameChanged();
    void emailChanged();
    void uuidChanged();
    void isLoggedInChanged();
    void loginSuccessful(const QString &username);
    void loginFailed(const QString &errorMessage);
    void loginRequired();
    void logoutSuccessful();

  private slots:
    void onBaseUrlChanged(); // 服务器基础URL变化槽

  private:
    UserAuth &m_auth; ///< 引用底层认证逻辑
    void connectSignals();
};
